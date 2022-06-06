load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "checker",
    srcs = [
        "checker.cc",
    ],
    hdrs = [
        "checker.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/lang/processors/ir:__subpackages__",
    ],
    deps = [
        "//src/common/logging",
        "//src/ir/checker",
        "//src/ir/representation",
        "//src/lang/representation",
    ],
)
