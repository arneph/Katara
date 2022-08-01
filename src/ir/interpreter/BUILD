load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "heap",
    srcs = ["heap.cc"],
    hdrs = ["heap.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/common/logging",
    ],
)

cc_test(
    name = "heap_test",
    srcs = ["heap_test.cc"],
    copts = COPTS,
    deps = [
        ":heap",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "interpreter",
    srcs = ["interpreter.cc"],
    hdrs = ["interpreter.h"],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        ":heap",
        "//src/ir/representation",
    ],
)

cc_test(
    name = "interpreter_integration_test",
    srcs = ["interpreter_integration_test.cc"],
    copts = COPTS,
    deps = [
        ":interpreter",
        "//src/common/filesystem",
        "//src/common/filesystem:test_filesystem",
        "//src/lang:lang_lib",
        "@gtest//:gtest_main",
    ],
)
