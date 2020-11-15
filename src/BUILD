load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "katara_lib",
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        "//src/common:katara_common_lib",
        "//src/lang:katara_lang_lib",
        "//src/ir:katara_ir_lib",
        "//src/ir_info:katara_ir_info_lib",
        "//src/ir_processors:katara_ir_processors_lib",
        "//src/vcg:katara_vcg_lib",
        "//src/x86_64:katara_x86_64_lib",
    ],
)

cc_binary(
    name = "katara",
    srcs = ["main.cc"],
    copts = ["-Isrc/ -std=c++2a"],
    deps = [
        ":katara_lib",
    ],
)
