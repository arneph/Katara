load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "ir_translator",
    srcs = [
        "ir_translator.cc",
    ],
    hdrs = [
        "ir_translator.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
    ],
    deps = [
        "//src/common:data",
        "//src/common:graph",
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
    ],
)
