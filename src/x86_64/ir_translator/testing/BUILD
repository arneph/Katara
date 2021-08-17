load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "instr_translator_test_setup",
    testonly = 1,
    hdrs = [
        "instr_translator_test_setup.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64/ir_translator:__subpackages__",
    ],
    deps = [
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
        "//src/x86_64/ir_translator:context",
        "@gtest//:gtest_main",
    ],
)
