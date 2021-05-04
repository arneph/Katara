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
    ],
)

cc_library(
    name = "instr",
    srcs = [
        "instr.cc",
    ],
    hdrs = [
        "instr.h",
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
        "//src/vcg:vcg_lib",
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
        ":instr",
        ":num_types",
        "//src/vcg:vcg_lib",
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
        "//src/vcg:vcg_lib",
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
        "//src/vcg:vcg_lib",
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
        ":instr",
        ":num_types",
        ":program",
        ":types",
        ":values",
    ],
)
