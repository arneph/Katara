load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "representation",
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation/positions",
        "//src/lang/representation/tokens",
        "//src/lang/representation/ast",
        "//src/lang/representation/constants",
        "//src/lang/representation/types",
     ],
    visibility = [
        "//visibility:public",
    ],
)
