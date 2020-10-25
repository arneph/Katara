load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "type_checker",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = ["-Isrc/ -std=c++17"],
    deps = [
        "//src/lang/representation",
        "//src/lang/processors/issues",
     ],
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
)
