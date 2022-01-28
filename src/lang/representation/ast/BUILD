load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "nodes",
    srcs = ["nodes.cc"],
    hdrs = ["nodes.h"],
    copts = COPTS,
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
    deps = [
        "//src/common:logging",
        "//src/lang/representation/positions",
        "//src/lang/representation/tokens",
    ],
)

cc_library(
    name = "ast",
    srcs = [
        "ast.cc",
        "ast_builder.cc",
    ],
    hdrs = [
        "ast.h",
        "ast_builder.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
    deps = [
        ":nodes",
    ],
)

cc_library(
    name = "ast_util",
    srcs = ["ast_util.cc"],
    hdrs = ["ast_util.h"],
    copts = COPTS,
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
    deps = [
        ":nodes",
        "//src/common/graph",
        "//src/lang/representation/positions",
    ],
)
