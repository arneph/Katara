load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "shared_to_unique_pointer_optimizer",
    srcs = ["shared_to_unique_pointer_optimizer.cc"],
    hdrs = ["shared_to_unique_pointer_optimizer.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/ir:ir_lib",
        "//src/lang/representation",
    ],
)

cc_test(
    name = "shared_to_unique_pointer_optimizer_test",
    srcs = ["shared_to_unique_pointer_optimizer_test.cc"],
    copts = COPTS,
    deps = [
        ":shared_to_unique_pointer_optimizer",
        "//src/ir/representation",
        "//src/ir/serialization",
        "//src/lang/processors",
        "//src/lang/processors/ir/check:check_test_util",
        "//src/lang/representation",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "unique_pointer_to_local_value_optimizer",
    srcs = ["unique_pointer_to_local_value_optimizer.cc"],
    hdrs = ["unique_pointer_to_local_value_optimizer.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/ir:ir_lib",
        "//src/lang/representation",
    ],
)

cc_test(
    name = "unique_pointer_to_local_value_optimizer_test",
    srcs = ["unique_pointer_to_local_value_optimizer_test.cc"],
    copts = COPTS,
    deps = [
        ":unique_pointer_to_local_value_optimizer",
        "//src/ir/representation",
        "//src/ir/serialization",
        "//src/lang/processors",
        "//src/lang/processors/ir/check:check_test_util",
        "//src/lang/representation",
        "@gtest//:gtest_main",
    ],
)
