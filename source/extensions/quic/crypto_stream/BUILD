load(
    "@envoy_build_config//:extensions_build_config.bzl",
    "LEGACY_ALWAYSLINK",
)
load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_extension",
    "envoy_cc_library",
    "envoy_extension_package",
    "envoy_select_enable_http3",
)

licenses(["notice"])  # Apache 2

# Extensions of QUIC crypto stream.

envoy_extension_package()

envoy_cc_library(
    name = "envoy_quic_crypto_server_stream_lib",
    srcs = envoy_select_enable_http3(["envoy_quic_crypto_server_stream.cc"]),
    hdrs = envoy_select_enable_http3(["envoy_quic_crypto_server_stream.h"]),
    visibility = [
        "//source/common/quic:__subpackages__",
        "//test:__subpackages__",
    ],
    deps = envoy_select_enable_http3([
        "//envoy/registry",
        "//source/common/quic:envoy_quic_server_crypto_stream_factory_lib",
        "@envoy_api//envoy/extensions/quic/crypto_stream/v3:pkg_cc_proto",
    ]),
    alwayslink = LEGACY_ALWAYSLINK,
)

envoy_cc_extension(
    name = "envoy_quic_default_crypto_server_stream",
    extra_visibility = [
        "//source/common/quic:__subpackages__",
        "//test:__subpackages__",
    ],
    deps = envoy_select_enable_http3([
        ":envoy_quic_crypto_server_stream_lib",
    ]),
)

envoy_cc_library(
    name = "envoy_quic_crypto_client_stream_lib",
    srcs = envoy_select_enable_http3(["envoy_quic_crypto_client_stream.cc"]),
    hdrs = envoy_select_enable_http3(["envoy_quic_crypto_client_stream.h"]),
    visibility = [
        "//source/common/quic:__subpackages__",
        "//test:__subpackages__",
    ],
    deps = envoy_select_enable_http3([
        "//source/common/quic:envoy_quic_client_crypto_stream_factory_lib",
    ]),
    alwayslink = LEGACY_ALWAYSLINK,
)
