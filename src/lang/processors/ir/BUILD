load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

exports_files(["ir_ext_file_check.cc"])

cc_library(
    name = "ir",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/lang/processors/ir/builder:ir_builder",
        "//src/lang/processors/ir/checker",
        "//src/lang/processors/ir/lowerers:shared_pointer_lowerer",
        "//src/lang/processors/ir/lowerers:unique_pointer_lowerer",
        "//src/lang/processors/ir/optimizers:shared_to_unique_pointer_optimizer",
        "//src/lang/processors/ir/optimizers:unique_pointer_to_local_value_optimizer",
        "//src/lang/processors/ir/serialization:parse",
    ],
)
