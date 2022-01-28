load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "ops",
    srcs = [
        "ops.cc",
    ],
    hdrs = [
        "ops.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64:__subpackages__",
    ],
    deps = [
        "//src/common:logging",
    ],
)

cc_test(
    name = "ops_test",
    srcs = ["ops_test.cc"],
    copts = COPTS,
    deps = [
        ":x86_64_lib",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "x86_64_lib",
    srcs = [
        "block.cc",
        "func.cc",
        "program.cc",
    ],
    hdrs = [
        "block.h",
        "func.h",
        "program.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/common:graph",
        "//src/common/data_view",
        "//src/x86_64/instrs",
        "//src/x86_64/machine_code:linker",
    ],
)
