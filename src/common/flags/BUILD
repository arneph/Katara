load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "flag_values",
    hdrs = ["flag_values.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
)

cc_library(
    name = "flags",
    srcs = ["flags.cc"],
    hdrs = ["flags.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":flag_values",
    ],
)

cc_test(
    name = "flags_test",
    srcs = ["flags_test.cc"],
    copts = COPTS,
    deps = [
        ":flag_values",
        ":flags",
        "@gtest//:gtest_main",
    ],
)
