load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "parser",
    srcs = ["parser.cc"],
    hdrs = ["parser.h"],
    copts = COPTS,
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
    deps = [
        "//src/lang/processors/issues",
        "//src/lang/processors/scanner",
        "//src/lang/representation",
    ],
)

cc_test(
    name = "parser_test",
    srcs = ["parser_test.cc"],
    copts = COPTS,
    deps = [
        ":parser",
        "//src/lang/processors/issues",
        "//src/lang/representation",
        "@gtest//:gtest_main",
    ],
)
