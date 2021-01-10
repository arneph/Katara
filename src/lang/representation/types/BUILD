load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "base",
    srcs = [
        "types.cc",
        "objects.cc",
        "scope.cc",
    ],
    hdrs = [
        "types.h",
        "objects.h",
        "scope.h",
        "package.h",
    ],
    copts = COPTS,
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
    srcs = [
        "types_util.cc",
    ],
    hdrs = [
        "types_util.h",
    ],
    copts = COPTS,
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
    name = "expr_info",
    srcs = [
        "expr_info.cc",
    ],
    hdrs = [
        "expr_info.h",
    ],
    copts = COPTS,
    deps = [
        ":base",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)

cc_library(
    name = "selection",
    hdrs = [
        "selection.h",
    ],
    copts = COPTS,
    deps = [
        ":base",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)

cc_library(
    name = "initializer",
    hdrs = [
        "initializer.h",
    ],
    copts = COPTS,
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
    srcs = [
        "info.cc",
        "info_builder.cc",
    ],
    hdrs = [
        "info.h",
        "info_builder.h",
    ],
    copts = COPTS,
    deps = [
        "//src/lang/representation/positions",
        "//src/lang/representation/ast",
        "//src/lang/representation/constants",
        ":base",
        ":expr_info",
        ":selection",
        ":initializer",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)

cc_library(
    name = "info_util",
    srcs = [
        "info_util.cc",
    ],
    hdrs = [
        "info_util.h",
    ],
    copts = COPTS,
    deps = [
        "//src/lang/representation/positions",
        "//src/lang/representation/ast",
        "//src/lang/representation/constants",
        ":base",
        ":expr_info",
        ":selection",
        ":initializer",
        ":info",
     ],
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
)
