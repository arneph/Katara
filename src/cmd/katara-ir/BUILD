load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "error_codes",
    hdrs = ["error_codes.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
)

cc_library(
    name = "parse",
    srcs = ["parse.cc"],
    hdrs = ["parse.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":error_codes",
        "//src/cmd:context",
        "//src/ir:ir_lib",
    ],
)

cc_library(
    name = "check",
    srcs = ["check.cc"],
    hdrs = ["check.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":error_codes",
        ":parse",
        "//src/cmd:context",
        "//src/ir:ir_lib",
    ],
)

cc_library(
    name = "debug",
    srcs = ["debug.cc"],
    hdrs = ["debug.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":check",
        ":error_codes",
        ":interpret",
        "//src/cmd:context",
        "//src/cmd:repl",
        "//src/ir:ir_lib",
    ],
)

cc_library(
    name = "format",
    srcs = ["format.cc"],
    hdrs = ["format.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":check",
        ":error_codes",
        "//src/cmd:context",
        "//src/ir:ir_lib",
    ],
)

cc_library(
    name = "interpret",
    srcs = ["interpret.cc"],
    hdrs = ["interpret.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":check",
        ":error_codes",
        "//src/cmd:context",
        "//src/ir:ir_lib",
    ],
)

cc_library(
    name = "cmd",
    srcs = ["cmd.cc"],
    hdrs = ["cmd.h"],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
    ],
    deps = [
        ":check",
        ":debug",
        ":format",
        ":interpret",
        "//src/cmd:context",
        "//src/cmd:version",
        "//src/common/flags",
    ],
)

cc_binary(
    name = "katara-ir",
    srcs = ["katara-ir.cc"],
    copts = COPTS,
    deps = [
        ":cmd",
        "//src/cmd:context",
        "//src/cmd:real_context",
        "//src/cmd:util",
    ],
)
