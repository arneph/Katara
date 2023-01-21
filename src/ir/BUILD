load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

exports_files(["ir_file_check.cc"])

cc_library(
    name = "ir_lib",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/ir/analyzers",
        "//src/ir/builder",
        "//src/ir/check",
        "//src/ir/info",
        "//src/ir/interpreter",
        "//src/ir/interpreter:debugger",
        "//src/ir/issues",
        "//src/ir/optimizers",
        "//src/ir/processors",
        "//src/ir/representation",
        "//src/ir/serialization",
    ],
)
