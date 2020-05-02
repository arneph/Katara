load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "katara_ir_lib",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = ["-Isrc/ -std=c++17"],
    deps = [
	"//src/vcg:katara_vcg_lib",
     ],
    visibility = [
	"//src/ir_info:__pkg__",
	"//src/ir_processors:__pkg__",
	"//src/x86_64:__pkg__",
	"//src:__pkg__",
    ],
)
