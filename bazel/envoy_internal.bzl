# DO NOT LOAD THIS FILE. Targets from this file should be considered private
# and not used outside of the @envoy//bazel package.
load("@bazel_skylib//lib:selects.bzl", "selects")
load(":envoy_select.bzl", "envoy_select_admin_html", "envoy_select_disable_exceptions", "envoy_select_disable_logging", "envoy_select_google_grpc", "envoy_select_hot_restart", "envoy_select_nghttp2", "envoy_select_signal_trace", "envoy_select_static_extension_registration")

# Compute the final copts based on various options.
def envoy_copts(repository, test = False):
    posix_options = [
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wnon-virtual-dtor",
        "-Woverloaded-virtual",
        "-Wold-style-cast",
        "-Wformat",
        "-Wformat-security",
        "-Wvla",
        "-Wno-deprecated-declarations",
        "-Wreturn-type",
    ]
    _repo = repo_label(repository)

    # Windows options for cleanest service compilation;
    #   General MSVC C++ options for Envoy current expectations.
    #   Target windows.h for all Windows 10 (0x0A) API prototypes (ntohll etc)
    #   (See https://msdn.microsoft.com/en-us/library/windows/desktop/aa383745(v=vs.85).aspx )
    #   Optimize Windows headers by dropping GUI-oriented features from compilation
    msvc_options = [
        "-WX",
        "-Zc:__cplusplus",
        "-DWIN32",
        "-D_WIN32_WINNT=0x0A00",  # _WIN32_WINNT_WIN10
        "-DNTDDI_VERSION=0x0A000005",  # NTDDI_WIN10_RS4
        "-DWIN32_LEAN_AND_MEAN",
        "-DNOUSER",
        "-DNOMCX",
        "-DNOIME",
        "-DNOCRYPT",
        # Ignore unguarded gcc pragmas in quiche (unrecognized by MSVC)
        "-wd4068",
        # Silence incorrect MSVC compiler warnings when converting between std::optional
        # data types (while conversions between primitive types are producing no error)
        "-wd4244",
        # Allow inline functions to be undefined
        "-wd4506",
    ]

    return select({
               _repo("//bazel:windows_x86_64"): msvc_options,
               "//conditions:default": posix_options,
           }) + select({
               # Simplify the amount of symbolic debug info for test binaries, since
               # debugging info detailing some 1600 test binaries would be wasteful.
               # targets listed in order from generic to increasing specificity.
               # Bazel adds an implicit -DNDEBUG for opt targets.
               _repo("//bazel:opt_build"): [] if test else ["-ggdb3"],
               _repo("//bazel:fastbuild_build"): [],
               _repo("//bazel:dbg_build"): ["-ggdb3"],
               _repo("//bazel:windows_opt_build"): [] if test else ["-Z7"],
               _repo("//bazel:windows_fastbuild_build"): [],
               _repo("//bazel:windows_dbg_build"): [],
               _repo("//bazel:clang_cl_opt_build"): [] if test else ["-Z7", "-fstandalone-debug"],
               _repo("//bazel:clang_cl_fastbuild_build"): ["-fno-standalone-debug"],
               _repo("//bazel:clang_cl_dbg_build"): ["-fstandalone-debug"],
           }) + select({
               # Toggle expected features and warnings by compiler
               _repo("//bazel:clang_build"): [
                   "-fno-limit-debug-info",
                   "-Wgnu-conditional-omitted-operand",
                   "-Wc++2a-extensions",
                   "-Wrange-loop-analysis",
               ],
               _repo("//bazel:gcc_build"): [
                   "-Wno-maybe-uninitialized",
                   # GCC implementation of this warning is too noisy.
                   #
                   # It generates warnings even in cases where there is no ambiguity
                   # between the overloaded version of a method and the hidden version
                   # from the base class. E.g., when the two have different number of
                   # arguments or incompatible types and therefore a wrong function
                   # cannot be called by mistake without triggering a compiler error.
                   #
                   # As a safeguard, this warning is only disabled for GCC builds, so
                   # if Clang catches a problem in the code we would get a warning
                   # anyways.
                   "-Wno-error=overloaded-virtual",
               ],
               # Allow 'nodiscard' function results values to be discarded for test code only
               # TODO(envoyproxy/windows-dev): Replace /Zc:preprocessor with /experimental:preprocessor
               # for msvc versions between 15.8 through 16.4.x. see
               # https://docs.microsoft.com/en-us/cpp/build/reference/zc-preprocessor
               _repo("//bazel:windows_x86_64"): ["-wd4834", "-Zc:preprocessor", "-Wv:19.4"] if test else ["-Zc:preprocessor", "-Wv:19.4"],
               _repo("//bazel:clang_cl_build"): ["-Wno-unused-result"] if test else [],
               "//conditions:default": [],
           }) + select({
               # TODO: Remove once https://reviews.llvm.org/D73007 is in the lowest supported Xcode version
               _repo("//bazel:apple"): ["-Wno-range-loop-analysis"],
               "//conditions:default": [],
           }) + select({
               _repo("//bazel:no_debug_info"): ["-g0"],
               "//conditions:default": [],
           }) + selects.with_or({
               _repo("//bazel:disable_tcmalloc"): ["-DABSL_MALLOC_HOOK_MMAP_DISABLE"],
               _repo("//bazel:debug_tcmalloc"): ["-DENVOY_MEMORY_DEBUG_ENABLED=1", "-DGPERFTOOLS_TCMALLOC"],
               _repo("//bazel:gperftools_tcmalloc"): ["-DGPERFTOOLS_TCMALLOC"],
               (
                   "@platforms//cpu:x86_64",
                   "@platforms//cpu:aarch64",
               ): ["-DTCMALLOC"],
               "//conditions:default": ["-DGPERFTOOLS_TCMALLOC"],
           }) + select({
               _repo("//bazel:disable_object_dump_on_signal_trace"): [],
               "//conditions:default": ["-DENVOY_OBJECT_TRACE_ON_DUMP"],
           }) + select({
               _repo("//bazel:disable_deprecated_features"): ["-DENVOY_DISABLE_DEPRECATED_FEATURES"],
               "//conditions:default": [],
           }) + select({
               _repo("//bazel:enable_log_debug_assert_in_release"): ["-DENVOY_LOG_DEBUG_ASSERT_IN_RELEASE"],
               "//conditions:default": [],
           }) + select({
               _repo("//bazel:enable_log_fast_debug_assert_in_release"): ["-DENVOY_LOG_FAST_DEBUG_ASSERT_IN_RELEASE"],
               "//conditions:default": [],
           }) + select({
               _repo("//bazel:disable_known_issue_asserts"): ["-DENVOY_DISABLE_KNOWN_ISSUE_ASSERTS"],
               "//conditions:default": [],
           }) + select({
               # APPLE_USE_RFC_3542 is needed to support IPV6_PKTINFO in MAC OS.
               _repo("//bazel:apple"): ["-D__APPLE_USE_RFC_3542"],
               "//conditions:default": [],
           }) + select({
               _repo("//bazel:uhv_enabled"): ["-DENVOY_ENABLE_UHV"],
               "//conditions:default": [],
           }) + envoy_select_hot_restart(["-DENVOY_HOT_RESTART"], repository) + \
           envoy_select_nghttp2(["-DENVOY_NGHTTP2"], repository) + \
           envoy_select_disable_exceptions(["-fno-exceptions"], repository) + \
           envoy_select_admin_html(["-DENVOY_ADMIN_HTML"], repository) + \
           envoy_select_static_extension_registration(["-DENVOY_STATIC_EXTENSION_REGISTRATION"], repository) + \
           envoy_select_disable_logging(["-DENVOY_DISABLE_LOGGING"], repository) + \
           _envoy_select_perf_annotation(["-DENVOY_PERF_ANNOTATION"]) + \
           _envoy_select_execution_context() + \
           _envoy_select_perfetto(["-DENVOY_PERFETTO"]) + \
           envoy_select_google_grpc(["-DENVOY_GOOGLE_GRPC"], repository) + \
           envoy_select_signal_trace(["-DENVOY_HANDLE_SIGNALS"], repository) + \
           _envoy_select_path_normalization_by_default(["-DENVOY_NORMALIZE_PATH_BY_DEFAULT"], repository)

