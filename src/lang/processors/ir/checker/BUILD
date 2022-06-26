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
        "//src/lang/processors/ir:__subpackages__",
    ],
    deps = [
        "//src/common/logging",
        "//src/ir/checker",
        "//src/ir/representation",
        "//src/lang/representation",
    ],
)

cc_test(
    name = "checker_test",
    srcs = ["checker_test.cc"],
    copts = COPTS,
    deps = [
        ":checker",
        "//src/ir/representation",
        "//src/lang/representation",
        "@gtest//:gtest_main",
    ],
)
