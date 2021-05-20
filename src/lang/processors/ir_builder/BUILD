load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "context",
    srcs = ["context.cc"],
    hdrs = ["context.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/ir:ir_lib",
        "//src/lang/representation",
    ],
)

cc_library(
    name = "ir_builder",
    srcs = ["ir_builder.cc"],
    hdrs = ["ir_builder.h"],
    copts = COPTS,
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
    deps = [
        ":context",
        "//src/ir:ir_lib",
        "//src/lang/processors/packages",
        "//src/lang/representation",
    ],
)