# References to Envoy external dependencies should be wrapped with this function.
def envoy_external_dep_path(dep):
    return "//external:%s" % dep

def envoy_linkstatic():
    return select({
        "@envoy//bazel:dynamic_link_tests": 0,
        "//conditions:default": 1,
    })

def envoy_select_force_libcpp(if_libcpp, default = None):
    return select({
        "@envoy//bazel:force_libcpp": if_libcpp,
        "@envoy//bazel:apple": [],
        "@envoy//bazel:windows_x86_64": [],
        "//conditions:default": default or [],
    })

def envoy_stdlib_deps():
    return select({
        "//conditions:default": ["@envoy//bazel:static_stdlib"],
    })

def envoy_dbg_linkopts():
    return select({
        # TODO: Remove once we have https://github.com/bazelbuild/bazel/pull/15635
        "@envoy//bazel:apple_non_opt": ["-Wl,-no_deduplicate"],
        "//conditions:default": [],
    })

# Dependencies on tcmalloc_and_profiler should be wrapped with this function.
def tcmalloc_external_dep(repository):
    _repo = repo_label(repository)
    return selects.with_or({
        (_repo("//bazel:disable_tcmalloc")): None,
        (
            _repo("//bazel:debug_tcmalloc"),
            _repo("//bazel:gperftools_tcmalloc"),
        ): _repo("//bazel/foreign_cc:gperftools"),
        "//conditions:default": _repo("//bazel:tcmalloc_lib"),
    })

# Select the given values if default path normalization is on in the current build.
def _envoy_select_path_normalization_by_default(xs, repository = ""):
    return select({
        repository + "//bazel:enable_path_normalization_by_default": xs,
        "//conditions:default": [],
    })

def _envoy_select_perf_annotation(xs):
    return select({
        "@envoy//bazel:enable_perf_annotation": xs,
        "//conditions:default": [],
    })

def _envoy_select_execution_context():
    return select({
        "@envoy//bazel:enable_execution_context": ["-DENVOY_ENABLE_EXECUTION_CONTEXT"],
        "//conditions:default": [],
    })

def _envoy_select_perfetto(xs):
    return select({
        "@envoy//bazel:enable_perf_tracing": xs,
        "//conditions:default": [],
    })

def envoy_exported_symbols_input():
    return [
        "@envoy//bazel:exported_symbols.txt",
        "@envoy//bazel:exported_symbols_apple.txt",
    ]

# Default symbols to be exported.
def _envoy_default_exported_symbols():
    return select({
        "@envoy//bazel:linux": [
            "-Wl,--dynamic-list=$(location @envoy//bazel:exported_symbols.txt)",
        ],
        "@envoy//bazel:apple": [
            "-Wl,-exported_symbols_list,$(location @envoy//bazel:exported_symbols_apple.txt)",
        ],
        "//conditions:default": [],
    })

# Select the given values if exporting is enabled in the current build.
def envoy_select_exported_symbols(xs):
    return select({
        "@envoy//bazel:enable_exported_symbols": xs,
        "//conditions:default": [],
    }) + _envoy_default_exported_symbols()

def repo_label(repository):
    def _repo_label(label):
        return "%s%s" % (repository, label)

    return _repo_label
