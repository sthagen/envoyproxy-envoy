load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_extension_package",
)

licenses(["notice"])  # Apache 2

envoy_extension_package()

envoy_cc_library(
    name = "logical_host_lib",
    srcs = ["logical_host.cc"],
    hdrs = ["logical_host.h"],
    deps = [
        "//source/common/upstream:upstream_includes",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
        "@envoy_api//envoy/config/endpoint/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "dns_cluster_backcompat_lib",
    srcs = ["dns_cluster_backcompat.cc"],
    hdrs = ["dns_cluster_backcompat.h"],
    deps = [
        "//source/common/upstream:cluster_factory_includes",
        "//source/common/upstream:upstream_includes",
        "@envoy_api//envoy/config/cluster/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/clusters/dns/v3:pkg_cc_proto",
    ],
)
