load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "linker",
    srcs = [
        "linker.cc",
    ],
    hdrs = [
        "linker.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64:__subpackages__",
    ],
    deps = [
        "//src/common/data_view",
        "//src/x86_64:ops",
    ],
)

cc_library(
    name = "unlinker",
    srcs = [
        "unlinker.cc",
    ],
    hdrs = [
        "unlinker.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64:__subpackages__",
    ],
    deps = [
        "//src/common/data_view",
        "//src/x86_64:ops",
    ],
)
