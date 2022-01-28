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
        ":logging",
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

cc_library(
    name = "memory",
    srcs = ["memory.cc"],
    hdrs = ["memory.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":data_view",
        ":logging",
    ],
)

cc_test(
    name = "memory_test",
    srcs = ["memory_test.cc"],
    copts = COPTS,
    deps = [
        ":memory",
        "@gtest//:gtest_main",
    ],
)
