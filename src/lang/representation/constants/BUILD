load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "constants",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation/tokens",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)
