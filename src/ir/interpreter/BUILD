load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "execution_point",
    srcs = ["execution_point.cc"],
    hdrs = ["execution_point.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/common/logging",
        "//src/ir/representation",
    ],
)

cc_test(
    name = "execution_point_test",
    srcs = ["execution_point_test.cc"],
    copts = COPTS,
    deps = [
        ":execution_point",
        "//src/ir/checker",
        "//src/ir/representation",
        "//src/ir/serialization:parse",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "stack",
    srcs = ["stack.cc"],
    hdrs = ["stack.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":execution_point",
        "//src/ir/representation",
    ],
)

cc_test(
    name = "stack_test",
    srcs = ["stack_test.cc"],
    copts = COPTS,
    deps = [
        ":stack",
        "//src/ir/checker",
        "//src/ir/representation",
        "//src/ir/serialization:parse",
        "@gtest//:gtest_main",
    ],
)

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
        ":execution_point",
        ":heap",
        ":stack",
        "//src/common/atomics",
        "//src/ir/representation",
    ],
)

cc_test(
    name = "interpreter_test",
    srcs = ["interpreter_test.cc"],
    copts = COPTS,
    deps = [
        ":interpreter",
        "//src/ir/checker",
        "//src/ir/representation",
        "//src/ir/serialization:parse",
        "@gtest//:gtest_main",
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
