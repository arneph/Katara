load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_test")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "x86_64_lib",
    srcs = [
        "block.cc",
        "coding/instr_decoder.cc",
        "coding/instr_encoder.cc",
        "func.cc",
        "instr.cc",
        "instrs/al_instrs.cc",
        "instrs/cf_instrs.cc",
        "instrs/data_instrs.cc",
        "instrs/instr_cond.cc",
        "mc/linker.cc",
        "mc/unlinker.cc",
        "ops.cc",
        "prog.cc",
    ],
    hdrs = [
        "block.h",
        "coding/instr_decoder.h",
        "coding/instr_encoder.h",
        "func.h",
        "instr.h",
        "instrs/al_instrs.h",
        "instrs/cf_instrs.h",
        "instrs/data_instrs.h",
        "instrs/instr_cond.h",
        "mc/linker.h",
        "mc/unlinker.h",
        "ops.h",
        "prog.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/common:data",
        "//src/common:graph",
        "//src/ir:ir_lib",
    ],
)

cc_test(
    name = "ops_test",
    srcs = ["ops_test.cc"],
    copts = COPTS,
    deps = [
        ":x86_64_lib",
        "@gtest//:gtest_main",
    ],
)
