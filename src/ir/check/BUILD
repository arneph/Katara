load("@rules_cc//cc:defs.bzl", "cc_library")
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
        "//src/lang/processors/ir/check:__pkg__",
    ],
    deps = [
        "//src/common/positions",
        "//src/ir/issues",
        "//src/ir/representation",
        "//src/ir/serialization:positions",
        "//src/ir/serialization:positions_util",
    ],
)

cc_library(
    name = "check",
    hdrs = [
        "check.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
    deps = [
        ":checker",
    ],
)

cc_test(
    name = "check_test",
    srcs = ["check_test.cc"],
    copts = COPTS,
    deps = [
        ":check",
        "//src/common/atomics",
        "//src/ir/issues",
        "//src/ir/representation",
        "//src/ir/serialization",
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
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
    deps = [
        ":check",
        "//src/common/logging",
        "//src/common/positions",
        "//src/ir/issues",
        "//src/ir/representation",
        "//src/ir/serialization",
    ],
)
