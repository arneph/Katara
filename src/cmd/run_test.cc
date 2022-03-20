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
                                 .optimize_ir = false,
                             },
                             BuildOptions{
                                 .optimize_ir = true,
                             }));

TEST_P(RunTest, RunsSmallProgramCorrectly) {
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

}  // namespace cmd
