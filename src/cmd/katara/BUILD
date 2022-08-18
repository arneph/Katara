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
    name = "debug",
    srcs = ["debug.cc"],
    hdrs = ["debug.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/cmd:context",
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
        ":debug",
        ":error_codes",
        "//src/cmd:context",
        "//src/common/graph",
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
        ":debug",
        ":error_codes",
        ":load",
        "//src/cmd:context",
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
        ":debug",
        ":error_codes",
        ":load",
        "//src/cmd:context",
        "//src/ir:ir_lib",
        "//src/lang:lang_lib",
        "//src/x86_64:x86_64_lib",
        "//src/x86_64/ir_translator",
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
        ":build",
        ":debug",
        ":error_codes",
        "//src/cmd:context",
        "//src/ir:ir_lib",
    ],
)

cc_test(
    name = "interpret_test",
    srcs = ["interpret_test.cc"],
    copts = COPTS,
    deps = [
        ":interpret",
        "//src/cmd:context",
        "//src/cmd:test_context",
        "@gtest//:gtest_main",
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
        ":debug",
        ":error_codes",
        "//src/cmd:context",
        "//src/common/data_view",
        "//src/common/memory",
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
    ],
)

cc_test(
    name = "run_test",
    srcs = ["run_test.cc"],
    copts = COPTS,
    deps = [
        ":run",
        "//src/cmd:context",
        "//src/cmd:test_context",
        "@gtest//:gtest_main",
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
        ":debug",
        ":doc",
        ":error_codes",
        ":interpret",
        ":run",
        "//src/cmd:context",
        "//src/common/flags",
    ],
)

cc_binary(
    name = "katara",
    srcs = ["katara.cc"],
    copts = COPTS,
    deps = [
        ":cmd",
        "//src/cmd:context",
        "//src/cmd:real_context",
        "//src/cmd:util",
    ],
)
