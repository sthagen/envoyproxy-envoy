#include <chrono>

#include "source/common/quic/client_connection_factory_impl.h"
#include "source/common/quic/quic_client_transport_socket_factory.h"

#include "test/common/upstream/utility.h"
#include "test/mocks/common.h"
#include "test/mocks/event/mocks.h"
#include "test/mocks/http/http_server_properties_cache.h"
#include "test/mocks/server/server_factory_context.h"
#include "test/mocks/ssl/mocks.h"
#include "test/mocks/upstream/cluster_info.h"
#include "test/mocks/upstream/host.h"
#include "test/test_common/environment.h"
#include "test/test_common/network_utility.h"
#include "test/test_common/simulated_time_system.h"
#include "test/test_common/threadsafe_singleton_injector.h"

#include "quiche/quic/core/crypto/quic_client_session_cache.h"
#include "quiche/quic/core/deterministic_connection_id_generator.h"

using testing::Return;

namespace Envoy {
namespace Quic {

constexpr int PEER_PORT = 54321;

class QuicNetworkConnectionTest : public Event::TestUsingSimulatedTime,
                                  public testing::TestWithParam<Network::Address::IpVersion> {
protected:
  void initialize() {
    ON_CALL(context_.server_context_, threadLocal()).WillByDefault(ReturnRef(thread_local_));
    EXPECT_CALL(*cluster_, perConnectionBufferLimitBytes()).WillOnce(Return(45));
    EXPECT_CALL(*cluster_, connectTimeout).WillOnce(Return(std::chrono::seconds(10)));
    auto* protocol_options = cluster_->http3_options_.mutable_quic_protocol_options();
    protocol_options->mutable_max_concurrent_streams()->set_value(43);
    protocol_options->mutable_initial_stream_window_size()->set_value(65555);
    protocol_options->set_connection_options("5RTO,ACKD");
    protocol_options->set_client_connection_options("6RTO,AKD4");
    quic_info_ = createPersistentQuicInfoForCluster(dispatcher_, *cluster_);
    EXPECT_EQ(quic_info_->quic_config_.max_time_before_crypto_handshake(),
              quic::QuicTime::Delta::FromSeconds(10));
    EXPECT_EQ(quic_info_->quic_config_.GetMaxBidirectionalStreamsToSend(),
              protocol_options->max_concurrent_streams().value());
    EXPECT_EQ(quic_info_->quic_config_.GetMaxUnidirectionalStreamsToSend(),
              protocol_options->max_concurrent_streams().value());
    EXPECT_EQ(quic_info_->quic_config_.GetInitialMaxStreamDataBytesIncomingBidirectionalToSend(),
              protocol_options->initial_stream_window_size().value());
    EXPECT_EQ(2, quic_info_->quic_config_.SendConnectionOptions().size());
    std::string quic_copts = "";
    for (auto& copt : quic_info_->quic_config_.SendConnectionOptions()) {
      quic_copts.append(quic::QuicTagToString(copt));
    }
    EXPECT_EQ(quic_copts, "5RTOACKD");
    EXPECT_EQ(
        2, quic_info_->quic_config_.ClientRequestedIndependentOptions(quic::Perspective::IS_CLIENT)
               .size());
    std::string quic_ccopts = "";
    for (auto& ccopt :
         quic_info_->quic_config_.ClientRequestedIndependentOptions(quic::Perspective::IS_CLIENT)) {
      quic_ccopts.append(quic::QuicTagToString(ccopt));
    }
    EXPECT_EQ(quic_ccopts, "6RTOAKD4");

    test_address_ = *Network::Utility::resolveUrl(absl::StrCat(
        "tcp://", Network::Test::getLoopbackAddressUrlString(GetParam()), ":", PEER_PORT));
    Ssl::ClientContextSharedPtr context{new Ssl::MockClientContext()};
    EXPECT_CALL(context_.server_context_.ssl_context_manager_, createSslClientContext(_, _))
        .WillOnce(Return(context));
    factory_ = *Quic::QuicClientTransportSocketFactory::create(
        std::unique_ptr<Envoy::Ssl::ClientContextConfig>(
            new NiceMock<Ssl::MockClientContextConfig>),
        context_);
    crypto_config_ = factory_->getCryptoConfig();
  }

  uint32_t highWatermark(EnvoyQuicClientSession* session) {
    return session->write_buffer_watermark_simulation_.highWatermark();
  }

  testing::NiceMock<ThreadLocal::MockInstance> thread_local_;
  NiceMock<Event::MockDispatcher> dispatcher_;
  std::unique_ptr<PersistentQuicInfoImpl> quic_info_;
  std::shared_ptr<Upstream::MockClusterInfo> cluster_{new NiceMock<Upstream::MockClusterInfo>()};
  Upstream::HostSharedPtr host_{new NiceMock<Upstream::MockHost>};
  NiceMock<Random::MockRandomGenerator> random_;
  Upstream::ClusterConnectivityState state_;
  Network::Address::InstanceConstSharedPtr test_address_;
  NiceMock<Server::Configuration::MockTransportSocketFactoryContext> context_;
  std::unique_ptr<Quic::QuicClientTransportSocketFactory> factory_;
  std::shared_ptr<quic::QuicCryptoClientConfig> crypto_config_;
  Stats::IsolatedStoreImpl store_;
  QuicStatNames quic_stat_names_{store_.symbolTable()};
  quic::DeterministicConnectionIdGenerator connection_id_generator_{
      quic::kQuicDefaultConnectionIdLength};
};

TEST_P(QuicNetworkConnectionTest, BufferLimits) {
  initialize();
  std::unique_ptr<Network::ClientConnection> client_connection = createQuicNetworkConnection(
      *quic_info_, crypto_config_,
      quic::QuicServerId{factory_->clientContextConfig()->serverNameIndication(), PEER_PORT},
      dispatcher_, test_address_, test_address_, quic_stat_names_, {}, *store_.rootScope(), nullptr,
      nullptr, connection_id_generator_, *factory_);
  EnvoyQuicClientSession* session = static_cast<EnvoyQuicClientSession*>(client_connection.get());
  session->Initialize();
  client_connection->connect();
  EXPECT_TRUE(client_connection->connecting());
  ASSERT(session != nullptr);
  EXPECT_EQ(highWatermark(session), 45);
  EXPECT_EQ(absl::nullopt, session->unixSocketPeerCredentials());
  EXPECT_NE(absl::nullopt, session->lastRoundTripTime());
  EXPECT_THAT(session->GetAlpnsToOffer(), testing::ElementsAre("h3"));
  client_connection->close(Network::ConnectionCloseType::NoFlush);
}

TEST_P(QuicNetworkConnectionTest, SocketOptions) {
  initialize();

  auto socket_option = std::make_shared<Network::MockSocketOption>();
  auto socket_options = std::make_shared<Network::ConnectionSocket::Options>();
  socket_options->push_back(socket_option);
  EXPECT_CALL(*socket_option, setOption(_, envoy::config::core::v3::SocketOption::STATE_PREBIND));
  EXPECT_CALL(*socket_option, setOption(_, envoy::config::core::v3::SocketOption::STATE_BOUND));
  EXPECT_CALL(*socket_option, setOption(_, envoy::config::core::v3::SocketOption::STATE_LISTENING));

  std::unique_ptr<Network::ClientConnection> client_connection = createQuicNetworkConnection(
      *quic_info_, crypto_config_,
      quic::QuicServerId{factory_->clientContextConfig()->serverNameIndication(), PEER_PORT},
      dispatcher_, test_address_, test_address_, quic_stat_names_, {}, *store_.rootScope(),
      socket_options, nullptr, connection_id_generator_, *factory_);
  EnvoyQuicClientSession* session = static_cast<EnvoyQuicClientSession*>(client_connection.get());
  session->Initialize();
  client_connection->connect();
  client_connection->close(Network::ConnectionCloseType::NoFlush);
}

TEST_P(QuicNetworkConnectionTest, PreBindSocketOptionsFailure) {
  initialize();

  auto socket_option = std::make_shared<Network::MockSocketOption>();
  auto socket_options = std::make_shared<Network::ConnectionSocket::Options>();
  socket_options->push_back(socket_option);
  EXPECT_CALL(*socket_option, setOption(_, envoy::config::core::v3::SocketOption::STATE_PREBIND))
      .WillOnce(Return(false));

  std::unique_ptr<Network::ClientConnection> client_connection = createQuicNetworkConnection(
      *quic_info_, crypto_config_,
      quic::QuicServerId{factory_->clientContextConfig()->serverNameIndication(), PEER_PORT},
      dispatcher_, test_address_, test_address_, quic_stat_names_, {}, *store_.rootScope(),
      socket_options, nullptr, connection_id_generator_, *factory_);
  EnvoyQuicClientSession* session = static_cast<EnvoyQuicClientSession*>(client_connection.get());
  session->Initialize();
  client_connection->connect();
  EXPECT_FALSE(session->connection()->connected());
  EXPECT_EQ(client_connection->state(), Network::Connection::State::Closed);
}

TEST_P(QuicNetworkConnectionTest, PostBindSocketOptionsFailure) {
  initialize();

  auto socket_option = std::make_shared<Network::MockSocketOption>();
  auto socket_options = std::make_shared<Network::ConnectionSocket::Options>();
  socket_options->push_back(socket_option);
  EXPECT_CALL(*socket_option, setOption(_, envoy::config::core::v3::SocketOption::STATE_PREBIND));
  EXPECT_CALL(*socket_option, setOption(_, envoy::config::core::v3::SocketOption::STATE_BOUND))
      .WillOnce(Return(false));

  std::unique_ptr<Network::ClientConnection> client_connection = createQuicNetworkConnection(
      *quic_info_, crypto_config_,
      quic::QuicServerId{factory_->clientContextConfig()->serverNameIndication(), PEER_PORT},
      dispatcher_, test_address_, test_address_, quic_stat_names_, {}, *store_.rootScope(),
      socket_options, nullptr, connection_id_generator_, *factory_);
  EnvoyQuicClientSession* session = static_cast<EnvoyQuicClientSession*>(client_connection.get());
  session->Initialize();
  client_connection->connect();
  EXPECT_FALSE(session->connection()->connected());
  EXPECT_EQ(client_connection->state(), Network::Connection::State::Closed);
}

TEST_P(QuicNetworkConnectionTest, LocalAddress) {
  initialize();
  Network::Address::InstanceConstSharedPtr local_addr =
      (GetParam() == Network::Address::IpVersion::v6)
          ? Network::Utility::getIpv6LoopbackAddress()
          : Network::Utility::getCanonicalIpv4LoopbackAddress();
  std::unique_ptr<Network::ClientConnection> client_connection = createQuicNetworkConnection(
      *quic_info_, crypto_config_,
      quic::QuicServerId{factory_->clientContextConfig()->serverNameIndication(), PEER_PORT},
      dispatcher_, test_address_, local_addr, quic_stat_names_, {}, *store_.rootScope(), nullptr,
      nullptr, connection_id_generator_, *factory_);
  EnvoyQuicClientSession* session = static_cast<EnvoyQuicClientSession*>(client_connection.get());
  session->Initialize();
  client_connection->connect();
  EXPECT_TRUE(client_connection->connecting());
  EXPECT_EQ(Network::Connection::State::Open, client_connection->state());
  EXPECT_THAT(client_connection->connectionInfoProvider().localAddress(), testing::NotNull());
  if (GetParam() == Network::Address::IpVersion::v6) {
    EXPECT_TRUE(client_connection->connectionInfoProvider().localAddress()->ip()->ipv6()->v6only());
  }
  client_connection->close(Network::ConnectionCloseType::NoFlush);
}

class MockGetSockOptSysCalls : public Api::OsSysCallsImpl {
public:
  MOCK_METHOD(Api::SysCallIntResult, getsockopt,
              (os_fd_t sockfd, int level, int optname, void* optval, socklen_t* optlen));
};

TEST_P(QuicNetworkConnectionTest, GetV6OnlySocketOptionFailure) {
  if (GetParam() == Network::Address::IpVersion::v4) {
    return;
  }
  initialize();
  MockGetSockOptSysCalls os_sys_calls;
  TestThreadsafeSingletonInjector<Api::OsSysCallsImpl> singleton_injector{&os_sys_calls};

  EXPECT_CALL(os_sys_calls, getsockopt(_, IPPROTO_IPV6, IPV6_V6ONLY, _, _))
      .WillOnce(Return(Api::SysCallIntResult{-1, SOCKET_ERROR_NOT_SUP}));
  std::unique_ptr<Network::ClientConnection> client_connection = createQuicNetworkConnection(
      *quic_info_, crypto_config_,
      quic::QuicServerId{factory_->clientContextConfig()->serverNameIndication(), PEER_PORT},
      dispatcher_, test_address_, test_address_, quic_stat_names_, {}, *store_.rootScope(), nullptr,
      nullptr, connection_id_generator_, *factory_);
  EnvoyQuicClientSession* session = static_cast<EnvoyQuicClientSession*>(client_connection.get());
  session->Initialize();
  client_connection->connect();
  EXPECT_TRUE(client_connection->connecting());
  EXPECT_FALSE(session->connection()->connected());
  EXPECT_EQ(client_connection->state(), Network::Connection::State::Closed);
}

TEST_P(QuicNetworkConnectionTest, Srtt) {
  initialize();

  Http::MockHttpServerPropertiesCache rtt_cache;
  PersistentQuicInfoImpl info{dispatcher_, 45};

  EXPECT_CALL(rtt_cache, getSrtt).WillOnce(Return(std::chrono::microseconds(5)));

  std::unique_ptr<Network::ClientConnection> client_connection = createQuicNetworkConnection(
      info, crypto_config_,
      quic::QuicServerId{factory_->clientContextConfig()->serverNameIndication(), PEER_PORT},
      dispatcher_, test_address_, test_address_, quic_stat_names_, rtt_cache, *store_.rootScope(),
      nullptr, nullptr, connection_id_generator_, *factory_);

  EnvoyQuicClientSession* session = static_cast<EnvoyQuicClientSession*>(client_connection.get());

  EXPECT_EQ(session->config()->GetInitialRoundTripTimeUsToSend(), 5);
  session->Initialize();
  client_connection->connect();
  EXPECT_TRUE(client_connection->connecting());
  client_connection->close(Network::ConnectionCloseType::NoFlush);
}

INSTANTIATE_TEST_SUITE_P(IpVersions, QuicNetworkConnectionTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()));
} // namespace Quic
} // namespace Envoy
