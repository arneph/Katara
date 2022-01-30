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

TEST(RunTest, RunsSmallProgramCorrectly) {
  TestContext ctx({"test.kat"}, "");
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

  ErrorCode result = ::cmd::Run(&ctx);

  EXPECT_EQ(result, 45);
}

}  // namespace cmd