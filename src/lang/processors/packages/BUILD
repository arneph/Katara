load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "packages",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = COPTS,
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
    deps = [
        "//src/lang/processors/issues",
        "//src/lang/processors/parser",
        "//src/lang/processors/type_checker",
        "//src/lang/representation",
    ],
)
