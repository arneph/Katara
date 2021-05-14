load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "ir_extension",
    srcs = [
        "values.cc",
    ],
    hdrs = [
        "values.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
    deps = [
        "//src/ir/representation:types",
        "//src/lang/representation/constants",
    ],
)
