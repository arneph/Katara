load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "nodes",
    srcs = ["nodes.cc"],
    hdrs = ["nodes.h"],
    copts = COPTS,
    deps = [
        "//src/lang/representation/positions",
        "//src/lang/representation/tokens",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
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
    deps = [
        ":nodes",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)

cc_library(
    name = "ast_util",
    srcs = ["ast_util.cc"],
    hdrs = ["ast_util.h"],
    copts = COPTS,
    deps = [
        "//src/vcg:katara_vcg_lib",
        "//src/lang/representation/positions",
        ":nodes",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)
