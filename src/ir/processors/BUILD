load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "processors",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/ir/info",
        "//src/ir/representation",
    ],
)
