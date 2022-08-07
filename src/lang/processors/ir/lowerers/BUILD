load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")
load("//src/lang/processors/ir:ir.bzl", "ir_ext_file_check")

ir_ext_file_check(
    name = "shared_pointer_impl_check",
    src = ":shared_pointer_impl.ir",
)

cc_library(
    name = "shared_pointer_lowerer",
    srcs = ["shared_pointer_lowerer.cc"],
    hdrs = ["shared_pointer_lowerer.h"],
    copts = COPTS,
    data = [":shared_pointer_impl.ir"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/ir:ir_lib",
        "//src/lang/processors/ir/serialization:parse",
        "//src/lang/representation",
    ],
)

cc_test(
    name = "shared_pointer_lowerer_test",
    srcs = ["shared_pointer_lowerer_test.cc"],
    copts = COPTS,
    deps = [
        ":shared_pointer_lowerer",
        "//src/ir:ir_lib",
        "//src/lang/processors/ir/checker",
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
        "//src/lang/processors/ir/checker",
        "//src/lang/processors/ir/serialization:parse",
        "//src/lang/representation",
        "@gtest//:gtest_main",
    ],
)
