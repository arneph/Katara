load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "scanner",
    srcs = [
        "scanner.cc",
    ],
    hdrs = [
        "scanner.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/common:logging",
    ],
)

cc_library(
    name = "parser",
    srcs = [
        "parser.cc",
    ],
    hdrs = [
        "parser.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        ":scanner",
        "//src/common:logging",
        "//src/common/atomics",
        "//src/ir/info",
        "//src/ir/representation",
    ],
)

cc_library(
    name = "serialization",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":parser",
        ":scanner",
    ],
)
