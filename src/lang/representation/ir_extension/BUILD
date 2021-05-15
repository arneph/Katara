load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "ir_extension",
    srcs = [
        "values.cc",
        "instrs.cc",
        "types.cc",
    ],
    hdrs = [
        "values.h",
        "instrs.h",
        "types.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
    deps = [
        "//src/ir/representation:instrs",
        "//src/ir/representation:values",
        "//src/ir/representation:types",
        "//src/lang/representation/constants",
    ],
)
