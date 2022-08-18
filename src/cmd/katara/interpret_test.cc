//
//  interpret_test.cc
//  Katara
//
//  Created by Arne Philipeit on 1/30/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/cmd/katara/interpret.h"

#include "gtest/gtest.h"
#include "src/cmd/katara/context/context.h"
#include "src/cmd/katara/context/test_context.h"

namespace cmd {

struct Options {
  BuildOptions build_options;
  InterpretOptions interpret_options;
};

class InterpretTest : public testing::TestWithParam<Options> {};

INSTANTIATE_TEST_SUITE_P(InterpretTestInstance, InterpretTest,
                         testing::Values(
                             Options{
                                 .build_options =
                                     BuildOptions{
                                         .optimize_ir_ext = false,
                                         .optimize_ir = false,
                                     },
                                 .interpret_options =
                                     InterpretOptions{
                                         .sanitize = false,
                                     },
                             },
                             Options{
                                 .build_options =
                                     BuildOptions{
                                         .optimize_ir_ext = true,
                                         .optimize_ir = false,
                                     },
                                 .interpret_options =
                                     InterpretOptions{
                                         .sanitize = false,
                                     },
                             },
                             Options{
                                 .build_options =
                                     BuildOptions{
                                         .optimize_ir_ext = false,
                                         .optimize_ir = true,
                                     },
                                 .interpret_options =
                                     InterpretOptions{
                                         .sanitize = false,
                                     },
                             },
                             Options{
                                 .build_options =
                                     BuildOptions{
                                         .optimize_ir_ext = true,
                                         .optimize_ir = true,
                                     },
                                 .interpret_options =
                                     InterpretOptions{
                                         .sanitize = false,
                                     },
                             },
                             Options{
                                 .build_options =
                                     BuildOptions{
                                         .optimize_ir_ext = false,
                                         .optimize_ir = false,
                                     },
                                 .interpret_options =
                                     InterpretOptions{
                                         .sanitize = true,
                                     },
                             },
                             Options{
                                 .build_options =
                                     BuildOptions{
                                         .optimize_ir_ext = true,
                                         .optimize_ir = false,
                                     },
                                 .interpret_options =
                                     InterpretOptions{
                                         .sanitize = true,
                                     },
                             },
                             Options{
                                 .build_options =
                                     BuildOptions{
                                         .optimize_ir_ext = false,
                                         .optimize_ir = true,
                                     },
                                 .interpret_options =
                                     InterpretOptions{
                                         .sanitize = true,
                                     },
                             },
                             Options{
                                 .build_options =
                                     BuildOptions{
                                         .optimize_ir_ext = true,
                                         .optimize_ir = true,
                                     },
                                 .interpret_options =
                                     InterpretOptions{
                                         .sanitize = true,
                                     },
                             }));

TEST_P(InterpretTest, InterpretsSmallProgramCorrectly) {
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
  BuildOptions build_options = GetParam().build_options;
  InterpretOptions interpret_options = GetParam().interpret_options;
  ErrorCode result = Interpret(paths, build_options, interpret_options,
                               DebugHandler::WithDebugEnabledButOutputDisabled(), &ctx);

  EXPECT_EQ(result, 45);
}

TEST_P(InterpretTest, RunsAddressOfLocalVariableCorrectly) {
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
  BuildOptions build_options = GetParam().build_options;
  InterpretOptions interpret_options = GetParam().interpret_options;
  ErrorCode result = Interpret(paths, build_options, interpret_options,
                               DebugHandler::WithDebugEnabledButOutputDisabled(), &ctx);

  EXPECT_EQ(result, 127);
}

}  // namespace cmd
