load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "type_checker",
    srcs = ["type_checker.cc"],
    hdrs = ["type_checker.h"],
    copts = COPTS,
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
    deps = [
        ":coordinator",
        ":identifier_resolver",
        "//src/lang/processors/issues",
        "//src/lang/representation",
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
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/lang/processors/issues",
        "//src/lang/representation",
    ],
)

cc_library(
    name = "type_resolver",
    srcs = [
        "base_handler.cc",
        "decl_handler.cc",
        "expr_handler.cc",
        "stmt_handler.cc",
        "type_handler.cc",
    ],
    hdrs = [
        "base_handler.h",
        "decl_handler.h",
        "expr_handler.h",
        "stmt_handler.h",
        "type_handler.h",
        "type_resolver.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/common/atomics",
        "//src/lang/processors/issues",
        "//src/lang/representation",
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
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":type_resolver",
        "//src/lang/processors/issues",
        "//src/lang/representation",
    ],
)
