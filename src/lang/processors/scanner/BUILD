load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "scanner",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = ["-Isrc/ -std=c++17"],
    deps = [
        "//src/lang/representation",
     ],
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
)
