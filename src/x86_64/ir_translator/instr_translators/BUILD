load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "arithmetic_logic_instrs_translator",
    srcs = [
        "arithmetic_logic_instrs_translator.cc",
    ],
    hdrs = [
        "arithmetic_logic_instrs_translator.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64/ir_translator:__pkg__",
    ],
    deps = [
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
        "//src/x86_64/ir_translator:context",
        "//src/x86_64/ir_translator:mov_generator",
        "//src/x86_64/ir_translator:size_translator",
        "//src/x86_64/ir_translator:temporary_reg",
        "//src/x86_64/ir_translator:value_translator",
    ],
)

cc_library(
    name = "control_flow_instrs_translator",
    srcs = [
        "control_flow_instrs_translator.cc",
    ],
    hdrs = [
        "control_flow_instrs_translator.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64/ir_translator:__pkg__",
    ],
    deps = [
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
        "//src/x86_64/ir_translator:context",
        "//src/x86_64/ir_translator:register_allocator",
        "//src/x86_64/ir_translator:size_translator",
        "//src/x86_64/ir_translator:value_translator",
    ],
)

cc_library(
    name = "data_instrs_translator",
    srcs = [
        "data_instrs_translator.cc",
    ],
    hdrs = [
        "data_instrs_translator.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64/ir_translator:__pkg__",
    ],
    deps = [
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
        "//src/x86_64/ir_translator:context",
        "//src/x86_64/ir_translator:mov_generator",
        "//src/x86_64/ir_translator:size_translator",
        "//src/x86_64/ir_translator:temporary_reg",
        "//src/x86_64/ir_translator:value_translator",
    ],
)
