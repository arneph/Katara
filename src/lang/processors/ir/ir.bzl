load("@rules_cc//cc:defs.bzl", "cc_test")
load("//src:katara.bzl", "COPTS")

def ir_ext_file_check(name, src, visibility = None):
    cc_test(
        name = name,
        srcs = ["//src/lang/processors/ir:ir_ext_file_check.cc"],
        copts = COPTS,
        data = [src],
        deps = [
            "//src/ir:ir_lib",
            "//src/lang/processors/ir",
            "//src/lang/processors/ir/check:check_test_util",
            "@gtest//:gtest_main",
        ],
    )
