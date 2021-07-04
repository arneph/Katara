load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "processors",
    srcs = [
        "live_range_analyzer.cc",
        "phi_resolver.cc",
        "register_allocator.cc",
    ],
    hdrs = [
        "live_range_analyzer.h",
        "phi_resolver.h",
        "register_allocator.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/ir:__subpackages__",
    ],
    deps = [
        "//src/ir/info",
        "//src/ir/representation",
    ],
)
