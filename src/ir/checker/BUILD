load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "issues",
    srcs = [
        "issues.cc",
    ],
    hdrs = [
        "issues.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
    deps = [
        "//src/ir/representation",
    ],
)

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
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
    deps = [
        ":issues",
        "//src/ir/representation",
    ],
)

cc_test(
    name = "checker_test",
    srcs = ["checker_test.cc"],
    copts = COPTS,
    deps = [
        ":checker",
        ":issues",
        "//src/common/atomics",
        "//src/ir/representation",
        "@gtest//:gtest_main",
    ],
)
