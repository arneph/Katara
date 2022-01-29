load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "context",
    srcs = ["context.cc"],
    hdrs = ["context.h"],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/cmd:__pkg__",
    ],
    deps = [
        "//src/common/filesystem",
    ],
)

cc_library(
    name = "real_context",
    hdrs = ["real_context.h"],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/cmd:__pkg__",
    ],
    deps = [
        ":context",
        "//src/common/filesystem",
        "//src/common/filesystem:real_filesystem",
    ],
)

cc_library(
    name = "test_context",
    testonly = True,
    hdrs = ["test_context.h"],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/cmd:__pkg__",
    ],
    deps = [
        ":context",
        "//src/common/filesystem",
        "//src/common/filesystem:test_filesystem",
    ],
)
