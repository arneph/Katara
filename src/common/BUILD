load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "katara_common_lib",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = COPTS,
    visibility = [
        "//src/x86_64:__pkg__",
	"//src:__pkg__",
    ],
)
