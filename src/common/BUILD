load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "data",
    srcs = ["data.cc"],
    hdrs = ["data.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
)

cc_library(
    name = "graph",
    srcs = [
        "edge.cc",
        "graph.cc",
        "node.cc",
    ],
    hdrs = [
        "edge.h",
        "graph.h",
        "node.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
)
