load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "type_checker",
    srcs = ["type_checker.cc"],
    hdrs = ["type_checker.h"],
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation",
        "//src/lang/processors/issues",
        ":type_checker_internal",
    ],
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
)

cc_library(
    name = "type_checker_internal",
    srcs = [
        "universe_builder.cc",
        "identifier_resolver.cc",
        "package_handler.cc",
        "type_handler.cc",
        "constant_handler.cc",
        "variable_handler.cc",
        "expr_handler.cc",
    ],
    hdrs = [
        "universe_builder.h",
        "identifier_resolver.h",
        "package_handler.h",
        "type_handler.h",
        "constant_handler.h",
        "variable_handler.h",
        "expr_handler.h",
    ],
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation",
        "//src/lang/processors/issues",
    ],
    visibility = [
        "//visibility:private",
    ],
)
