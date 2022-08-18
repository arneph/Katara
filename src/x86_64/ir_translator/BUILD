load("@rules_cc//cc:defs.bzl", "cc_library")
load("//src:katara.bzl", "COPTS")

cc_library(
    name = "context",
    srcs = [
        "context.cc",
    ],
    hdrs = [
        "context.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64/ir_translator:__subpackages__",
    ],
    deps = [
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
    ],
)

cc_library(
    name = "register_allocator",
    srcs = [
        "register_allocator.cc",
    ],
    hdrs = [
        "register_allocator.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64/ir_translator:__subpackages__",
    ],
    deps = [
        "//src/common/logging",
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
    ],
)

cc_library(
    name = "temporary_reg",
    srcs = [
        "temporary_reg.cc",
    ],
    hdrs = [
        "temporary_reg.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64/ir_translator:__subpackages__",
    ],
    deps = [
        ":context",
        ":register_allocator",
        "//src/common/logging",
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
    ],
)

cc_library(
    name = "size_translator",
    srcs = [
        "size_translator.cc",
    ],
    hdrs = [
        "size_translator.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64/ir_translator:__subpackages__",
    ],
    deps = [
        "//src/common/atomics",
        "//src/common/logging",
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
    ],
)

cc_library(
    name = "value_translator",
    srcs = [
        "value_translator.cc",
    ],
    hdrs = [
        "value_translator.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64/ir_translator:__subpackages__",
    ],
    deps = [
        ":context",
        ":register_allocator",
        "//src/common/logging",
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
    ],
)

cc_library(
    name = "mov_generator",
    srcs = [
        "mov_generator.cc",
    ],
    hdrs = [
        "mov_generator.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64/ir_translator:__subpackages__",
    ],
    deps = [
        ":context",
        ":size_translator",
        ":temporary_reg",
        ":value_translator",
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
    ],
)

cc_test(
    name = "mov_generator_test",
    srcs = [
        "mov_generator_test.cc",
    ],
    copts = COPTS,
    deps = [
        ":context",
        ":mov_generator",
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
        "//src/x86_64/ir_translator/testing:instr_translator_test_setup",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "call_generator",
    srcs = [
        "call_generator.cc",
    ],
    hdrs = [
        "call_generator.h",
    ],
    copts = COPTS,
    visibility = [
        "//src/x86_64/ir_translator:__subpackages__",
    ],
    deps = [
        ":context",
        ":mov_generator",
        ":register_allocator",
        ":size_translator",
        ":value_translator",
        "//src/common/logging",
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
    ],
)

cc_test(
    name = "call_generator_test",
    srcs = [
        "call_generator_test.cc",
    ],
    copts = COPTS,
    deps = [
        ":call_generator",
        ":context",
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
        "//src/x86_64/ir_translator/testing:instr_translator_test_setup",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "instrs_translator",
    srcs = [
        "instrs_translator.cc",
    ],
    hdrs = [
        "instrs_translator.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "//src/common/logging",
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
        "//src/x86_64/ir_translator/instr_translators:arithmetic_logic_instrs_translator",
        "//src/x86_64/ir_translator/instr_translators:control_flow_instrs_translator",
        "//src/x86_64/ir_translator/instr_translators:data_instrs_translator",
    ],
)

cc_library(
    name = "func_translator",
    srcs = [
        "func_translator.cc",
    ],
    hdrs = [
        "func_translator.h",
    ],
    copts = COPTS,
    visibility = [
        "//visibility:private",
    ],
    deps = [
        ":context",
        ":instrs_translator",
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
    ],
)

cc_library(
    name = "ir_translator",
    srcs = [
        "ir_translator.cc",
    ],
    hdrs = [
        "ir_translator.h",
    ],
    copts = COPTS,
    visibility = [
        "//src:__pkg__",
        "//src/cmd/katara:__pkg__",
    ],
    deps = [
        ":func_translator",
        ":register_allocator",
        "//src/common/data_view",
        "//src/common/graph",
        "//src/ir:ir_lib",
        "//src/x86_64:x86_64_lib",
    ],
)
