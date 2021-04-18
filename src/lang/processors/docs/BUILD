load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "common",
    srcs = ["common.cc"],
    hdrs = ["common.h"],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/lang/representation",
    ],
)

cc_library(
    name = "file_doc",
    srcs = ["file_doc.cc"],
    hdrs = ["file_doc.h"],
    copts = COPTS,
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
    deps = [
        ":common",
        "//src/lang/processors/packages",
        "//src/lang/processors/scanner",
        "//src/lang/representation",
    ],
)

cc_library(
    name = "package_doc",
    srcs = ["package_doc.cc"],
    hdrs = ["package_doc.h"],
    copts = COPTS,
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
    deps = [
        ":common",
        ":file_doc",
        "//src/lang/processors/packages",
        "//src/lang/representation",
    ],
)
