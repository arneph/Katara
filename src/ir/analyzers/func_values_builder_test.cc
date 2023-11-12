//
//  func_values_builder_test.cc
//  Katara
//
//  Created by Arne Philipeit on 11/12/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#include "src/ir/analyzers/func_values_builder.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/info/func_values.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/parse.h"

namespace ir_analyzers {
namespace {

using ::ir_info::FuncValues;
using ::testing::IsEmpty;

TEST(FindValuesInFuncTest, HandlesEmptyFunc) {
  std::unique_ptr<ir::Program> input_program = ir_serialization::ParseProgramOrDie(R"ir(
@0 f() => () {
{0}
  ret
}
)ir");
  const FuncValues func_values = FindValuesInFunc(input_program->GetFunc(0));

  EXPECT_THAT(func_values.GetValues(), IsEmpty());

  EXPECT_THAT(func_values.GetValuesWithType(ir::bool_type()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::u8()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::i64()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::pointer_type()), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithType(ir::func_type()), IsEmpty());

  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kBool), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kInt), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kPointer), IsEmpty());
  EXPECT_THAT(func_values.GetValuesWithTypeKind(ir::TypeKind::kFunc), IsEmpty());

  EXPECT_EQ(func_values.GetInstrDefiningValue(0), nullptr);
  EXPECT_EQ(func_values.GetInstrDefiningValue(1), nullptr);
  EXPECT_EQ(func_values.GetInstrDefiningValue(123), nullptr);

  EXPECT_THAT(func_values.GetInstrsUsingValue(0), IsEmpty());
  EXPECT_THAT(func_values.GetInstrsUsingValue(1), IsEmpty());
  EXPECT_THAT(func_values.GetInstrsUsingValue(123), IsEmpty());
}

}  // namespace
}  // namespace ir_analyzers
