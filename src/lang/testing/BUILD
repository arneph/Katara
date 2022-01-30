load("@rules_cc//cc:defs.bzl", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_test(
    name = "integration_test",
    srcs = ["integration_test.cc"],
    copts = COPTS,
    deps = [
        "//src/common/filesystem",
        "//src/common/filesystem:test_filesystem",
        "//src/ir/representation",
        "//src/lang:lang_lib",
        "@gtest//:gtest_main",
    ],
)
