load("@rules_cc//cc:defs.bzl", "cc_library")
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
