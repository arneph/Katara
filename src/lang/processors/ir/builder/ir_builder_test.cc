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
#include "src/lang/processors/ir/check/check_test_util.h"
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
    %1:lshared_ptr<i64, s> = copy_shared %0, #0:i64
    store %1, #42:i64
    delete_shared %1
    %2:i64 = load %0
    delete_shared %0
    ret %2
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
    %1:lshared_ptr<u16, s> = copy_shared %0, #0:i64
    %2:u16 = conv #65535:i64
    store %1, %2
    delete_shared %1
    %3:lshared_ptr<u16, s> = copy_shared %0, #0:i64
    delete_shared %0
    ret %3
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
    %1:lshared_ptr<lshared_ptr<i32, s>, s> = copy_shared %0, #0:i64
    %2:lshared_ptr<i32, s> = make_shared #1:i64
    store %2, #0:i32
    %3:lshared_ptr<i32, s> = load %1
    delete_shared %3
    store %1, %2
    delete_shared %1
    %4:lshared_ptr<i32, s> = load %0
    %5:lshared_ptr<i32, s> = copy_shared %4, #0:i64
    delete_shared %0
    ret %5
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
    %1:lshared_ptr<lshared_ptr<i32, s>, s> = copy_shared %0, #0:i64
    %2:lshared_ptr<i32, s> = make_shared #1:i64
    store %2, #0:i32
    %3:lshared_ptr<i32, s> = load %1
    delete_shared %3
    store %1, %2
    delete_shared %1
    %4:lshared_ptr<lshared_ptr<i32, s>, s> = make_shared #1:i64
    store %4, 0x0
    %5:lshared_ptr<lshared_ptr<i32, s>, s> = copy_shared %4, #0:i64
    %6:lshared_ptr<i32, s> = load %0
    %7:lshared_ptr<i32, s> = copy_shared %6, #0:i64
    %8:lshared_ptr<i32, s> = load %5
    delete_shared %8
    store %5, %7
    delete_shared %5
    %9:lshared_ptr<i32, s> = load %4
    %10:lshared_ptr<i32, s> = copy_shared %9, #0:i64
    delete_shared %4
    delete_shared %0
    ret %10
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
    %1:lshared_ptr<i64, s> = copy_shared %0, #0:i64
    store %1, #42:i64
    delete_shared %1
    %2:lshared_ptr<i64, s> = copy_shared %0, #0:i64
    %3:i64 = load %2
    delete_shared %2
    delete_shared %0
  ret %3
}
)ir",
                             },
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
package main

func get(a *uint16) uint16 {
  return *a
}
)kat",
                                 .expected_ir_program = R"ir(
@0 get (%0:lshared_ptr<u16, s>) => (u16) {
  {0}
    %1:lshared_ptr<lshared_ptr<u16, s>, s> = make_shared #1:i64
    store %1, 0x0
    store %1, %0
    %2:lshared_ptr<u16, s> = load %1
    %3:lshared_ptr<u16, s> = copy_shared %2, #0:i64
    %4:u16 = load %3
    delete_shared %3
    delete_shared %1
    ret %4
}
)ir",
                             },
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
package main

func set(a *uint16) {
  *a = 123
}
)kat",
                                 .expected_ir_program = R"ir(
@0 set (%0:lshared_ptr<u16, s>) => () {
  {0}
    %1:lshared_ptr<lshared_ptr<u16, s>, s> = make_shared #1:i64
    store %1, 0x0
    store %1, %0
    %2:lshared_ptr<u16, s> = load %1
    %3:lshared_ptr<u16, s> = copy_shared %2, #0:i64
    %4:u16 = conv #123:i64
    store %3, %4
    delete_shared %3
    delete_shared %1
    ret
}
)ir",
                             },
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
package main

func inc(a *uint16) {
  *a = *a + 1
}
)kat",
                                 .expected_ir_program = R"ir(
@0 inc (%0:lshared_ptr<u16, s>) => () {
  {0}
    %1:lshared_ptr<lshared_ptr<u16, s>, s> = make_shared #1:i64
    store %1, 0x0
    store %1, %0
    %2:lshared_ptr<u16, s> = load %1
    %3:lshared_ptr<u16, s> = copy_shared %2, #0:i64
    %4:lshared_ptr<u16, s> = load %1
    %5:lshared_ptr<u16, s> = copy_shared %4, #0:i64
    %6:u16 = load %5
    delete_shared %5
    %7:u16 = conv #1:i64
    %8:u16 = iadd %6, %7
    store %3, %8
    delete_shared %3
    delete_shared %1
    ret
}
)ir",
                             },
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
package main

func inc(a *uint16) {
  *a++
}
)kat",
                                 .expected_ir_program = R"ir(
@0 inc (%0:lshared_ptr<u16, s>) => () {
  {0}
    %1:lshared_ptr<lshared_ptr<u16, s>, s> = make_shared #1:i64
    store %1, 0x0
    store %1, %0
    %2:lshared_ptr<u16, s> = load %1
    %3:lshared_ptr<u16, s> = copy_shared %2, #0:i64
    %4:u16 = load %3
    %5:u16 = iadd %4, #1:u16
    store %3, %5
    delete_shared %3
    delete_shared %1
    ret
}
)ir",
                             },
                             IRBuilderTestParams{
                                 .input_lang_program = R"kat(
package main

func add(a *uint16, b uint16) {
  *a += b
}
)kat",
                                 .expected_ir_program = R"ir(
@0 add (%0:lshared_ptr<u16, s>, %2:u16) => () {
  {0}
    %1:lshared_ptr<lshared_ptr<u16, s>, s> = make_shared #1:i64
    store %1, 0x0
    store %1, %0
    %3:lshared_ptr<u16, s> = make_shared #1:i64
    store %3, #0:u16
    store %3, %2
    %4:lshared_ptr<u16, s> = load %1
    %5:lshared_ptr<u16, s> = copy_shared %4, #0:i64
    %6:u16 = load %3
    %7:u16 = load %5
    %8:u16 = iadd %7, %6
    store %5, %8
    delete_shared %5
    delete_shared %3
    delete_shared %1
    ret
}
)ir",
                             }));

TEST_P(IRBuilderTest, BuildsIR) {
  common::filesystem::TestFilesystem filesystem_;
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
  lang::ir_check::CheckProgramOrDie(actual_ir_program.get());

  // Check IR is as expected:
  std::unique_ptr<ir::Program> expected_ir_program =
      lang::ir_serialization::ParseProgramOrDie(GetParam().expected_ir_program);
  lang::ir_check::CheckProgramOrDie(expected_ir_program.get());
  EXPECT_TRUE(ir::IsEqual(actual_ir_program.get(), expected_ir_program.get()))
      << "For Katara program:" << GetParam().input_lang_program
      << "expected different IR program:\n"
      << ir_serialization::Print(expected_ir_program.get()) << "\ngot:\n"
      << ir_serialization::Print(actual_ir_program.get());
}
