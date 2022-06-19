//
//  types_test.cc
//  Katara
//
//  Created by Arne Philipeit on 6/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/ir/representation/types.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(TypeEqualityTest, ReturnsCorrectEqualityVerdicts) {
  EXPECT_TRUE(ir::IsEqual(ir::bool_type(), ir::bool_type()));
  EXPECT_TRUE(ir::IsEqual(ir::i8(), ir::i8()));
  EXPECT_TRUE(ir::IsEqual(ir::i16(), ir::i16()));
  EXPECT_TRUE(ir::IsEqual(ir::i32(), ir::i32()));
  EXPECT_TRUE(ir::IsEqual(ir::i64(), ir::i64()));
  EXPECT_TRUE(ir::IsEqual(ir::u8(), ir::u8()));
  EXPECT_TRUE(ir::IsEqual(ir::u16(), ir::u16()));
  EXPECT_TRUE(ir::IsEqual(ir::u32(), ir::u32()));
  EXPECT_TRUE(ir::IsEqual(ir::u64(), ir::u64()));
  EXPECT_TRUE(ir::IsEqual(ir::pointer_type(), ir::pointer_type()));
  EXPECT_TRUE(ir::IsEqual(ir::func_type(), ir::func_type()));

  EXPECT_FALSE(ir::IsEqual(ir::bool_type(), ir::i8()));
  EXPECT_FALSE(ir::IsEqual(ir::bool_type(), ir::i16()));
  EXPECT_FALSE(ir::IsEqual(ir::bool_type(), ir::i32()));
  EXPECT_FALSE(ir::IsEqual(ir::bool_type(), ir::i64()));
  EXPECT_FALSE(ir::IsEqual(ir::bool_type(), ir::u8()));
  EXPECT_FALSE(ir::IsEqual(ir::bool_type(), ir::u16()));
  EXPECT_FALSE(ir::IsEqual(ir::bool_type(), ir::u32()));
  EXPECT_FALSE(ir::IsEqual(ir::bool_type(), ir::u64()));
  EXPECT_FALSE(ir::IsEqual(ir::bool_type(), ir::pointer_type()));
  EXPECT_FALSE(ir::IsEqual(ir::bool_type(), ir::func_type()));

  EXPECT_FALSE(ir::IsEqual(ir::i8(), ir::bool_type()));
  EXPECT_FALSE(ir::IsEqual(ir::i8(), ir::i16()));
  EXPECT_FALSE(ir::IsEqual(ir::i8(), ir::i32()));
  EXPECT_FALSE(ir::IsEqual(ir::i8(), ir::i64()));
  EXPECT_FALSE(ir::IsEqual(ir::i8(), ir::u8()));
  EXPECT_FALSE(ir::IsEqual(ir::i8(), ir::u16()));
  EXPECT_FALSE(ir::IsEqual(ir::i8(), ir::u32()));
  EXPECT_FALSE(ir::IsEqual(ir::i8(), ir::u64()));
  EXPECT_FALSE(ir::IsEqual(ir::i8(), ir::pointer_type()));
  EXPECT_FALSE(ir::IsEqual(ir::i8(), ir::func_type()));

  EXPECT_FALSE(ir::IsEqual(ir::u32(), ir::bool_type()));
  EXPECT_FALSE(ir::IsEqual(ir::u32(), ir::i8()));
  EXPECT_FALSE(ir::IsEqual(ir::u32(), ir::i16()));
  EXPECT_FALSE(ir::IsEqual(ir::u32(), ir::i32()));
  EXPECT_FALSE(ir::IsEqual(ir::u32(), ir::i64()));
  EXPECT_FALSE(ir::IsEqual(ir::u32(), ir::u8()));
  EXPECT_FALSE(ir::IsEqual(ir::u32(), ir::u16()));
  EXPECT_FALSE(ir::IsEqual(ir::u32(), ir::u64()));
  EXPECT_FALSE(ir::IsEqual(ir::u32(), ir::pointer_type()));
  EXPECT_FALSE(ir::IsEqual(ir::u32(), ir::func_type()));

  EXPECT_FALSE(ir::IsEqual(ir::pointer_type(), ir::bool_type()));
  EXPECT_FALSE(ir::IsEqual(ir::pointer_type(), ir::i8()));
  EXPECT_FALSE(ir::IsEqual(ir::pointer_type(), ir::i16()));
  EXPECT_FALSE(ir::IsEqual(ir::pointer_type(), ir::i32()));
  EXPECT_FALSE(ir::IsEqual(ir::pointer_type(), ir::i64()));
  EXPECT_FALSE(ir::IsEqual(ir::pointer_type(), ir::u8()));
  EXPECT_FALSE(ir::IsEqual(ir::pointer_type(), ir::u16()));
  EXPECT_FALSE(ir::IsEqual(ir::pointer_type(), ir::u32()));
  EXPECT_FALSE(ir::IsEqual(ir::pointer_type(), ir::u64()));
  EXPECT_FALSE(ir::IsEqual(ir::pointer_type(), ir::func_type()));

  EXPECT_FALSE(ir::IsEqual(ir::func_type(), ir::bool_type()));
  EXPECT_FALSE(ir::IsEqual(ir::func_type(), ir::i8()));
  EXPECT_FALSE(ir::IsEqual(ir::func_type(), ir::i16()));
  EXPECT_FALSE(ir::IsEqual(ir::func_type(), ir::i32()));
  EXPECT_FALSE(ir::IsEqual(ir::func_type(), ir::i64()));
  EXPECT_FALSE(ir::IsEqual(ir::func_type(), ir::u8()));
  EXPECT_FALSE(ir::IsEqual(ir::func_type(), ir::u16()));
  EXPECT_FALSE(ir::IsEqual(ir::func_type(), ir::u32()));
  EXPECT_FALSE(ir::IsEqual(ir::func_type(), ir::u64()));
  EXPECT_FALSE(ir::IsEqual(ir::func_type(), ir::pointer_type()));
}
