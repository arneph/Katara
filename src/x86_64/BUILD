load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "katara_x86_64_lib",
    srcs = glob(
        ["**/*.cc"],
        exclude=["**/*-test.cc"],
    ),
    hdrs = glob(["**/*.h"]),
    copts = ["-Isrc/ -std=c++17"],
    deps = [
        "//src/common:katara_common_lib",
        "//src/ir:katara_ir_lib",
        "//src/ir_info:katara_ir_info_lib",
    ],
    visibility = [
	"//src:__pkg__",
    ], 
)

cc_test(
    name = "katara_x86_64_lib-test",
    srcs = glob(["**/*-test.cc"]),
    copts = ["-Isrc/ -std=c++17 -Iexternal/gtest/include"],
    deps = [
        "@gtest//:main",
        "//src/x86_64:katara_x86_64_lib",
    ],
)