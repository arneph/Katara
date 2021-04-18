load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "lang_lib",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/lang/processors",
        "//src/lang/representation",
    ],
)
