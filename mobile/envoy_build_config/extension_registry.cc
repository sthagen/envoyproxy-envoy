#include "extension_registry.h"

#ifdef ENVOY_MOBILE_REQUEST_COMPRESSION
#include "source/extensions/compression/brotli/compressor/config.h"
#include "source/extensions/compression/gzip/compressor/config.h"
#include "source/extensions/filters/http/composite/action.h"
#include "source/extensions/filters/http/composite/config.h"
#include "source/extensions/filters/http/compressor/config.h"
#endif

#include "source/common/http/match_delegate/config.h"
#include "source/common/http/matching/inputs.h"
#include "source/common/network/default_client_connection_factory.h"
#include "source/common/network/socket_interface_impl.h"
#include "source/common/router/upstream_codec_filter.h"
#include "source/common/watchdog/abort_action_config.h"
#include "source/extensions/clusters/dynamic_forward_proxy/cluster.h"
#include "source/extensions/clusters/static/static_cluster.h"
#include "source/extensions/compression/brotli/decompressor/config.h"
#include "source/extensions/compression/gzip/decompressor/config.h"
#include "source/extensions/early_data/default_early_data_policy.h"
#include "source/extensions/filters/http/alternate_protocols_cache/config.h"
#include "source/extensions/filters/http/buffer/config.h"
#include "source/extensions/filters/http/decompressor/config.h"
#include "source/extensions/filters/http/dynamic_forward_proxy/config.h"
#include "source/extensions/filters/http/router/config.h"
#include "source/extensions/filters/network/http_connection_manager/config.h"
#include "source/extensions/http/header_formatters/preserve_case/config.h"
#include "source/extensions/http/header_validators/envoy_default/config.h"
#include "source/extensions/http/original_ip_detection/xff/config.h"
#include "source/extensions/network/dns_resolver/getaddrinfo/getaddrinfo.h"
#include "source/extensions/path/match/uri_template/config.h"
#include "source/extensions/path/rewrite/uri_template/config.h"
#include "source/extensions/request_id/uuid/config.h"
#include "source/extensions/transport_sockets/http_11_proxy/config.h"
#include "source/extensions/transport_sockets/raw_buffer/config.h"
#include "source/extensions/transport_sockets/tls/cert_validator/default_validator.h"
#include "source/extensions/transport_sockets/tls/config.h"
#include "source/extensions/udp_packet_writer/default/config.h"
#include "source/extensions/upstreams/http/generic/config.h"

#ifdef ENVOY_MOBILE_ENABLE_LISTENER
#include "source/extensions/listener_managers/listener_manager/listener_manager_impl.h"
#include "source/extensions/listener_managers/listener_manager/connection_handler_impl.h"
#endif

#ifdef ENVOY_ENABLE_QUIC
#ifdef ENVOY_MOBILE_ENABLE_LISTENER
#include "source/common/quic/server_codec_impl.h"
#include "source/extensions/quic/connection_id_generator/envoy_deterministic_connection_id_generator_config.h"
#include "source/extensions/quic/crypto_stream/envoy_quic_crypto_server_stream.h"
#include "source/extensions/quic/proof_source/envoy_quic_proof_source_factory_impl.h"
#endif
#include "source/common/quic/quic_transport_socket_factory.h"
#endif

#ifdef ENVOY_MOBILE_STATS_REPORTING
#include "source/extensions/clusters/logical_dns/logical_dns_cluster.h"
#include "source/extensions/stat_sinks/metrics_service/config.h"
#include "source/extensions/stat_sinks/statsd/config.h"
#endif

#include "extension_registry_platform_additions.h"
#include "library/common/extensions/cert_validator/platform_bridge/config.h"
#include "library/common/extensions/filters/http/assertion/config.h"
#include "library/common/extensions/filters/http/local_error/config.h"
#include "library/common/extensions/filters/http/network_configuration/config.h"
#include "library/common/extensions/filters/http/platform_bridge/config.h"
#include "library/common/extensions/filters/http/route_cache_reset/config.h"
#include "library/common/extensions/filters/http/socket_tag/config.h"
#include "library/common/extensions/key_value/platform/config.h"
#include "library/common/extensions/listener_managers/api_listener_manager/api_listener_manager.h"
#include "library/common/extensions/retry/options/network_configuration/config.h"

