load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "scanner",
    srcs = [
        "scanner.cc",
    ],
    hdrs = [
        "scanner.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/common/atomics",
        "//src/common/logging",
        "//src/common/positions",
        "//src/ir/issues",
    ],
)

cc_test(
    name = "scanner_test",
    srcs = ["scanner_test.cc"],
    copts = COPTS,
    deps = [
        ":scanner",
        "@gtest//:gtest_main",
    ],
)

cc_fuzz_test(
    name = "scanner_fuzz_test",
    srcs = ["scanner_fuzz_test.cc"],
    copts = COPTS,
    deps = [
        ":scanner",
        "//src/common/positions",
        "//src/ir/issues",
    ],
)

cc_library(
    name = "type_parser",
    srcs = [
        "type_parser.cc",
    ],
    hdrs = [
        "type_parser.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":scanner",
        "//src/common/logging",
        "//src/ir/representation",
    ],
)

cc_library(
    name = "constant_parser",
    srcs = [
        "constant_parser.cc",
    ],
    hdrs = [
        "constant_parser.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":scanner",
        ":type_parser",
        "//src/common/atomics",
        "//src/common/logging",
        "//src/ir/representation",
    ],
)

cc_library(
    name = "func_parser",
    srcs = [
        "func_parser.cc",
    ],
    hdrs = [
        "func_parser.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":constant_parser",
        ":scanner",
        ":type_parser",
        "//src/common/atomics",
        "//src/common/logging",
        "//src/ir/representation",
    ],
)

cc_library(
    name = "parse",
    hdrs = [
        "parse.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        ":func_parser",
        ":scanner",
    ],
)

cc_test(
    name = "parse_test",
    srcs = ["parse_test.cc"],
    copts = COPTS,
    deps = [
        ":parse",
        "//src/ir/checker",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "print",
    srcs = [
        "print.cc",
    ],
    hdrs = [
        "print.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/ir/representation",
    ],
)

cc_library(
    name = "serialization_specialization",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":func_parser",
        ":scanner",
    ],
)

cc_library(
    name = "serialization",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":parse",
        ":print",
    ],
)
