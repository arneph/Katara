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
        "//src/ir/check:check_test_util",
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
        "//src/ir/check:check_test_util",
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
        "//src/ir/check:check_test_util",
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
        "//src/lang/processors/ir/check:check_test_util",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "debugger",
    srcs = ["debugger.cc"],
    hdrs = ["debugger.h"],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        ":execution_point",
        ":heap",
        ":interpreter",
        ":stack",
        "//src/ir/representation",
    ],
)

cc_test(
    name = "debugger_test",
    srcs = ["debugger_test.cc"],
    copts = COPTS,
    deps = [
        ":debugger",
        "//src/ir/check:check_test_util",
        "//src/ir/representation",
        "//src/ir/serialization:parse",
        "@gtest//:gtest_main",
    ],
)
