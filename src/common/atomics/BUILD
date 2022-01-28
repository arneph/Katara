load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "atomics",
    srcs = ["atomics.cc"],
    hdrs = ["atomics.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
)

cc_test(
    name = "atomics_test",
    srcs = ["atomics_test.cc"],
    copts = COPTS,
    deps = [
        ":atomics",
        "@gtest//:gtest_main",
    ],
)
