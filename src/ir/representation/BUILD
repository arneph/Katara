load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "representation",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
    deps = [
        "//src/vcg:vcg_lib",
    ],
)
