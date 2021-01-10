load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_fuzzing//fuzzing:cc_deps.bzl", "cc_fuzz_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "parser",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = COPTS,
    deps = [
        "//src/lang/representation",
        "//src/lang/processors/issues",
        "//src/lang/processors/scanner",
     ],
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
)

cc_fuzz_test(
    name = "parser_fuzz_test",
    srcs = ["parser_fuzz_test.cc"],
    copts = COPTS,
    deps = [
        "//src/lang/representation",
        "//src/lang/processors/issues",
        ":parser",
    ],
)
