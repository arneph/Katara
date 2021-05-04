load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "base",
    srcs = [
        "objects.cc",
        "scope.cc",
        "types.cc",
    ],
    hdrs = [
        "objects.h",
        "package.h",
        "scope.h",
        "types.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
    deps = [
        "//src/ir/representation:types",
        "//src/lang/representation/ast",
        "//src/lang/representation/constants",
        "//src/lang/representation/positions",
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
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
    deps = [
        ":base",
        ":info",
        "//src/lang/representation/ast",
        "//src/lang/representation/positions",
        "//src/vcg:vcg_lib",
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
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
    deps = [
        ":base",
    ],
)

cc_library(
    name = "selection",
    hdrs = [
        "selection.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
    deps = [
        ":base",
    ],
)

cc_library(
    name = "initializer",
    hdrs = [
        "initializer.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
    deps = [
        ":base",
        "//src/lang/representation/ast",
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
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
    deps = [
        ":base",
        ":expr_info",
        ":initializer",
        ":selection",
        "//src/lang/representation/ast",
        "//src/lang/representation/constants",
        "//src/lang/representation/positions",
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
    visibility = [
        "//src/lang/representation:__subpackages__",
    ],
    deps = [
        ":base",
        ":expr_info",
        ":info",
        ":initializer",
        ":selection",
        "//src/lang/representation/ast",
        "//src/lang/representation/constants",
        "//src/lang/representation/positions",
    ],
)
