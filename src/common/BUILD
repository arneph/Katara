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

cc_library(
    name = "data",
    srcs = ["data.cc"],
    hdrs = ["data.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":logging",
    ],
)

cc_library(
    name = "graph",
    srcs = ["graph.cc"],
    hdrs = ["graph.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
)

cc_library(
    name = "logging",
    srcs = ["logging.cc"],
    hdrs = ["logging.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
)
