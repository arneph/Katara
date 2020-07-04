load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "katara_lang_lib",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = ["-Isrc/ -std=c++17"],
    deps = [
	"//src/vcg:katara_vcg_lib",
     ],
    visibility = [
	"//src/ast_processors:__pkg__",
	"//src:__pkg__",
    ],
)
