load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "ir_lib",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/ir/info",
        "//src/ir/interpreter",
        "//src/ir/processors",
        "//src/ir/representation",
    ],
)
