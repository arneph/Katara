load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "checker",
    srcs = [
        "checker.cc",
    ],
    hdrs = [
        "checker.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/common/logging",
        "//src/ir/check",
        "//src/ir/representation",
        "//src/ir/serialization",
        "//src/lang/processors/ir/serialization:positions_util",
        "//src/lang/representation",
    ],
)

cc_library(
    name = "check",
    hdrs = [
        "check.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/lang/processors/ir:__subpackages__",
        "//src/lang/runtime:__subpackages__",
    ],
    deps = [
        ":checker",
        "//src/ir/check",
        "//src/ir/representation",
        "//src/ir/serialization",
    ],
)

cc_test(
    name = "check_test",
    srcs = ["check_test.cc"],
    copts = COPTS,
    deps = [
        ":check",
        "//src/ir/representation",
        "//src/lang/representation",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "check_test_util",
    testonly = 1,
    hdrs = [
        "check_test_util.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir/interpreter:__subpackages__",
        "//src/lang/processors/ir:__subpackages__",
        "//src/lang/runtime:__subpackages__",
    ],
    deps = [
        ":checker",
        "//src/ir/check:check_test_util",
        "//src/ir/representation",
    ],
)
