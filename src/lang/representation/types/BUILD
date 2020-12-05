load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "base",
    srcs = glob([
        "types.cc",
        "objects.cc",
        "scope.cc",
        "package.cc",
    ]),
    hdrs = glob([
        "types.h",
        "objects.h",
        "scope.h",
        "package.h",
    ]),
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation/positions",
        "//src/lang/representation/ast",
        "//src/lang/representation/constants",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)

cc_library(
    name = "types_util",
    srcs = glob([
        "types_util.cc",
    ]),
    hdrs = glob([
        "types_util.h",
    ]),
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/vcg:katara_vcg_lib",
        "//src/lang/representation/positions",
        "//src/lang/representation/ast",
        ":base",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)

cc_library(
    name = "initializer",
    srcs = glob([
        "initializer.cc",
    ]),
    hdrs = glob([
        "initializer.h",
    ]),
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation/ast",
        ":base",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)

cc_library(
    name = "info",
    srcs = glob([
        "info.cc",
        "info_builder.cc",
    ]),
    hdrs = glob([
        "info.h",
        "info_builder.h",
    ]),
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation/positions",
        "//src/lang/representation/ast",
        "//src/lang/representation/constants",
        ":base",
        ":initializer",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)

cc_library(
    name = "info_util",
    srcs = glob([
        "info_util.cc",
    ]),
    hdrs = glob([
        "info_util.h",
    ]),
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation/positions",
        "//src/lang/representation/ast",
        "//src/lang/representation/constants",
        ":base",
        ":initializer",
        ":info",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)
