load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "gtest",
    build_file = "@//:BUILD",
    sha256 = "b58cb7547a28b2c718d1e38aee18a3659c9e3ff52440297e965f5edffe34b6d0",
    strip_prefix = "googletest-release-1.7.0",
    url = "https://github.com/google/googletest/archive/release-1.7.0.zip",
)

http_archive(
    name = "rules_fuzzing",
    sha256 = "a1cde2a5ccc05bdeb75bd0f4c62c6df966134a50278492468bd03ea8ffcaa133",
    strip_prefix = "rules_fuzzing-4de19aafba32cd586abf1bd66ebd3f8d2ea98350",
    urls = ["https://github.com/bazelbuild/rules_fuzzing/archive/4de19aafba32cd586abf1bd66ebd3f8d2ea98350.zip"],
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:dependency_imports.bzl", "fuzzing_dependency_imports")

fuzzing_dependency_imports()
