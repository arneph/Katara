load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "util",
    srcs = ["util.cc"],
    hdrs = ["util.h"],
    copts = COPTS,
    visibility = [
        "//src/cmd:__subpackages__",
    ],
)

cc_library(
    name = "context",
    hdrs = ["context.h"],
    copts = COPTS,
    visibility = [
        "//src/cmd:__subpackages__",
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
        "//src/cmd:__subpackages__",
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
        "//src/cmd:__subpackages__",
    ],
    deps = [
        ":context",
        "//src/common/filesystem",
        "//src/common/filesystem:test_filesystem",
    ],
)

cc_library(
    name = "version",
    srcs = ["version.cc"],
    hdrs = ["version.h"],
    copts = COPTS,
    visibility = [
        "//src/cmd:__subpackages__",
    ],
    deps = [
        ":context",
    ],
)

cc_library(
    name = "repl",
    srcs = ["repl.cc"],
    hdrs = ["repl.h"],
    copts = COPTS,
    visibility = [
        "//src/cmd:__subpackages__",
    ],
    deps = [
        ":context",
        "//src/common/logging",
    ],
)
