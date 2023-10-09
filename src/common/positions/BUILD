load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "positions",
    srcs = ["positions.cc"],
    hdrs = ["positions.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = ["//src/common/logging"],
)

cc_test(
    name = "positions_test",
    srcs = ["positions_test.cc"],
    copts = COPTS,
    deps = [
        ":positions",
        "@gtest//:gtest_main",
    ],
)
