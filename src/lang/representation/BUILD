load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "representation",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/lang/representation/ast",
        "//src/lang/representation/ast:ast_util",
        "//src/lang/representation/ast:nodes",
        "//src/lang/representation/constants",
        "//src/lang/representation/positions",
        "//src/lang/representation/tokens",
        "//src/lang/representation/types:base",
        "//src/lang/representation/types:info",
        "//src/lang/representation/types:info_util",
        "//src/lang/representation/types:initializer",
        "//src/lang/representation/types:types_util",
    ],
)
