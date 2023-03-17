# List of modules to compile into the test files. For JVM we always compile these into the .so all
# the time, while for iOS we have to force register the extensions, so test files must pull in
# the TestExtensions module and call register_test_extensions().
TEST_EXTENSIONS = [
    "//library/common/extensions/filters/http/test_accessor:config",
    "//library/common/extensions/filters/http/test_event_tracker:config",
    "//library/common/extensions/filters/http/test_kv_store:config",
    "//library/common/extensions/filters/http/test_logger:config",
    "//library/common/extensions/filters/http/test_read:config",
    "//library/common/extensions/filters/http/test_remote_response:config",
    "@envoy//source/extensions/clusters/static:static_cluster_lib",
]
