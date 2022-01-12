load("@rules_python//python:defs.bzl", "py_binary", "py_library")
load("@base_pip3//:requirements.bzl", base_entry_point = "entry_point")

def envoy_py_test(name, package, visibility, envoy_prefix = "@envoy"):
    filepath = "$(location %s//tools/testing:base_pytest_runner.py)" % envoy_prefix
    output = "$(@D)/pytest_%s.py" % name

    native.genrule(
        name = "generate_pytest_" + name,
        cmd = "sed s/_PACKAGE_NAME_/%s/ %s > \"%s\"" % (package, filepath, output),
        tools = ["%s//tools/testing:base_pytest_runner.py" % envoy_prefix],
        outs = ["pytest_%s.py" % name],
    )

    test_deps = [
        ":%s" % name,
    ]

    if name != "python_pytest":
        test_deps.append("%s//tools/testing:python_pytest" % envoy_prefix)

    py_binary(
        name = "pytest_%s" % name,
        srcs = [
            "pytest_%s.py" % name,
            "tests/test_%s.py" % name,
        ],
        data = [":generate_pytest_%s" % name],
        deps = test_deps,
        visibility = visibility,
    )

def envoy_py_library(
        name = None,
        deps = [],
        data = [],
        visibility = ["//visibility:public"],
        envoy_prefix = "",
        test = True):
    _parts = name.split(".")
    package = ".".join(_parts[:-1])
    name = _parts[-1]

    py_library(
        name = name,
        srcs = ["%s.py" % name],
        deps = deps,
        data = data,
        visibility = visibility,
    )
    if test:
        envoy_py_test(name, package, visibility, envoy_prefix = envoy_prefix)

def envoy_py_binary(
        name = None,
        deps = [],
        data = [],
        visibility = ["//visibility:public"],
        envoy_prefix = "@envoy",
        test = True):
    _parts = name.split(".")
    package = ".".join(_parts[:-1])
    name = _parts[-1]

    py_binary(
        name = name,
        srcs = ["%s.py" % name],
        deps = deps,
        data = data,
        visibility = visibility,
    )

    if test:
        envoy_py_test(name, package, visibility, envoy_prefix = envoy_prefix)

def envoy_entry_point(
        name,
        pkg,
        main = "//tools/base:entry_point.py",
        entry_point = base_entry_point,
        script = None,
        data = None,
        args = None):
    """This macro provides the convenience of using an `entry_point` while
    also being able to create a rule with associated `args` and `data`, as is
    possible with the normal `py_binary` rule.

    We may wish to remove this macro should https://github.com/bazelbuild/rules_python/issues/600
    be resolved.

    The `script` and `pkg` args are passed directly to the `entry_point`.

    By default, the pip `entry_point` from `@base_pip3` is used. You can provide
    a custom `entry_point` if eg you want to provide an `entry_point` with dev
    requirements, or from some other requirements set.

    A `py_binary` is dynamically created to wrap the `entry_point` with provided
    `args` and `data`.
    """
    script = script or pkg
    entry_point_name = "%s_entry_point" % name
    native.alias(
        name = entry_point_name,
        actual = entry_point(
            pkg = pkg,
            script = script,
        ),
    )
    py_binary(
        name = name,
        srcs = [main],
        main = main,
        args = ["$(location %s)" % entry_point_name] + args,
        data = [entry_point_name] + data,
    )
