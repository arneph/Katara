load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "tokens",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = ["-Isrc/ -std=c++2a"],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)
