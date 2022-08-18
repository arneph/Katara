load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "katara_lib",
    copts = COPTS,
    deps = [
        "//src/ir:ir_lib",
        "//src/lang:lang_lib",
        "//src/x86_64:x86_64_lib",
        "//src/x86_64/ir_translator",
    ],
)

cc_binary(
    name = "katara-tests",
    srcs = ["tests.cc"],
    copts = COPTS,
    deps = [
        ":katara_lib",
    ],
)
