load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "func_parser",
    srcs = [
        "func_parser.cc",
    ],
    hdrs = [
        "func_parser.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/lang/processors/ir:__subpackages__",
    ],
    deps = [
        "//src/common/logging",
        "//src/ir/representation",
        "//src/ir/serialization:serialization_specialization",
        "//src/lang/representation",
    ],
)
