load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "ir_extension",
    srcs = [
        "instrs.cc",
        "types.cc",
        "values.cc",
    ],
    hdrs = [
        "instrs.h",
        "types.h",
        "values.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
    deps = [
        "//src/ir/representation:instrs",
        "//src/ir/representation:types",
        "//src/ir/representation:values",
        "//src/lang/representation/constants",
    ],
)
