load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "x86_64_lib",
    srcs = glob(
        ["**/*.cc"],
        exclude = ["**/*-test.cc"],
    ),
    hdrs = glob(["**/*.h"]),
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
    ],
    deps = [
        "//src/common:common_lib",
        "//src/ir:ir_lib",
    ],
)

cc_test(
    name = "katara_x86_64_lib-test",
    srcs = glob(["**/*-test.cc"]),
    copts = COPTS + ["-Iexternal/gtest/include"],
    deps = [
        "//src/x86_64:katara_x86_64_lib",
        "@gtest//:main",
    ],
)
