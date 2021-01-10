load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "tokens",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = COPTS,
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)
