load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
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
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/common:logging",
    ],
)

cc_library(
    name = "parser",
    srcs = [
        "parser.cc",
    ],
    hdrs = [
        "parser.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        ":scanner",
        "//src/common:atomics",
        "//src/common:logging",
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
    name = "phi_resolver",
    srcs = [
        "phi_resolver.cc",
    ],
    hdrs = [
        "phi_resolver.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/ir/representation",
    ],
)

cc_test(
    name = "phi_resolver_test",
    srcs = ["phi_resolver_test.cc"],
    copts = COPTS,
    deps = [
        ":phi_resolver",
        "//src/common:atomics",
        "//src/ir/representation",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "processors",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":interference_graph_builder",
        ":interference_graph_colorer",
        ":live_range_analyzer",
        ":parser",
        ":phi_resolver",
        ":scanner",
    ],
)
