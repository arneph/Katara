load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "func_call_graph_optimizer",
    srcs = [
        "func_call_graph_optimizer.cc",
    ],
    hdrs = [
        "func_call_graph_optimizer.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/ir/analyzers",
        "//src/ir/info",
        "//src/ir/representation",
    ],
)

cc_library(
    name = "optimizers",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":func_call_graph_optimizer",
    ],
)
