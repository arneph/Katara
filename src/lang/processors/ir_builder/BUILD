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
    name = "type_builder",
    srcs = ["type_builder.cc"],
    hdrs = ["type_builder.h"],
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
    name = "expr_builder",
    srcs = ["expr_builder.cc"],
    hdrs = ["expr_builder.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":context",
        ":type_builder",
        "//src/ir:ir_lib",
        "//src/lang/representation",
    ],
)

cc_library(
    name = "stmt_builder",
    srcs = ["stmt_builder.cc"],
    hdrs = ["stmt_builder.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":context",
        ":expr_builder",
        ":type_builder",
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
        ":expr_builder",
        ":stmt_builder",
        ":type_builder",
        "//src/ir:ir_lib",
        "//src/lang/processors/packages:package",
        "//src/lang/processors/packages:package_manager",
        "//src/lang/representation",
    ],
)
