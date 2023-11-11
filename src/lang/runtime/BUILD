load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")
load("//src/lang/processors/ir:ir.bzl", "ir_ext_file_check")

ir_ext_file_check(
    name = "shared_pointer_check",
    src = ":shared_pointer.ir",
)

cc_library(
    name = "shared_pointer",
    srcs = ["shared_pointer.cc"],
    hdrs = ["shared_pointer.h"],
    copts = COPTS,
    data = [":shared_pointer.ir"],
    deps = [
        "//src/ir:ir_lib",
        "//src/lang/processors/ir/serialization:parse",
    ],
)

cc_test(
    name = "shared_pointer_test",
    srcs = ["shared_pointer_test.cc"],
    copts = COPTS,
    deps = [
        ":shared_pointer",
        "//src/ir:ir_lib",
        "//src/lang/processors/ir/check:check_test_util",
        "//src/lang/processors/ir/serialization:parse",
        "//src/lang/representation",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "runtime",
    srcs = ["runtime.cc"],
    hdrs = ["runtime.h"],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":shared_pointer",
    ],
)
