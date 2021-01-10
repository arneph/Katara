load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "packages",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = COPTS,
    deps = [
        "//src/lang/representation",
        "//src/lang/processors/issues",
        "//src/lang/processors/parser",
        "//src/lang/processors/type_checker",
     ],
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
)
