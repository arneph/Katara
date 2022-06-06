load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "type_parser",
    srcs = [
        "type_parser.cc",
    ],
    hdrs = [
        "type_parser.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/common/logging",
        "//src/ir/representation",
        "//src/ir/serialization:serialization_specialization",
        "//src/lang/representation",
    ],
)

cc_library(
    name = "constant_parser",
    srcs = [
        "constant_parser.cc",
    ],
    hdrs = [
        "constant_parser.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/common/logging",
        "//src/ir/representation",
        "//src/ir/serialization:serialization_specialization",
        "//src/lang/representation",
    ],
)

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
        "//visibility:private",
    ],
    deps = [
        "//src/common/logging",
        "//src/ir/representation",
        "//src/ir/serialization:serialization_specialization",
        "//src/lang/representation",
    ],
)

cc_library(
    name = "parse",
    hdrs = [
        "parse.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/lang/processors/ir:__subpackages__",
    ],
    deps = [
        ":constant_parser",
        ":func_parser",
        ":type_parser",
        "//src/ir/representation",
        "//src/ir/serialization",
        "//src/lang/representation",
    ],
)
