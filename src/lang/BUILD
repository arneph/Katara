load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "katara_lang_lib",
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation",
        "//src/lang/processors",
     ],
    visibility = [
        "//visibility:public",
    ],
)
