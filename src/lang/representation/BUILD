load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "representation",
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation/positions",
        "//src/lang/representation/tokens",
        "//src/lang/representation/ast:nodes",
        "//src/lang/representation/ast:ast",
        "//src/lang/representation/ast:ast_util",
        "//src/lang/representation/constants",
        "//src/lang/representation/types:base",
        "//src/lang/representation/types:types_util",
        "//src/lang/representation/types:initializer",
        "//src/lang/representation/types:info",
        "//src/lang/representation/types:info_util",
     ],
    visibility = [
        "//visibility:public",
    ],
)
