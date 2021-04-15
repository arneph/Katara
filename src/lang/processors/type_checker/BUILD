load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "type_checker",
    srcs = ["type_checker.cc"],
    hdrs = ["type_checker.h"],
    copts = COPTS,
    deps = [
        "//src/lang/representation",
        "//src/lang/processors/issues",
        ":identifier_resolver",
        ":coordinator",
    ],
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
)

cc_library(
    name = "identifier_resolver",
    srcs = [
        "identifier_resolver.cc",
    ],
    hdrs = [
        "identifier_resolver.h",
    ],
    copts = COPTS,
    deps = [
        "//src/lang/representation",
        "//src/lang/processors/issues",
    ],
    visibility = [
        "//visibility:private",
    ],
)

cc_library(
    name = "type_resolver",
    srcs = [
        "base_handler.cc",
        "type_handler.cc",
        "expr_handler.cc",
        "stmt_handler.cc",
        "decl_handler.cc",
    ],
    hdrs = [
        "base_handler.h",
        "type_resolver.h",
        "type_handler.h",
        "expr_handler.h",
        "stmt_handler.h",
        "decl_handler.h",
    ],
    copts = COPTS,
    deps = [
        "//src/lang/representation",
        "//src/lang/processors/issues",
    ],
    visibility = [
        "//visibility:private",
    ],
)

cc_library(
    name = "coordinator",
    srcs = [
        "coordinator.cc",
    ],
    hdrs = [
        "coordinator.h",
    ],
    copts = COPTS,
    deps = [
        "//src/lang/representation",
        "//src/lang/processors/issues",
        ":type_resolver",
    ],
    visibility = [
        "//visibility:private",
    ],
)
