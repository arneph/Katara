load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "values",
    hdrs = [
        "values.h",
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
    name = "func_call_graph",
    srcs = [
        "func_call_graph.cc",
    ],
    hdrs = [
        "func_call_graph.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/common:graph",
        "//src/ir/representation",
    ],
)

cc_library(
    name = "interference_graph",
    srcs = [
        "interference_graph.cc",
    ],
    hdrs = [
        "interference_graph.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/common:graph",
        "//src/ir/representation",
    ],
)

cc_library(
    name = "live_ranges",
    srcs = [
        "block_live_ranges.cc",
        "func_live_ranges.cc",
    ],
    hdrs = [
        "block_live_ranges.h",
        "func_live_ranges.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        ":interference_graph",
        "//src/ir/representation",
    ],
)

cc_library(
    name = "info",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":func_call_graph",
        ":interference_graph",
        ":live_ranges",
        ":values",
    ],
)
