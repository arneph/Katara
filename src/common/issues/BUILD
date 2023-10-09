load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "issues",
    srcs = ["issues.cc"],
    hdrs = ["issues.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/common/positions",
    ],
)

cc_test(
    name = "issues_test",
    srcs = ["issues_test.cc"],
    copts = COPTS,
    deps = [
        ":issues",
        "//src/common/positions",
        "@gtest//:gtest_main",
    ],
)
