load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_fuzzing//fuzzing:cc_deps.bzl", "cc_fuzz_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "parser",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = COPTS,
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
    deps = [
        "//src/lang/processors/issues",
        "//src/lang/processors/scanner",
        "//src/lang/representation",
    ],
)

cc_fuzz_test(
    name = "parser_fuzz_test",
    srcs = ["parser_fuzz_test.cc"],
    copts = COPTS,
    deps = [
        ":parser",
        "//src/lang/processors/issues",
        "//src/lang/representation",
    ],
)
