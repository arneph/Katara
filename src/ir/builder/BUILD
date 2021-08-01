load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "builder",
    srcs = [
        "block_builder.cc",
        "func_builder.cc",
    ],
    hdrs = [
        "block_builder.h",
        "func_builder.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/ir:__subpackages__",
        "//src/lang:__subpackages__",
        "//src/x86_64:__pkg__",
    ],
    deps = [
        "//src/common:atomics",
        "//src/ir/representation",
    ],
)
