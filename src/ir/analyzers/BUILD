load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

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
        ":interference_graph_builder",
        ":interference_graph_colorer",
        ":live_range_analyzer",
    ],
)
