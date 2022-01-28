load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "data_view",
    srcs = ["data_view.cc"],
    hdrs = ["data_view.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/common/logging",
    ],
)

cc_test(
    name = "data_view_test",
    srcs = ["data_view_test.cc"],
    copts = COPTS,
    deps = [
        ":data_view",
        "@gtest//:gtest_main",
    ],
)
