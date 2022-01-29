load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "filesystem",
    srcs = ["filesystem.cc"],
    hdrs = ["filesystem.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
)

cc_library(
    name = "real_filesystem",
    srcs = ["real_filesystem.cc"],
    hdrs = ["real_filesystem.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":filesystem",
        "//src/common/logging",
    ],
)

cc_library(
    name = "test_filesystem",
    srcs = ["test_filesystem.cc"],
    hdrs = ["test_filesystem.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":filesystem",
        "//src/common/logging",
    ],
)

cc_test(
    name = "test_filesystem_test",
    srcs = ["test_filesystem_test.cc"],
    copts = COPTS,
    deps = [
        ":filesystem",
        ":test_filesystem",
        "@gtest//:gtest_main",
    ],
)
