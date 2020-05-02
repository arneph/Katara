load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "katara_ir_processors_lib",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = ["-Isrc/ -std=c++17"],
    deps = [
	"//src/ir:katara_ir_lib",
	"//src/ir_info:katara_ir_info_lib",
    ],
    visibility = [
	"//src:__pkg__",
    ],
)
