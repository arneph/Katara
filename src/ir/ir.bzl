load("@rules_cc//cc:defs.bzl", "cc_test")
load("//src:katara.bzl", "COPTS")

def ir_file_check(name, src, visibility = None):
    cc_test(
        name = name,
        srcs = ["//src/ir:ir_file_check.cc"],
        copts = COPTS,
        data = [src],
        deps = [
            "//src/ir:ir_lib",
            "@gtest//:gtest_main",
        ],
    )
