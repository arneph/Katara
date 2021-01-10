load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "katara_lang_lib",
    copts = COPTS,
    deps = [
        "//src/lang/representation",
        "//src/lang/processors",
     ],
    visibility = [
        "//visibility:public",
    ],
)
