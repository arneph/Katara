//
//  ir_builder_test.cc
//  Katara
//
//  Created by Arne Philipeit on 10/22/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "ir_builder.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/common/filesystem/filesystem.h"
#include "src/common/filesystem/test_filesystem.h"
#include "src/ir/serialization/print.h"
#include "src/lang/processors/ir/builder/ir_builder.h"
#include "src/lang/processors/ir/checker/checker.h"
#include "src/lang/processors/ir/serialization/parse.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/processors/packages/package_manager.h"

struct IRBuilderTestParams {
  std::string input_lang_program;
  std::string expected_ir_program;
};

class IRBuilderTest : public testing::TestWithParam<IRBuilderTestParams> {};

INSTANTIATE_TEST_SUITE_P(IRBuilderTestInstance, IRBuilderTest,
                         testing::Values(
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
  package main
)kat",
                                 .expected_ir_program = R"ir(
)ir",
                             },
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
package main

func main() int {
  return 0
}
)kat",
                                 .expected_ir_program = R"ir(
@0 main () => (i64) {
  {0}
    ret #0:i64
}
)ir",
                             },
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
package main

func f() {
  var x int
}
)kat",
                                 .expected_ir_program = R"ir(
@0 f () => () {
  {0}
    %0:lshared_ptr<i64, s> = make_shared #1:i64
    store %0, #0:i64
    delete_shared %0
    ret
}
)ir",
                             },
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
package main

func main() int {
  x := 42
  return x
}
)kat",
                                 .expected_ir_program = R"ir(
@0 main () => (i64) {
  {0}
    %0:lshared_ptr<i64, s> = make_shared #1:i64
    store %0, #0:i64
    store %0, #42:i64
    %1:i64 = load %0
    delete_shared %0
    ret %1
}
)ir",
                             },
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
package main

func a() *uint16 {
  var x uint16 = 65535
  return &x
}
)kat",
                                 .expected_ir_program = R"ir(
@0 a () => (lshared_ptr<u16, s>) {
  {0}
    %0:lshared_ptr<u16, s> = make_shared #1:i64
    store %0, #0:u16
    %1:u16 = conv #65535:i64
    store %0, %1
    %2:lshared_ptr<u16, s> = copy_shared %0, #0:i64
    delete_shared %0
    ret %2
}
)ir",
                             },
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
package main

func a() *int32 {
  x := new<int32>()
  return x
}
)kat",
                                 .expected_ir_program = R"ir(
@0 a () => (lshared_ptr<i32, s>) {
  {0}
    %0:lshared_ptr<lshared_ptr<i32, s>, s> = make_shared #1:i64
    store %0, 0x0
    %1:lshared_ptr<i32, s> = make_shared #1:i64
    store %1, #0:i32
    %2:lshared_ptr<i32, s> = load %0
    delete_shared %2
    store %0, %1
    %3:lshared_ptr<i32, s> = load %0
    %4:lshared_ptr<i32, s> = copy_shared %3, #0:i64
    delete_shared %0
    ret %4
}
)ir",
                             },
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
package main

func a() *int32 {
  x := new<int32>()
  y := x
  return y
}
)kat",
                                 .expected_ir_program = R"ir(
@0 a () => (lshared_ptr<i32, s>) {
  {0}
    %0:lshared_ptr<lshared_ptr<i32, s>, s> = make_shared #1:i64
    store %0, 0x0
    %1:lshared_ptr<i32, s> = make_shared #1:i64
    store %1, #0:i32
    %2:lshared_ptr<i32, s> = load %0
    delete_shared %2
    store %0, %1
    %3:lshared_ptr<lshared_ptr<i32, s>, s> = make_shared #1:i64
    store %3, 0x0
    %4:lshared_ptr<i32, s> = load %0
    %5:lshared_ptr<i32, s> = copy_shared %4, #0:i64
    %6:lshared_ptr<i32, s> = load %3
    delete_shared %6
    store %3, %5
    %7:lshared_ptr<i32, s> = load %3
    %8:lshared_ptr<i32, s> = copy_shared %7, #0:i64
    delete_shared %3
    delete_shared %0
    ret %8
}
)ir",
                             },
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
package main

func a() uint8 {
  return *new<uint8>()
}
)kat",
                                 .expected_ir_program = R"ir(
@0 a () => (u8) {
  {0}
    %0:lshared_ptr<u8, s> = make_shared #1:i64
    store %0, #0:u8
    %1:u8 = load %0
    delete_shared %0
    ret %1
}
)ir",
                             },
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
package main

func a() int {
  var x = 42
  return *&x
}
)kat",
                                 .expected_ir_program = R"ir(
@0 a () => (i64) {
  {0}
    %0:lshared_ptr<i64, s> = make_shared #1:i64
    store %0, #0:i64
    store %0, #42:i64
    %1:lshared_ptr<i64, s> = copy_shared %0, #0:i64
    %2:i64 = load %1
    delete_shared %1
    delete_shared %0
    ret %2
}
)ir",
                             }));

TEST_P(IRBuilderTest, BuildsIR) {
  common::TestFilesystem filesystem_;
  filesystem_.WriteContentsOfFile("main.kat", GetParam().input_lang_program);
  lang::packages::PackageManager pkg_manager(&filesystem_, /*stdlib_path=*/"", /*src_path=*/"");

  // Load main package:
  lang::packages::Package* pkg = pkg_manager.LoadMainPackage("/");
  EXPECT_TRUE(pkg_manager.issue_tracker()->issues().empty());
  EXPECT_TRUE(pkg != nullptr);
  EXPECT_TRUE(pkg->issue_tracker().issues().empty());

  // Build IR:
  std::unique_ptr<ir::Program> actual_ir_program =
      lang::ir_builder::IRBuilder::TranslateProgram(pkg, pkg_manager.type_info());
  EXPECT_TRUE(actual_ir_program != nullptr);
  //  EXPECT_TRUE(false) << ir_serialization::Print(actual_ir_program.get());
  lang::ir_checker::AssertProgramIsOkay(actual_ir_program.get());

  // Check IR is as expected:
  std::unique_ptr<ir::Program> expected_ir_program =
      lang::ir_serialization::ParseProgram(GetParam().expected_ir_program);
  lang::ir_checker::AssertProgramIsOkay(expected_ir_program.get());
  EXPECT_TRUE(ir::IsEqual(actual_ir_program.get(), expected_ir_program.get()))
      << "For Katara program:" << GetParam().input_lang_program
      << "expected different IR program:\n"
      << ir_serialization::Print(expected_ir_program.get()) << "\ngot:\n"
      << ir_serialization::Print(actual_ir_program.get());
}
