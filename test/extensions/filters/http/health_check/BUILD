load(
    "//bazel:envoy_build_system.bzl",
    "envoy_package",
)
load(
    "//test/extensions:extensions_build_system.bzl",
    "envoy_extension_cc_test",
)

licenses(["notice"])  # Apache 2

envoy_package()

envoy_extension_cc_test(
    name = "health_check_test",
    srcs = ["health_check_test.cc"],
    extension_names = ["envoy.filters.http.health_check"],
    rbe_pool = "6gig",
    deps = [
        "//source/common/buffer:buffer_lib",
        "//source/common/http:header_utility_lib",
        "//source/extensions/filters/http/health_check:health_check_lib",
        "//test/mocks/server:factory_context_mocks",
        "//test/test_common:utility_lib",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
    ],
)

envoy_extension_cc_test(
    name = "config_test",
    srcs = ["config_test.cc"],
    extension_names = ["envoy.filters.http.health_check"],
    rbe_pool = "6gig",
    deps = [
        "//source/extensions/filters/http/health_check:config",
        "//test/mocks/server:factory_context_mocks",
        "//test/test_common:utility_lib",
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/filters/http/health_check/v3:pkg_cc_proto",
    ],
)

envoy_extension_cc_test(
    name = "health_check_integration_test",
    size = "large",
    srcs = [
        "health_check_integration_test.cc",
    ],
    extension_names = ["envoy.filters.http.health_check"],
    rbe_pool = "6gig",
    deps = [
        "//source/extensions/filters/http/buffer:config",
        "//source/extensions/filters/http/health_check:config",
        "//test/integration:http_protocol_integration_lib",
        "//test/test_common:utility_lib",
    ],
)
