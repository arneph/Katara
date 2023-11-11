load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "shared_pointer_lowerer",
    srcs = ["shared_pointer_lowerer.cc"],
    hdrs = ["shared_pointer_lowerer.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/ir:ir_lib",
        "//src/lang/processors/ir/serialization:parse",
        "//src/lang/representation",
        "//src/lang/runtime",
    ],
)

cc_test(
    name = "shared_pointer_lowerer_test",
    srcs = ["shared_pointer_lowerer_test.cc"],
    copts = COPTS,
    deps = [
        ":shared_pointer_lowerer",
        "//src/ir:ir_lib",
        "//src/lang/processors/ir/check",
        "//src/lang/processors/ir/check:check_test_util",
        "//src/lang/processors/ir/serialization:parse",
        "//src/lang/representation",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "unique_pointer_lowerer",
    srcs = ["unique_pointer_lowerer.cc"],
    hdrs = ["unique_pointer_lowerer.h"],
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
    name = "unique_pointer_lowerer_test",
    srcs = ["unique_pointer_lowerer_test.cc"],
    copts = COPTS,
    deps = [
        ":unique_pointer_lowerer",
        "//src/ir:ir_lib",
        "//src/lang/processors/ir/check:check_test_util",
        "//src/lang/processors/ir/serialization:parse",
        "//src/lang/representation",
        "@gtest//:gtest_main",
    ],
)
