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
    name = "context",
    srcs = ["context.cc"],
    hdrs = ["context.h"],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/cmd:__pkg__",
    ],
)

cc_library(
    name = "util",
    srcs = ["util.cc"],
    hdrs = ["util.h"],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/cmd:__pkg__",
    ],
)

cc_library(
    name = "load",
    srcs = ["load.cc"],
    hdrs = ["load.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":context",
        ":error_codes",
        "//src/common:graph",
        "//src/lang:lang_lib",
    ],
)

cc_library(
    name = "doc",
    srcs = ["doc.cc"],
    hdrs = ["doc.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":context",
        ":error_codes",
        ":load",
    ],
)

cc_library(
    name = "build",
    srcs = ["build.cc"],
    hdrs = ["build.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":context",
        ":error_codes",
        ":load",
        "//src/ir:ir_lib",
        "//src/lang:lang_lib",
        "//src/x86_64:x86_64_lib",
        "//src/x86_64/ir_translator",
    ],
)

cc_library(
    name = "run",
    srcs = ["run.cc"],
    hdrs = ["run.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":build",
        ":context",
        ":error_codes",
        "//src/common:data_view",
        "//src/common:memory",
        "//src/x86_64:x86_64_lib",
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
        ":build",
        ":context",
        ":doc",
        ":error_codes",
        ":run",
    ],
)