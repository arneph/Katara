load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@io_bazel_rules_go//go:def.bzl", "go_test")
load("//src:katara.bzl", "COPTS")

cc_binary(
    name = "execution_test_executable",
    srcs = ["execution_test.cc"],
    copts = COPTS,
    deps = [
        "//src/common/data_view",
        "//src/x86_64:x86_64_lib",
    ],
)

go_test(
    name = "execution_test",
    srcs = ["execution_test.go"],
    data = [
        ":execution_test_executable",
    ],
)
