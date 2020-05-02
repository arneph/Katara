load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "katara_vcg_lib",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = ["-Isrc/ -std=c++17"],
    visibility = [
        "//src/ir:__pkg__",
	"//src:__pkg__",
    ],
)
