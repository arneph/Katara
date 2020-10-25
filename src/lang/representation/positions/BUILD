load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "positions",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = ["-Isrc/ -std=c++17"],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)
