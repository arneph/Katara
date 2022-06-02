load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "func_call_graph_builder",
    srcs = [
        "func_call_graph_builder.cc",
    ],
    hdrs = [
        "func_call_graph_builder.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/ir/info",
        "//src/ir/representation",
    ],
)

cc_library(
    name = "live_range_analyzer",
    srcs = [
        "live_range_analyzer.cc",
    ],
    hdrs = [
        "live_range_analyzer.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/ir/info",
        "//src/ir/representation",
    ],
)

cc_library(
    name = "func_values_builder",
    srcs = [
        "func_values_builder.cc",
    ],
    hdrs = [
        "func_values_builder.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/ir/info",
        "//src/ir/representation",
    ],
)

cc_library(
    name = "interference_graph_builder",
    srcs = [
        "interference_graph_builder.cc",
    ],
    hdrs = [
        "interference_graph_builder.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/ir/info",
        "//src/ir/representation",
    ],
)

cc_library(
    name = "interference_graph_colorer",
    srcs = [
        "interference_graph_colorer.cc",
    ],
    hdrs = [
        "interference_graph_colorer.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/ir/info",
        "//src/ir/representation",
    ],
)

cc_library(
    name = "analyzers",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":func_call_graph_builder",
        ":func_values_builder",
        ":interference_graph_builder",
        ":interference_graph_colorer",
        ":live_range_analyzer",
    ],
)
