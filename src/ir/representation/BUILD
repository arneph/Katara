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
        ":values",
        "//src/common:logging",
        "//src/common/atomics",
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
        "//src/common:graph",
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
        "//src/common:graph",
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
        ":program",
        ":types",
        ":values",
    ],
)
