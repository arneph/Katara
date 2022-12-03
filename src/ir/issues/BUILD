load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "issues",
    srcs = [
        "issues.cc",
    ],
    hdrs = [
        "issues.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
    deps = [
        "//src/common/issues",
        "//src/common/positions",
        "//src/ir/representation",
    ],
)
