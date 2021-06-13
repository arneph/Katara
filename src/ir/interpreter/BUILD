load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "values",
    srcs = ["values.cc"],
    hdrs = ["values.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/ir/representation",
    ],
)

cc_library(
    name = "interpreter",
    srcs = ["interpreter.cc"],
    hdrs = ["interpreter.h"],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        ":values",
        "//src/ir/representation",
    ],
)
