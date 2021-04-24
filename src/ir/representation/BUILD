load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "representation",
    srcs = [
        "block.cc",
        "func.cc",
        "instr.cc",
        "program.cc",
        "value.cc",
    ],
    hdrs = [
        "block.h",
        "func.h",
        "instr.h",
        "program.h",
        "value.h",
    ],
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
