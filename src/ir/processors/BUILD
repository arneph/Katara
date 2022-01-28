load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "phi_resolver",
    srcs = [
        "phi_resolver.cc",
    ],
    hdrs = [
        "phi_resolver.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/ir/representation",
    ],
)

cc_test(
    name = "phi_resolver_test",
    srcs = ["phi_resolver_test.cc"],
    copts = COPTS,
    deps = [
        ":phi_resolver",
        "//src/common/atomics",
        "//src/ir/representation",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "processors",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":phi_resolver",
    ],
)
