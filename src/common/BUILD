load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_test")
load("//src:katara.bzl", "COPTS")

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
        ":logging",
        "//src/common/data_view",
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
