load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "ir",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/lang/processors/ir/builder:ir_builder",
        "//src/lang/processors/ir/lowerers:shared_pointer_lowerer",
        "//src/lang/processors/ir/lowerers:unique_pointer_lowerer",
        "//src/lang/processors/ir/optimizers:shared_to_unique_pointer_optimizer",
    ],
)
