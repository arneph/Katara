load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "processors",
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/processors/issues",
        "//src/lang/processors/scanner",
        "//src/lang/processors/parser",
        "//src/lang/processors/type_checker",
        "//src/lang/processors/packages",
     ],
    visibility = [
        "//visibility:public",
    ],
)
