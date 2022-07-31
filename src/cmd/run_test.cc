//
//  run_test.cc
//  Katara
//
//  Created by Arne Philipeit on 1/30/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/cmd/run.h"

#include "gtest/gtest.h"
#include "src/cmd/context/context.h"
#include "src/cmd/context/test_context.h"

namespace cmd {

class RunTest : public testing::TestWithParam<BuildOptions> {};

INSTANTIATE_TEST_SUITE_P(RunTestInstance, RunTest,
                         testing::Values(
                             BuildOptions{
                                 .optimize_ir_ext = false,
                                 .optimize_ir = false,
                             },
                             BuildOptions{
                                 .optimize_ir_ext = false,
                                 .optimize_ir = true,
                             },
                             BuildOptions{
                                 .optimize_ir_ext = true,
                                 .optimize_ir = false,
                             },
                             BuildOptions{
                                 .optimize_ir_ext = true,
                                 .optimize_ir = true,
                             }));

TEST_P(RunTest, RunsSumCorrectly) {
  TestContext ctx;
  ctx.filesystem()->WriteContentsOfFile("test.kat", R"kat(
package main

func main() int {
  var sum int
  for i := 0; i < 10; i++ {
    sum += i
  }
  return sum
}
  )kat");

  std::vector<std::filesystem::path> paths{"test.kat"};
  BuildOptions build_options = GetParam();
  ErrorCode result =
      ::cmd::Run(paths, build_options, DebugHandler::WithDebugEnabledButOutputDisabled(), &ctx);

  EXPECT_EQ(result, 45);
}

TEST_P(RunTest, RunsRecursiveFibonacci1Correctly) {
  TestContext ctx;
  ctx.filesystem()->WriteContentsOfFile("test.kat", R"kat(
package main

func fib(n int) int {
  if 0 == n || 1 == n {
    return 1
  } else {
    return fib(n-1) + fib(n-2)
  }
}

func main() int {
  return fib(11)
}
)kat");

  std::vector<std::filesystem::path> paths{"test.kat"};
  BuildOptions build_options = GetParam();
  ErrorCode result =
      ::cmd::Run(paths, build_options, DebugHandler::WithDebugEnabledButOutputDisabled(), &ctx);

  EXPECT_EQ(result, 144);
}

TEST_P(RunTest, RunsRecursiveFibonacci2Correctly) {
  TestContext ctx;
  ctx.filesystem()->WriteContentsOfFile("test.kat", R"kat(
package main

func fib(n int) int {
  if 0 <= n <= 1 {
    return 1
  } else {
    return fib(n-1) + fib(n-2)
  }
}

func main() int {
  return fib(11)
}
)kat");

  std::vector<std::filesystem::path> paths{"test.kat"};
  BuildOptions build_options = GetParam();
  ErrorCode result =
      ::cmd::Run(paths, build_options, DebugHandler::WithDebugEnabledButOutputDisabled(), &ctx);

  EXPECT_EQ(result, 144);
}

TEST_P(RunTest, RunsNewCorrectly) {
  TestContext ctx;
  ctx.filesystem()->WriteContentsOfFile("test.kat", R"kat(
package main

func inc(a *int64) {
  *a++
}

func main() int64 {
  x := new<int64>()
  *x = 42
  inc(x)
  return *x
}
)kat");

  std::vector<std::filesystem::path> paths{"test.kat"};
  BuildOptions build_options = GetParam();
  ErrorCode result =
      ::cmd::Run(paths, build_options, DebugHandler::WithDebugEnabledButOutputDisabled(), &ctx);

  EXPECT_EQ(result, 43);
}

TEST_P(RunTest, RunsAddressOfLocalVariableCorrectly) {
  TestContext ctx;
  ctx.filesystem()->WriteContentsOfFile("test.kat", R"kat(
package main

func create() *int64 {
  var a int64 = 42
  return &a
}

func inc(a *int64) {
  *a++
}

func main() int64 {
  x := create()
  *x *= 3
  inc(x)
  return *x
}
)kat");

  std::vector<std::filesystem::path> paths{"test.kat"};
  BuildOptions build_options = GetParam();
  ErrorCode result =
      ::cmd::Run(paths, build_options, DebugHandler::WithDebugEnabledButOutputDisabled(), &ctx);

  EXPECT_EQ(result, 127);
}

}  // namespace cmd
