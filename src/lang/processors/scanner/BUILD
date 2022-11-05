load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "scanner",
    srcs = ["scanner.cc"],
    hdrs = ["scanner.h"],
    copts = COPTS,
    visibility = [
        "//src/lang/processors:__subpackages__",
    ],
    deps = [
        "//src/lang/representation",
    ],
)

cc_fuzz_test(
    name = "scanner_fuzz_test",
    srcs = ["scanner_fuzz_test.cc"],
    copts = COPTS,
    deps = [
        ":scanner",
        "//src/lang/representation",
    ],
)
