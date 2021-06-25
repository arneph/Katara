load("@rules_cc//cc:defs.bzl", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_test(
    name = "integration_test",
    srcs = ["integration_test.cc"],
    copts = COPTS,
    deps = [
        "//src/ir/representation",
        "//src/lang:lang_lib",
        "//src/lang/processors/packages:mock_loader",
        "@gtest//:gtest_main",
    ],
)
