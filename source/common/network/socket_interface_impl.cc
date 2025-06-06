#include "source/common/network/socket_interface_impl.h"

#include "envoy/common/exception.h"
#include "envoy/extensions/network/socket_interface/v3/default_socket_interface.pb.h"
#include "envoy/extensions/network/socket_interface/v3/default_socket_interface.pb.validate.h"

#include "source/common/api/os_sys_calls_impl.h"
#include "source/common/common/assert.h"
#include "source/common/common/utility.h"
#include "source/common/network/address_impl.h"
#include "source/common/network/io_socket_handle_impl.h"
#include "source/common/network/win32_socket_handle_impl.h"

#if defined(__linux__) && !defined(__ANDROID_API__) && defined(ENVOY_ENABLE_IO_URING)
#include "source/common/io/io_uring_worker_factory_impl.h"
#include "source/common/io/io_uring_impl.h"
#include "source/common/network/io_uring_socket_handle_impl.h"
#endif

namespace Envoy {
namespace Network {

namespace {
[[maybe_unused]] bool hasIoUringWorkerFactory(Io::IoUringWorkerFactory* io_uring_worker_factory) {
  return io_uring_worker_factory != nullptr && io_uring_worker_factory->currentThreadRegistered() &&
         io_uring_worker_factory->getIoUringWorker() != absl::nullopt;
}
} // namespace

void DefaultSocketInterfaceExtension::onWorkerThreadInitialized() {
  if (io_uring_worker_factory_ != nullptr) {
    io_uring_worker_factory_->onWorkerThreadInitialized();
  }
}

IoHandlePtr SocketInterfaceImpl::makePlatformSpecificSocket(
    int socket_fd, bool socket_v6only, absl::optional<int> domain,
    const SocketCreationOptions& options,
    [[maybe_unused]] Io::IoUringWorkerFactory* io_uring_worker_factory) {
  if constexpr (Event::PlatformDefaultTriggerType == Event::FileTriggerType::EmulatedEdge) {
    return std::make_unique<Win32SocketHandleImpl>(socket_fd, socket_v6only, domain);
  }
#if defined(__linux__) && !defined(__ANDROID_API__) && defined(ENVOY_ENABLE_IO_URING)
  // Only create IoUringSocketHandleImpl when the IoUringWorkerFactory has been created and it has
  // been registered in the TLS, initialized. There are cases that test may create threads before
  // IoUringWorkerFactory has been added to the TLS and got initialized.
  if (hasIoUringWorkerFactory(io_uring_worker_factory)) {
    return std::make_unique<IoUringSocketHandleImpl>(*io_uring_worker_factory, socket_fd,
                                                     socket_v6only, domain);
  }
#endif
  return std::make_unique<IoSocketHandleImpl>(socket_fd, socket_v6only, domain,
                                              options.max_addresses_cache_size_);
}

IoHandlePtr SocketInterfaceImpl::makeSocket(int socket_fd, bool socket_v6only,
                                            Socket::Type socket_type, absl::optional<int> domain,
                                            const SocketCreationOptions& options) const {
  if (socket_type == Socket::Type::Datagram) {
    return makePlatformSpecificSocket(socket_fd, socket_v6only, domain, options, nullptr);
  }
  return makePlatformSpecificSocket(socket_fd, socket_v6only, domain, options,
                                    io_uring_worker_factory_.lock().get());
}

IoHandlePtr SocketInterfaceImpl::socket(Socket::Type socket_type, Address::Type addr_type,
                                        Address::IpVersion version, bool socket_v6only,
                                        const SocketCreationOptions& options) const {
  int protocol = 0;
#if defined(__APPLE__) || defined(WIN32)
  ASSERT(!options.mptcp_enabled_, "MPTCP is only supported on Linux");
  int flags = 0;
#else
  int flags = SOCK_NONBLOCK;

  // When io_uring is enabled, SOCK_NONBLOCK becomes redundant. io_uring can multiplex sockets on
  // its own, and the EAGAIN caused by SOCK_NONBLOCK can lead to unnecessary event triggers.
  if (hasIoUringWorkerFactory(io_uring_worker_factory_.lock().get()) &&
      socket_type == Socket::Type::Stream) {
    flags = 0;
  }

  if (options.mptcp_enabled_) {
    ASSERT(socket_type == Socket::Type::Stream);
    ASSERT(addr_type == Address::Type::Ip);
    protocol = IPPROTO_MPTCP;
  }
#endif

  if (socket_type == Socket::Type::Stream) {
    flags |= SOCK_STREAM;
  } else {
    flags |= SOCK_DGRAM;
  }

  int domain;
  if (addr_type == Address::Type::Ip) {
    if (version == Address::IpVersion::v6 || Address::forceV6()) {
      domain = AF_INET6;
    } else {
      ASSERT(version == Address::IpVersion::v4);
      domain = AF_INET;
    }
  } else if (addr_type == Address::Type::Pipe) {
    domain = AF_UNIX;
  } else {
    ASSERT(addr_type == Address::Type::EnvoyInternal);
    PANIC("not implemented");
    // TODO(lambdai): Add InternalIoSocketHandleImpl to support internal address.
    return nullptr;
  }

  const Api::SysCallSocketResult result =
      Api::OsSysCallsSingleton::get().socket(domain, flags, protocol);
  if (!SOCKET_VALID(result.return_value_)) {
    IS_ENVOY_BUG(fmt::format("socket(2) failed, got error: {}", errorDetails(result.errno_)));
    return nullptr;
  }
  IoHandlePtr io_handle =
      makeSocket(result.return_value_, socket_v6only, socket_type, domain, options);

#if defined(__APPLE__) || defined(WIN32)
  // Cannot set SOCK_NONBLOCK as a ::socket flag.
  const int rc = io_handle->setBlocking(false).return_value_;
  if (SOCKET_FAILURE(result.return_value_)) {
    IS_ENVOY_BUG(fmt::format("Unable to set socket non-blocking: got error: {}", rc));
    return nullptr;
  }
#endif

  return io_handle;
}

IoHandlePtr SocketInterfaceImpl::socket(Socket::Type socket_type,
                                        const Address::InstanceConstSharedPtr addr,
                                        const SocketCreationOptions& options) const {
  Address::IpVersion ip_version = addr->ip() ? addr->ip()->version() : Address::IpVersion::v4;
  int v6only = 0;
  if (addr->type() == Address::Type::Ip && ip_version == Address::IpVersion::v6) {
    v6only = addr->ip()->ipv6()->v6only();
  }

  IoHandlePtr io_handle =
      SocketInterfaceImpl::socket(socket_type, addr->type(), ip_version, v6only, options);
  if (io_handle && addr->type() == Address::Type::Ip && ip_version == Address::IpVersion::v6 &&
      !Address::forceV6()) {
    // Setting IPV6_V6ONLY restricts the IPv6 socket to IPv6 connections only.
    const Api::SysCallIntResult result = io_handle->setOption(
        IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&v6only), sizeof(v6only));
    ENVOY_BUG(!SOCKET_FAILURE(result.return_value_),
              fmt::format("Unable to set socket v6-only: got error: {}", result.return_value_));
  }
  return io_handle;
}

bool SocketInterfaceImpl::ipFamilySupported(int domain) {
  Api::OsSysCalls& os_sys_calls = Api::OsSysCallsSingleton::get();
  const Api::SysCallSocketResult result = os_sys_calls.socket(domain, SOCK_STREAM, 0);
  if (SOCKET_VALID(result.return_value_)) {
    RELEASE_ASSERT(
        os_sys_calls.close(result.return_value_).return_value_ == 0,
        fmt::format("Fail to close fd: response code {}", errorDetails(result.return_value_)));
  }
  return SOCKET_VALID(result.return_value_);
}

Server::BootstrapExtensionPtr SocketInterfaceImpl::createBootstrapExtension(
    [[maybe_unused]] const Protobuf::Message& config,
    [[maybe_unused]] Server::Configuration::ServerFactoryContext& context) {
#if defined(__linux__) && !defined(__ANDROID_API__) && defined(ENVOY_ENABLE_IO_URING)
  const auto& message = MessageUtil::downcastAndValidate<
      const envoy::extensions::network::socket_interface::v3::DefaultSocketInterface&>(
      config, context.messageValidationVisitor());
  if (message.has_io_uring_options() && Io::isIoUringSupported()) {
    const auto& options = message.io_uring_options();
    std::shared_ptr<Io::IoUringWorkerFactoryImpl> io_uring_worker_factory =
        std::make_shared<Io::IoUringWorkerFactoryImpl>(
            PROTOBUF_GET_WRAPPED_OR_DEFAULT(options, io_uring_size, 1000),
            options.enable_submission_queue_polling(),
            PROTOBUF_GET_WRAPPED_OR_DEFAULT(options, read_buffer_size, 8192),
            PROTOBUF_GET_WRAPPED_OR_DEFAULT(options, write_timeout_ms, 1000),
            context.threadLocal());
    io_uring_worker_factory_ = io_uring_worker_factory;

    return std::make_unique<DefaultSocketInterfaceExtension>(*this, io_uring_worker_factory);
  } else {
    return std::make_unique<DefaultSocketInterfaceExtension>(*this, nullptr);
  }
#else
  return std::make_unique<DefaultSocketInterfaceExtension>(*this, nullptr);
#endif
}

ProtobufTypes::MessagePtr SocketInterfaceImpl::createEmptyConfigProto() {
  return std::make_unique<
      envoy::extensions::network::socket_interface::v3::DefaultSocketInterface>();
}

REGISTER_FACTORY(SocketInterfaceImpl, Server::Configuration::BootstrapExtensionFactory);

static SocketInterfaceLoader* socket_interface_ =
    new SocketInterfaceLoader(std::make_unique<SocketInterfaceImpl>());

} // namespace Network
} // namespace Envoy
