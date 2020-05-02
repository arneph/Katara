load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "katara_ir_info_lib",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = ["-Isrc/ -std=c++17"],
    deps = [
	"//src/ir:katara_ir_lib",
    ],
    visibility = [
        "//src/ir_processors:__pkg__",
        "//src/x86_64:__pkg__",
	"//src:__pkg__",
    ],
)
