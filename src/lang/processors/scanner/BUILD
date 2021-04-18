load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "scanner",
    srcs = ["scanner.cc"],
    hdrs = ["scanner.h"],
    copts = COPTS,
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
    deps = [
        "//src/lang/representation",
    ],
)
