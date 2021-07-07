load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "instr_encoder",
    srcs = [
        "instr_encoder.cc",
    ],
    hdrs = [
        "instr_encoder.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/common:data",
        "//src/x86_64:ops",
    ],
)

cc_library(
    name = "instr_decoder",
    srcs = [
        "instr_decoder.cc",
    ],
    hdrs = [
        "instr_decoder.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/common:data",
        "//src/x86_64:ops",
    ],
)

cc_library(
    name = "instrs",
    srcs = [
        "al_instrs.cc",
        "cf_instrs.cc",
        "data_instrs.cc",
        "instr_cond.cc",
    ],
    hdrs = [
        "al_instrs.h",
        "cf_instrs.h",
        "data_instrs.h",
        "instr.h",
        "instr_cond.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64:__subpackages__",
    ],
    deps = [
        ":instr_encoder",
        "//src/common:data",
        "//src/x86_64:ops",
        "//src/x86_64/machine_code:linker",
    ],
)
