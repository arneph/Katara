load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "processors",
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/lang/processors/docs:file_doc",
        "//src/lang/processors/docs:package_doc",
        "//src/lang/processors/ir_builder",
        "//src/lang/processors/ir_lowerers:shared_pointer_lowerer",
        "//src/lang/processors/issues",
        "//src/lang/processors/packages:filesystem_loader",
        "//src/lang/processors/packages:loader",
        "//src/lang/processors/packages:package",
        "//src/lang/processors/packages:package_manager",
        "//src/lang/processors/parser",
        "//src/lang/processors/scanner",
        "//src/lang/processors/type_checker",
    ],
)
