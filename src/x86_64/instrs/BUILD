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
        "//src/common/data_view",
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
        "//src/common/data_view",
        "//src/x86_64:ops",
    ],
)

cc_library(
    name = "instrs",
    srcs = [
        "arithmetic_logic_instrs.cc",
        "control_flow_instrs.cc",
        "data_instrs.cc",
        "instr_cond.cc",
    ],
    hdrs = [
        "arithmetic_logic_instrs.h",
        "control_flow_instrs.h",
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
        "//src/common/data_view",
        "//src/x86_64:ops",
        "//src/x86_64/machine_code:linker",
    ],
)
