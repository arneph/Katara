load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "package",
    hdrs = ["package.h"],
    copts = COPTS,
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
    deps = [
        "//src/lang/processors/issues",
        "//src/lang/representation",
    ],
)

cc_library(
    name = "package_manager",
    srcs = ["package_manager.cc"],
    hdrs = ["package_manager.h"],
    copts = COPTS,
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
    deps = [
        ":package",
        "//src/common/filesystem",
        "//src/lang/processors/issues",
        "//src/lang/processors/parser",
        "//src/lang/processors/type_checker",
        "//src/lang/representation",
    ],
)