namespace Envoy {

void ExtensionRegistry::registerFactories() {
  Common::Http::MatchDelegate::Factory::forceRegisterSkipActionFactory();
  Common::Http::MatchDelegate::forceRegisterMatchDelegateConfig();

  ExtensionRegistryPlatformAdditions::registerFactories();

  // The uuid extension is required for E-M for server mode. Ideally E-M could skip it.
  Extensions::RequestId::forceRegisterUUIDRequestIDExtensionFactory();
  // This is the original IP detection code which ideally E-M could skip.
  Extensions::Http::OriginalIPDetection::Xff::forceRegisterXffIPDetectionFactory();

  // TODO(alyssar) verify with Lyft that we can move this to be a test-only filter.
  Extensions::HttpFilters::Assertion::forceRegisterAssertionFilterFactory();
  // TODO(alyssar) verify with Lyft that we can move this to be a test-only filter.
  Extensions::HttpFilters::BufferFilter::forceRegisterBufferFilterFactory();
  // TODO(alyssawilk) verify with Lyft we can move this to be a test-only filter.
  Extensions::HttpFilters::RouteCacheReset::forceRegisterRouteCacheResetFilterFactory();
  // TODO(alyssawilk) verify with Lyft we can move this to be a test-only filter.
  Upstream::forceRegisterStaticClusterFactory();

  // This is the default cluster used by Envoy mobile to establish connections upstream.
  Extensions::Clusters::DynamicForwardProxy::forceRegisterClusterFactory();
  // This allows decompression of brotli-compresssed responses.
  Extensions::Compression::Brotli::Decompressor::forceRegisterBrotliDecompressorLibraryFactory();
  // This allows decompression of gzip-decompressed responses.
  Extensions::Compression::Gzip::Decompressor::forceRegisterGzipDecompressorLibraryFactory();
  // This is the base decompressor filter used for both gzip and brotli.
  Extensions::HttpFilters::Decompressor::forceRegisterDecompressorFilterFactory();
  // This allows HTTP/1.1 requests to preserve their case, as many servers for example do not
  // correctly content-length headers and instead expect Content-Length.
  Extensions::Http::HeaderFormatters::PreserveCase::
      forceRegisterPreserveCaseFormatterFactoryConfig();
  // This is for UHV-based header validation.
  Extensions::Http::HeaderValidators::EnvoyDefault::forceRegisterHeaderValidatorFactoryConfig();

  // This caches upstream protocol capabilities to maximize latency for H2 and H3 endpoints.
  Extensions::HttpFilters::AlternateProtocolsCache::
      forceRegisterAlternateProtocolsCacheFilterFactory();
  // This handles DNS lookup for all upstream requests.
  Extensions::HttpFilters::DynamicForwardProxy::forceRegisterDynamicForwardProxyFilterFactory();
  // This converts Envoy "local reply" errors to not look like remote replies to the app.
  Extensions::HttpFilters::LocalError::forceRegisterLocalErrorFilterFactory();
  // This filter handles mobile-specific network config like interface binding and system proxies.
  Extensions::HttpFilters::NetworkConfiguration::forceRegisterNetworkConfigurationFilterFactory();
  // This filter, if configured, allows platform-specific filters e.g. swift or kotlin.
  Extensions::HttpFilters::PlatformBridge::forceRegisterPlatformBridgeFilterFactory();
  // This is Envoy's router filter, required for a functional L7 data plane.
  Extensions::HttpFilters::RouterFilter::forceRegisterRouterFilterConfig();
  // This is Envoy's codec filter, required for a functional L7 data plane.
  Router::forceRegisterUpstreamCodecFilterFactory();

  // This filter applies socket tagging based on the x-envoy-mobile-socket-tag header.
  Extensions::HttpFilters::SocketTag::forceRegisterSocketTagFilterFactory();
  // The k-v store allows caching things like DNS and prefered protocol across application restarts.
  Extensions::KeyValue::forceRegisterPlatformKeyValueStoreFactory();
  // This is Envoy's HCM filter, currently required for a functional L7 data plane.
  Extensions::NetworkFilters::HttpConnectionManager::
      forceRegisterHttpConnectionManagerFilterConfigFactory();
  // This works with the connectivity manager to allow retries across network interfaces.
  Extensions::Retry::Options::forceRegisterNetworkConfigurationRetryOptionsPredicateFactory();
  // TODO(alyssawilk) move to the listener build.
  Extensions::TransportSockets::RawBuffer::forceRegisterDownstreamRawBufferSocketFactory();
  // This is the default certificate validator, still compiled by default but hopefully soon to be
  // deprecated in production by iOS and Android platform validators.
  Extensions::TransportSockets::Tls::forceRegisterDefaultCertValidatorFactory();
  // This is the base for the still-being-validated platform validators.
  Extensions::TransportSockets::Tls::forceRegisterPlatformBridgeCertValidatorFactory();

  // This transport socket handles upstream TLS connections.
  Extensions::TransportSockets::Tls::forceRegisterUpstreamSslSocketFactory();
  // This transport socket handles doing CONNECT requests to any configured system proxies.
  Extensions::TransportSockets::Http11Connect::
      forceRegisterUpstreamHttp11ConnectSocketConfigFactory();
  Extensions::Upstreams::Http::forceRegisterProtocolOptionsConfigFactory();
  // This transport socket handles plaintext (http) traffic.
  Extensions::TransportSockets::RawBuffer::forceRegisterUpstreamRawBufferSocketFactory();

  // This is the default HTTP connection pool factory required for L7 upstream traffic.
  Extensions::Upstreams::Http::Generic::forceRegisterGenericGenericConnPoolFactory();
  // This is the default TCP client connection factory required for L7 upstream traffic.
  Network::forceRegisterDefaultClientConnectionFactory();
  // This is the default socket factory required for L7 traffic.
  Network::forceRegisterSocketInterfaceImpl();
  // This is the RE factory, required at least if any config uses regex, which stats do.
  Regex::forceRegisterGoogleReEngineFactory();

  // These are required to support specific route configs, if they are on.
  // It's likely no current users of E-M require them so we could optionally compile out by default.
  Router::forceRegisterDefaultEarlyDataPolicyFactory();
  Router::forceRegisterRouteListMatchActionFactory();
  Router::forceRegisterRouteMatchActionFactory();
  Extensions::UriTemplate::Match::forceRegisterUriTemplateMatcherFactory();
  Extensions::UriTemplate::Rewrite::forceRegisterUriTemplateRewriterFactory();
  Http::Matching::forceRegisterHttpRequestHeadersDataInputFactory();
  Http::Matching::forceRegisterHttpRequestTrailersDataInputFactory();
  Http::Matching::forceRegisterHttpResponseHeadersDataInputFactory();
  Http::Matching::forceRegisterHttpResponseTrailersDataInputFactory();

  // Envoy Mobile uses the GetAddrInfo resolver for DNS lookups by default.
  Network::forceRegisterGetAddrInfoDnsResolverFactory();

  // TODO(alyssawilk) looks like this is only needed for H3 downstream. Test and remove.
  Network::forceRegisterUdpDefaultWriterFactoryFactory();

  // This is Envoy's lightweight listener manager which lets E-M avoid the 1M
  // hit of compiling in downstream code.
  Server::forceRegisterApiListenerManagerFactoryImpl();

  // This is required code for certain watchdog config, required until Envoy
  // Mobile compiles out watchdog support.
  Watchdog::forceRegisterAbortActionFactory();

#ifdef ENVOY_MOBILE_STATS_REPORTING
  Network::Address::forceRegisterIpResolver();
  Upstream::forceRegisterLogicalDnsClusterFactory();
  Extensions::StatSinks::MetricsService::forceRegisterMetricsServiceSinkFactory();
  Extensions::StatSinks::Statsd::forceRegisterStatsdSinkFactory();
#endif

#ifdef ENVOY_MOBILE_ENABLE_LISTENER
  // These are downstream factories required if Envoy Mobile is compiled with
  // proxy functionality.
  Server::forceRegisterConnectionHandlerFactoryImpl();
  Server::forceRegisterDefaultListenerManagerFactoryImpl();
  Server::FilterChain::forceRegisterFilterChainNameActionFactory();
#endif

#ifdef ENVOY_ENABLE_QUIC

#ifdef ENVOY_MOBILE_ENABLE_LISTENER
  // These are QUIC downstream factories required if Envoy Mobile is compiled with
  // proxy functionality and QUIC support.
  Server::forceRegisterConnectionHandlerFactoryImpl();
  Quic::forceRegisterQuicHttpServerConnectionFactoryImpl();
  Quic::forceRegisterQuicServerTransportSocketConfigFactory();
  Quic::forceRegisterEnvoyQuicProofSourceFactoryImpl();
  Quic::forceRegisterEnvoyDeterministicConnectionIdGeneratorConfigFactory();
#endif
  Quic::forceRegisterQuicClientTransportSocketConfigFactory();
#endif

#ifdef ENVOY_MOBILE_REQUEST_COMPRESSION
  // These are factories required for request decompression.
  Extensions::Compression::Brotli::Compressor::forceRegisterBrotliCompressorLibraryFactory();
  Extensions::Compression::Gzip::Compressor::forceRegisterGzipCompressorLibraryFactory();
  Extensions::HttpFilters::Composite::forceRegisterCompositeFilterFactory();
  Extensions::HttpFilters::Composite::forceRegisterExecuteFilterActionFactory();
  Extensions::HttpFilters::Compressor::forceRegisterCompressorFilterFactory();
#endif
}

} // namespace Envoy
