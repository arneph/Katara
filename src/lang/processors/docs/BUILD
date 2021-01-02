load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "common",
    srcs = ["common.cc"],
    hdrs = ["common.h"],
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation",
    ],
    visibility = [
        "//visibility:private",
    ],
)

cc_library(
    name = "file_doc",
    srcs = ["file_doc.cc"],
    hdrs = ["file_doc.h"],
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation",
        "//src/lang/processors/scanner",
        "//src/lang/processors/packages",
        ":common",
     ],
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
)

cc_library(
    name = "package_doc",
    srcs = ["package_doc.cc"],
    hdrs = ["package_doc.h"],
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/lang/representation",
        "//src/lang/processors/packages",
        ":common",
        ":file_doc",
     ],
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
)
