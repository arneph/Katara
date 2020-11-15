load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "ast",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/vcg:katara_vcg_lib",
        "//src/lang/representation/positions",
        "//src/lang/representation/tokens",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)
