load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "num_types",
    hdrs = [
        "num_types.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
)

cc_library(
    name = "object",
    srcs = [
        "object.cc",
    ],
    hdrs = [
        "object.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
)

cc_library(
    name = "types",
    srcs = [
        "types.cc",
    ],
    hdrs = [
        "types.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
    deps = [
        ":object",
        "//src/common/atomics",
    ],
)

cc_library(
    name = "values",
    srcs = [
        "values.cc",
    ],
    hdrs = [
        "values.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
    deps = [
        ":num_types",
        ":object",
        ":types",
        "//src/common/atomics",
    ],
)

cc_library(
    name = "instrs",
    srcs = [
        "instrs.cc",
    ],
    hdrs = [
        "instrs.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
    deps = [
        ":num_types",
        ":object",
        ":values",
        "//src/common/atomics",
        "//src/common/logging",
    ],
)

cc_library(
    name = "block",
    srcs = [
        "block.cc",
    ],
    hdrs = [
        "block.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
    deps = [
        ":instrs",
        ":num_types",
        ":object",
        "//src/common/graph",
    ],
)

cc_library(
    name = "func",
    srcs = [
        "func.cc",
    ],
    hdrs = [
        "func.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
    deps = [
        ":block",
        ":num_types",
        ":object",
        "//src/common/graph",
    ],
)

cc_test(
    name = "func_test",
    srcs = ["func_test.cc"],
    copts = COPTS,
    deps = [
        ":block",
        ":func",
        ":num_types",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "program",
    srcs = [
        "program.cc",
    ],
    hdrs = [
        "program.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
    deps = [
        ":func",
        ":object",
    ],
)

cc_library(
    name = "representation",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":block",
        ":func",
        ":instrs",
        ":num_types",
        ":object",
        ":program",
        ":types",
        ":values",
    ],
)
