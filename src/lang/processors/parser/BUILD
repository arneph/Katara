load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "parser",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation",
        "//src/lang/processors/issues",
        "//src/lang/processors/scanner",
     ],
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
)
