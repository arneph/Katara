load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "katara_ir_info_lib",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = COPTS,
    deps = [
	"//src/ir:katara_ir_lib",
    ],
    visibility = [
        "//src/ir_processors:__pkg__",
        "//src/x86_64:__pkg__",
	"//src:__pkg__",
    ],
)
