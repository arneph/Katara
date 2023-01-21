//
//  atomics_test.cc
//  Katara
//
//  Created by Arne Philipeit on 6/27/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "src/common/atomics/atomics.h"

#include <array>
#include <iostream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace common::atomics {

using ::testing::AllOf;
using ::testing::Optional;
using ::testing::Property;

TEST(IntTest, Type) {
  EXPECT_EQ(IntType::kI8, Int(int8_t{42}).type());
  EXPECT_EQ(IntType::kI16, Int(int16_t{42}).type());
  EXPECT_EQ(IntType::kI32, Int(int32_t{42}).type());
  EXPECT_EQ(IntType::kI64, Int(int64_t{42}).type());
  EXPECT_EQ(IntType::kU8, Int(uint8_t{42}).type());
  EXPECT_EQ(IntType::kU16, Int(uint16_t{42}).type());
  EXPECT_EQ(IntType::kU32, Int(uint32_t{42}).type());
  EXPECT_EQ(IntType::kU64, Int(uint64_t{42}).type());
}

TEST(IntTest, IsZero) {
  for (int64_t num : std::array<int64_t, 6>{1, 2, 42, -42, -2, -1}) {
    EXPECT_FALSE(Int(int8_t(num)).IsZero());
    EXPECT_FALSE(Int(int16_t(num)).IsZero());
    EXPECT_FALSE(Int(int32_t(num)).IsZero());
    EXPECT_FALSE(Int(int64_t(num)).IsZero());
  }
  for (int64_t num : std::array<int64_t, 3>{1, 2, 42}) {
    EXPECT_FALSE(Int(uint8_t(num)).IsZero());
    EXPECT_FALSE(Int(uint16_t(num)).IsZero());
    EXPECT_FALSE(Int(uint32_t(num)).IsZero());
    EXPECT_FALSE(Int(uint64_t(num)).IsZero());
  }
  EXPECT_TRUE(Int(int8_t{0}).IsZero());
  EXPECT_TRUE(Int(int16_t{0}).IsZero());
  EXPECT_TRUE(Int(int32_t{0}).IsZero());
  EXPECT_TRUE(Int(int64_t{0}).IsZero());
  EXPECT_TRUE(Int(uint8_t{0}).IsZero());
  EXPECT_TRUE(Int(uint16_t{0}).IsZero());
  EXPECT_TRUE(Int(uint32_t{0}).IsZero());
  EXPECT_TRUE(Int(uint64_t{0}).IsZero());
}

TEST(IntTest, IsOne) {
  for (int64_t num : std::array<int64_t, 6>{0, 2, 42, -42, -2, -1}) {
    EXPECT_FALSE(Int(int8_t(num)).IsOne());
    EXPECT_FALSE(Int(int16_t(num)).IsOne());
    EXPECT_FALSE(Int(int32_t(num)).IsOne());
    EXPECT_FALSE(Int(int64_t(num)).IsOne());
  }
  for (int64_t num : std::array<int64_t, 3>{0, 2, 42}) {
    EXPECT_FALSE(Int(uint8_t(num)).IsOne());
    EXPECT_FALSE(Int(uint16_t(num)).IsOne());
    EXPECT_FALSE(Int(uint32_t(num)).IsOne());
    EXPECT_FALSE(Int(uint64_t(num)).IsOne());
  }
  EXPECT_TRUE(Int(int8_t{1}).IsOne());
  EXPECT_TRUE(Int(int16_t{1}).IsOne());
  EXPECT_TRUE(Int(int32_t{1}).IsOne());
  EXPECT_TRUE(Int(int64_t{1}).IsOne());
  EXPECT_TRUE(Int(uint8_t{1}).IsOne());
  EXPECT_TRUE(Int(uint16_t{1}).IsOne());
  EXPECT_TRUE(Int(uint32_t{1}).IsOne());
  EXPECT_TRUE(Int(uint64_t{1}).IsOne());
}

TEST(IntTest, IsMinusOne) {
  for (int64_t num : std::array<int64_t, 6>{0, 1, 2, 42, -42, -2}) {
    EXPECT_FALSE(Int(int8_t(num)).IsMinusOne());
    EXPECT_FALSE(Int(int16_t(num)).IsMinusOne());
    EXPECT_FALSE(Int(int32_t(num)).IsMinusOne());
    EXPECT_FALSE(Int(int64_t(num)).IsMinusOne());
  }
  for (int64_t num : std::array<int64_t, 4>{0, 1, 2, 42}) {
    EXPECT_FALSE(Int(uint8_t(num)).IsMinusOne());
    EXPECT_FALSE(Int(uint16_t(num)).IsMinusOne());
    EXPECT_FALSE(Int(uint32_t(num)).IsMinusOne());
    EXPECT_FALSE(Int(uint64_t(num)).IsMinusOne());
  }
  EXPECT_TRUE(Int(int8_t{-1}).IsMinusOne());
  EXPECT_TRUE(Int(int16_t{-1}).IsMinusOne());
  EXPECT_TRUE(Int(int32_t{-1}).IsMinusOne());
  EXPECT_TRUE(Int(int64_t{-1}).IsMinusOne());
}

TEST(IntTest, IsMin) {
  for (int64_t num : std::array<int64_t, 7>{0, 1, 2, 42, -42, -2, -1}) {
    EXPECT_FALSE(Int(int8_t(num)).IsMin());
    EXPECT_FALSE(Int(int16_t(num)).IsMin());
    EXPECT_FALSE(Int(int32_t(num)).IsMin());
    EXPECT_FALSE(Int(int64_t(num)).IsMin());
  }
  for (int64_t num : std::array<int64_t, 3>{1, 2, 42}) {
    EXPECT_FALSE(Int(uint8_t(num)).IsMin());
    EXPECT_FALSE(Int(uint16_t(num)).IsMin());
    EXPECT_FALSE(Int(uint32_t(num)).IsMin());
    EXPECT_FALSE(Int(uint64_t(num)).IsMin());
  }
  EXPECT_TRUE(Int(int8_t{-128}).IsMin());
  EXPECT_TRUE(Int(int16_t{-32768}).IsMin());
  EXPECT_TRUE(Int(int32_t{-2147483648}).IsMin());
  EXPECT_TRUE(Int(int64_t{-9223372036854775807 - 1}).IsMin());
  EXPECT_TRUE(Int(uint8_t{0}).IsMin());
  EXPECT_TRUE(Int(uint16_t{0}).IsMin());
  EXPECT_TRUE(Int(uint32_t{0}).IsMin());
  EXPECT_TRUE(Int(uint64_t{0}).IsMin());
}

TEST(IntTest, IsMax) {
  for (int64_t num : std::array<int64_t, 7>{0, 1, 2, 42, -42, -2, -1}) {
    EXPECT_FALSE(Int(int8_t(num)).IsMax());
    EXPECT_FALSE(Int(int16_t(num)).IsMax());
    EXPECT_FALSE(Int(int32_t(num)).IsMax());
    EXPECT_FALSE(Int(int64_t(num)).IsMax());
  }
  for (int64_t num : std::array<int64_t, 4>{0, 1, 2, 42}) {
    EXPECT_FALSE(Int(uint8_t(num)).IsMax());
    EXPECT_FALSE(Int(uint16_t(num)).IsMax());
    EXPECT_FALSE(Int(uint32_t(num)).IsMax());
    EXPECT_FALSE(Int(uint64_t(num)).IsMax());
  }
  EXPECT_TRUE(Int(int8_t{127}).IsMax());
  EXPECT_TRUE(Int(int16_t{32767}).IsMax());
  EXPECT_TRUE(Int(int32_t{2147483647}).IsMax());
  EXPECT_TRUE(Int(int64_t{9223372036854775807}).IsMax());
  EXPECT_TRUE(Int(uint8_t{255}).IsMax());
  EXPECT_TRUE(Int(uint16_t{65535}).IsMax());
  EXPECT_TRUE(Int(uint32_t{4294967295}).IsMax());
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicitly-unsigned-literal"
  EXPECT_TRUE(Int(uint64_t{18446744073709551615}).IsMax());
#pragma clang diagnostic pop
}

TEST(IntTest, IsLessThanZero) {
  for (int64_t num : std::array<int64_t, 4>{0, 1, 2, 42}) {
    EXPECT_FALSE(Int(int8_t(num)).IsLessThanZero());
    EXPECT_FALSE(Int(int16_t(num)).IsLessThanZero());
    EXPECT_FALSE(Int(int32_t(num)).IsLessThanZero());
    EXPECT_FALSE(Int(int64_t(num)).IsLessThanZero());
    EXPECT_FALSE(Int(uint8_t(num)).IsLessThanZero());
    EXPECT_FALSE(Int(uint16_t(num)).IsLessThanZero());
    EXPECT_FALSE(Int(uint32_t(num)).IsLessThanZero());
    EXPECT_FALSE(Int(uint64_t(num)).IsLessThanZero());
  }
  for (int64_t num : std::array<int64_t, 3>{-1, -2, -42}) {
    EXPECT_TRUE(Int(int8_t(num)).IsLessThanZero());
    EXPECT_TRUE(Int(int16_t(num)).IsLessThanZero());
    EXPECT_TRUE(Int(int32_t(num)).IsLessThanZero());
    EXPECT_TRUE(Int(int64_t(num)).IsLessThanZero());
  }
}

TEST(IntTest, IsLessThanOrEqualToZero) {
  for (int64_t num : std::array<int64_t, 3>{1, 2, 42}) {
    EXPECT_FALSE(Int(int8_t(num)).IsLessThanOrEqualToZero());
    EXPECT_FALSE(Int(int16_t(num)).IsLessThanOrEqualToZero());
    EXPECT_FALSE(Int(int32_t(num)).IsLessThanOrEqualToZero());
    EXPECT_FALSE(Int(int64_t(num)).IsLessThanOrEqualToZero());
    EXPECT_FALSE(Int(uint8_t(num)).IsLessThanOrEqualToZero());
    EXPECT_FALSE(Int(uint16_t(num)).IsLessThanOrEqualToZero());
    EXPECT_FALSE(Int(uint32_t(num)).IsLessThanOrEqualToZero());
    EXPECT_FALSE(Int(uint64_t(num)).IsLessThanOrEqualToZero());
  }
  for (int64_t num : std::array<int64_t, 4>{0, -1, -2, -42}) {
    EXPECT_TRUE(Int(int8_t(num)).IsLessThanOrEqualToZero());
    EXPECT_TRUE(Int(int16_t(num)).IsLessThanOrEqualToZero());
    EXPECT_TRUE(Int(int32_t(num)).IsLessThanOrEqualToZero());
    EXPECT_TRUE(Int(int64_t(num)).IsLessThanOrEqualToZero());
  }
  EXPECT_TRUE(Int(uint8_t{0}).IsLessThanOrEqualToZero());
  EXPECT_TRUE(Int(uint16_t{0}).IsLessThanOrEqualToZero());
  EXPECT_TRUE(Int(uint32_t{0}).IsLessThanOrEqualToZero());
  EXPECT_TRUE(Int(uint64_t{0}).IsLessThanOrEqualToZero());
}

TEST(IntTest, IsGreaterThanOrEqualToZero) {
  for (int64_t num : std::array<int64_t, 4>{0, 1, 2, 42}) {
    EXPECT_TRUE(Int(int8_t(num)).IsGreaterThanOrEqualToZero());
    EXPECT_TRUE(Int(int16_t(num)).IsGreaterThanOrEqualToZero());
    EXPECT_TRUE(Int(int32_t(num)).IsGreaterThanOrEqualToZero());
    EXPECT_TRUE(Int(int64_t(num)).IsGreaterThanOrEqualToZero());
    EXPECT_TRUE(Int(uint8_t(num)).IsGreaterThanOrEqualToZero());
    EXPECT_TRUE(Int(uint16_t(num)).IsGreaterThanOrEqualToZero());
    EXPECT_TRUE(Int(uint32_t(num)).IsGreaterThanOrEqualToZero());
    EXPECT_TRUE(Int(uint64_t(num)).IsGreaterThanOrEqualToZero());
  }
  for (int64_t num : std::array<int64_t, 3>{-1, -2, -42}) {
    EXPECT_FALSE(Int(int8_t(num)).IsGreaterThanOrEqualToZero());
    EXPECT_FALSE(Int(int16_t(num)).IsGreaterThanOrEqualToZero());
    EXPECT_FALSE(Int(int32_t(num)).IsGreaterThanOrEqualToZero());
    EXPECT_FALSE(Int(int64_t(num)).IsGreaterThanOrEqualToZero());
  }
}

TEST(IntTest, IsGreaterThanZero) {
  for (int64_t num : std::array<int64_t, 3>{1, 2, 42}) {
    EXPECT_TRUE(Int(int8_t(num)).IsGreaterThanZero());
    EXPECT_TRUE(Int(int16_t(num)).IsGreaterThanZero());
    EXPECT_TRUE(Int(int32_t(num)).IsGreaterThanZero());
    EXPECT_TRUE(Int(int64_t(num)).IsGreaterThanZero());
    EXPECT_TRUE(Int(uint8_t(num)).IsGreaterThanZero());
    EXPECT_TRUE(Int(uint16_t(num)).IsGreaterThanZero());
    EXPECT_TRUE(Int(uint32_t(num)).IsGreaterThanZero());
    EXPECT_TRUE(Int(uint64_t(num)).IsGreaterThanZero());
  }
  for (int64_t num : std::array<int64_t, 4>{0, -1, -2, -42}) {
    EXPECT_FALSE(Int(int8_t(num)).IsGreaterThanZero());
    EXPECT_FALSE(Int(int16_t(num)).IsGreaterThanZero());
    EXPECT_FALSE(Int(int32_t(num)).IsGreaterThanZero());
    EXPECT_FALSE(Int(int64_t(num)).IsGreaterThanZero());
  }
  EXPECT_FALSE(Int(uint8_t{0}).IsGreaterThanZero());
  EXPECT_FALSE(Int(uint16_t{0}).IsGreaterThanZero());
  EXPECT_FALSE(Int(uint32_t{0}).IsGreaterThanZero());
  EXPECT_FALSE(Int(uint64_t{0}).IsGreaterThanZero());
}

TEST(IntTest, HandlesInt64Conversion) {
  for (int64_t num : std::array<int64_t, 7>{0, 1, 2, 42, -42, -2, -1}) {
    EXPECT_TRUE(Int(int8_t(num)).IsRepresentableAsInt64());
    EXPECT_EQ(num, Int(int8_t(num)).AsInt64());
    EXPECT_TRUE(Int(int16_t(num)).IsRepresentableAsInt64());
    EXPECT_EQ(num, Int(int16_t(num)).AsInt64());
    EXPECT_TRUE(Int(int32_t(num)).IsRepresentableAsInt64());
    EXPECT_EQ(num, Int(int32_t(num)).AsInt64());
    EXPECT_TRUE(Int(int64_t(num)).IsRepresentableAsInt64());
    EXPECT_EQ(num, Int(int64_t(num)).AsInt64());
  }
  for (int64_t num : std::array<int64_t, 4>{0, 1, 2, 42}) {
    EXPECT_TRUE(Int(uint8_t(num)).IsRepresentableAsInt64());
    EXPECT_EQ(num, Int(uint8_t(num)).AsInt64());
    EXPECT_TRUE(Int(uint16_t(num)).IsRepresentableAsInt64());
    EXPECT_EQ(num, Int(uint16_t(num)).AsInt64());
    EXPECT_TRUE(Int(uint32_t(num)).IsRepresentableAsInt64());
    EXPECT_EQ(num, Int(uint32_t(num)).AsInt64());
    EXPECT_TRUE(Int(uint64_t(num)).IsRepresentableAsInt64());
    EXPECT_EQ(num, Int(uint64_t(num)).AsInt64());
  }
  EXPECT_TRUE(Int(int64_t{-9223372036854775807 - 1}).IsRepresentableAsInt64());
  EXPECT_TRUE(Int(uint64_t{9223372036854775807}).IsRepresentableAsInt64());
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicitly-unsigned-literal"
  EXPECT_FALSE(Int(uint64_t{9223372036854775808}).IsRepresentableAsInt64());
  EXPECT_FALSE(Int(uint64_t{18446744073709551615}).IsRepresentableAsInt64());
#pragma clang diagnostic pop
}

TEST(IntTest, HandlesUint64Conversion) {
  for (int64_t num : std::array<int64_t, 4>{0, 1, 2, 42}) {
    EXPECT_TRUE(Int(int8_t(num)).IsRepresentableAsUint64());
    EXPECT_EQ(num, Int(int8_t(num)).AsUint64());
    EXPECT_TRUE(Int(int16_t(num)).IsRepresentableAsUint64());
    EXPECT_EQ(num, Int(int16_t(num)).AsUint64());
    EXPECT_TRUE(Int(int32_t(num)).IsRepresentableAsUint64());
    EXPECT_EQ(num, Int(int32_t(num)).AsUint64());
    EXPECT_TRUE(Int(int64_t(num)).IsRepresentableAsUint64());
    EXPECT_EQ(num, Int(uint64_t(num)).AsUint64());
    EXPECT_TRUE(Int(uint8_t(num)).IsRepresentableAsUint64());
    EXPECT_EQ(num, Int(uint8_t(num)).AsUint64());
    EXPECT_TRUE(Int(uint16_t(num)).IsRepresentableAsUint64());
    EXPECT_EQ(num, Int(uint16_t(num)).AsUint64());
    EXPECT_TRUE(Int(uint32_t(num)).IsRepresentableAsUint64());
    EXPECT_EQ(num, Int(uint32_t(num)).AsUint64());
    EXPECT_TRUE(Int(uint64_t(num)).IsRepresentableAsUint64());
    EXPECT_EQ(num, Int(uint64_t(num)).AsUint64());
  }
  for (int64_t num : std::array<int64_t, 3>{-1, -2, -42}) {
    EXPECT_FALSE(Int(int8_t(num)).IsRepresentableAsUint64());
    EXPECT_FALSE(Int(int16_t(num)).IsRepresentableAsUint64());
    EXPECT_FALSE(Int(int32_t(num)).IsRepresentableAsUint64());
    EXPECT_FALSE(Int(int64_t(num)).IsRepresentableAsUint64());
  }
  EXPECT_FALSE(Int(int64_t{-9223372036854775807 - 1}).IsRepresentableAsUint64());
  EXPECT_TRUE(Int(uint64_t{9223372036854775807}).IsRepresentableAsUint64());
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicitly-unsigned-literal"
  EXPECT_TRUE(Int(uint64_t{9223372036854775808}).IsRepresentableAsUint64());
  EXPECT_TRUE(Int(uint64_t{18446744073709551615}).IsRepresentableAsUint64());
#pragma clang diagnostic pop
}

TEST(IntTest, HandlesConversion) {
  for (IntType result_type :
       std::array<IntType, 4>{IntType::kI8, IntType::kI16, IntType::kI32, IntType::kI64}) {
    for (int64_t num : std::array<int64_t, 7>{0, 1, 2, 42, -42, -2, -1}) {
      EXPECT_TRUE(Int(int8_t(num)).CanConvertTo(result_type));
      EXPECT_EQ(result_type, Int(int8_t(num)).ConvertTo(result_type).type());
      EXPECT_EQ(num, Int(int8_t(num)).ConvertTo(result_type).AsInt64());

      EXPECT_TRUE(Int(int16_t(num)).CanConvertTo(result_type));
      EXPECT_EQ(result_type, Int(int16_t(num)).ConvertTo(result_type).type());
      EXPECT_EQ(num, Int(int16_t(num)).ConvertTo(result_type).AsInt64());

      EXPECT_TRUE(Int(int32_t(num)).CanConvertTo(result_type));
      EXPECT_EQ(result_type, Int(int32_t(num)).ConvertTo(result_type).type());
      EXPECT_EQ(num, Int(int32_t(num)).ConvertTo(result_type).AsInt64());

      EXPECT_TRUE(Int(int64_t(num)).CanConvertTo(result_type));
      EXPECT_EQ(result_type, Int(int64_t(num)).ConvertTo(result_type).type());
      EXPECT_EQ(num, Int(int64_t(num)).ConvertTo(result_type).AsInt64());
    }
  }
  for (IntType result_type :
       std::array<IntType, 4>{IntType::kU8, IntType::kU16, IntType::kU32, IntType::kU64}) {
    for (int64_t num : std::array<int64_t, 4>{0, 1, 2, 42}) {
      EXPECT_TRUE(Int(int8_t(num)).CanConvertTo(result_type));
      EXPECT_EQ(result_type, Int(int8_t(num)).ConvertTo(result_type).type());
      EXPECT_EQ(num, Int(int8_t(num)).ConvertTo(result_type).AsInt64());

      EXPECT_TRUE(Int(int16_t(num)).CanConvertTo(result_type));
      EXPECT_EQ(result_type, Int(int16_t(num)).ConvertTo(result_type).type());
      EXPECT_EQ(num, Int(int16_t(num)).ConvertTo(result_type).AsInt64());

      EXPECT_TRUE(Int(int32_t(num)).CanConvertTo(result_type));
      EXPECT_EQ(result_type, Int(int32_t(num)).ConvertTo(result_type).type());
      EXPECT_EQ(num, Int(int32_t(num)).ConvertTo(result_type).AsInt64());

      EXPECT_TRUE(Int(int64_t(num)).CanConvertTo(result_type));
      EXPECT_EQ(result_type, Int(int64_t(num)).ConvertTo(result_type).type());
      EXPECT_EQ(num, Int(int64_t(num)).ConvertTo(result_type).AsInt64());

      EXPECT_TRUE(Int(uint8_t(num)).CanConvertTo(result_type));
      EXPECT_EQ(result_type, Int(uint8_t(num)).ConvertTo(result_type).type());
      EXPECT_EQ(num, Int(uint8_t(num)).ConvertTo(result_type).AsInt64());

      EXPECT_TRUE(Int(uint16_t(num)).CanConvertTo(result_type));
      EXPECT_EQ(result_type, Int(uint16_t(num)).ConvertTo(result_type).type());
      EXPECT_EQ(num, Int(uint16_t(num)).ConvertTo(result_type).AsInt64());

      EXPECT_TRUE(Int(uint32_t(num)).CanConvertTo(result_type));
      EXPECT_EQ(result_type, Int(uint32_t(num)).ConvertTo(result_type).type());
      EXPECT_EQ(num, Int(uint32_t(num)).ConvertTo(result_type).AsInt64());

      EXPECT_TRUE(Int(uint64_t(num)).CanConvertTo(result_type));
      EXPECT_EQ(result_type, Int(uint64_t(num)).ConvertTo(result_type).type());
      EXPECT_EQ(num, Int(uint64_t(num)).ConvertTo(result_type).AsInt64());
    }
  }
  for (IntType result_type :
       std::array<IntType, 4>{IntType::kU8, IntType::kU16, IntType::kU32, IntType::kU64}) {
    for (int64_t num : std::array<int64_t, 3>{-42, -2, -1}) {
      EXPECT_FALSE(Int(int8_t(num)).CanConvertTo(result_type));
      EXPECT_FALSE(Int(int16_t(num)).CanConvertTo(result_type));
      EXPECT_FALSE(Int(int32_t(num)).CanConvertTo(result_type));
      EXPECT_FALSE(Int(int64_t(num)).CanConvertTo(result_type));
    }
  }

  // I8 and U8 limits:
  EXPECT_FALSE(Int(int16_t{128}).CanConvertTo(IntType::kI8));
  EXPECT_FALSE(Int(uint16_t{128}).CanConvertTo(IntType::kI8));
  EXPECT_FALSE(Int(int16_t{-129}).CanConvertTo(IntType::kI8));
  EXPECT_FALSE(Int(int16_t{256}).CanConvertTo(IntType::kU8));
  EXPECT_FALSE(Int(uint16_t{256}).CanConvertTo(IntType::kU8));

  EXPECT_FALSE(Int(int32_t{128}).CanConvertTo(IntType::kI8));
  EXPECT_FALSE(Int(uint32_t{128}).CanConvertTo(IntType::kI8));
  EXPECT_FALSE(Int(int32_t{-129}).CanConvertTo(IntType::kI8));
  EXPECT_FALSE(Int(int32_t{256}).CanConvertTo(IntType::kU8));
  EXPECT_FALSE(Int(uint32_t{256}).CanConvertTo(IntType::kU8));

  EXPECT_FALSE(Int(int64_t{128}).CanConvertTo(IntType::kI8));
  EXPECT_FALSE(Int(uint64_t{128}).CanConvertTo(IntType::kI8));
  EXPECT_FALSE(Int(int64_t{-129}).CanConvertTo(IntType::kI8));
  EXPECT_FALSE(Int(int64_t{256}).CanConvertTo(IntType::kU8));
  EXPECT_FALSE(Int(uint64_t{256}).CanConvertTo(IntType::kU8));

  // I16 and U16 limits:
  for (IntType signed_int_type : std::array<IntType, 3>{IntType::kI8, IntType::kI16}) {
    IntType unsigned_int_type = ToUnsigned(signed_int_type);

    EXPECT_FALSE(Int(int32_t{32768}).CanConvertTo(signed_int_type));
    EXPECT_FALSE(Int(uint32_t{32768}).CanConvertTo(signed_int_type));
    EXPECT_FALSE(Int(int32_t{-32769}).CanConvertTo(signed_int_type));
    EXPECT_FALSE(Int(int32_t{65536}).CanConvertTo(unsigned_int_type));
    EXPECT_FALSE(Int(uint32_t{65536}).CanConvertTo(unsigned_int_type));

    EXPECT_FALSE(Int(int64_t{32768}).CanConvertTo(signed_int_type));
    EXPECT_FALSE(Int(uint64_t{32768}).CanConvertTo(signed_int_type));
    EXPECT_FALSE(Int(int64_t{-32769}).CanConvertTo(signed_int_type));
    EXPECT_FALSE(Int(int64_t{65536}).CanConvertTo(unsigned_int_type));
    EXPECT_FALSE(Int(uint64_t{65536}).CanConvertTo(unsigned_int_type));
  }

  // I32 and U32 limits:
  for (IntType signed_int_type :
       std::array<IntType, 3>{IntType::kI8, IntType::kI16, IntType::kI32}) {
    IntType unsigned_int_type = ToUnsigned(signed_int_type);
    EXPECT_FALSE(Int(int64_t{2147483648}).CanConvertTo(signed_int_type));
    EXPECT_FALSE(Int(uint64_t{2147483648}).CanConvertTo(signed_int_type));
    EXPECT_FALSE(Int(int64_t{-2147483649}).CanConvertTo(signed_int_type));
    EXPECT_FALSE(Int(int64_t{4294967296}).CanConvertTo(unsigned_int_type));
    EXPECT_FALSE(Int(uint64_t{4294967296}).CanConvertTo(unsigned_int_type));
  }

  // I64 and U64 limits:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicitly-unsigned-literal"
  EXPECT_FALSE(Int(uint64_t{9223372036854775808}).CanConvertTo(IntType::kI64));
  EXPECT_FALSE(Int(uint64_t{18446744073709551615}).CanConvertTo(IntType::kI64));
  EXPECT_FALSE(Int(int64_t{-9223372036854775807 - 1}).CanConvertTo(IntType::kU64));
#pragma clang diagnostic pop
}

TEST(IntTest, HandlesNegUnaryOp) {
  struct TestCase {
    int64_t num;
    int64_t neg;
  };
  for (auto [num, neg] :
       std::array<TestCase, 7>{TestCase{0, 0}, TestCase{1, -1}, TestCase{2, -2}, TestCase{42, -42},
                               TestCase{-42, 42}, TestCase{-2, 2}, TestCase{-1, 1}}) {
    EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int8_t(num))));
    EXPECT_EQ(IntType::kI8, Int::Compute(Int::UnaryOp::kNeg, Int(int8_t(num))).type());
    EXPECT_EQ(neg, Int::Compute(Int::UnaryOp::kNeg, Int(int8_t(num))).AsInt64());
    EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int16_t(num))));
    EXPECT_EQ(IntType::kI16, Int::Compute(Int::UnaryOp::kNeg, Int(int16_t(num))).type());
    EXPECT_EQ(neg, Int::Compute(Int::UnaryOp::kNeg, Int(int16_t(num))).AsInt64());
    EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int32_t(num))));
    EXPECT_EQ(IntType::kI32, Int::Compute(Int::UnaryOp::kNeg, Int(int32_t(num))).type());
    EXPECT_EQ(neg, Int::Compute(Int::UnaryOp::kNeg, Int(int32_t(num))).AsInt64());
    EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int64_t(num))));
    EXPECT_EQ(IntType::kI64, Int::Compute(Int::UnaryOp::kNeg, Int(int64_t(num))).type());
    EXPECT_EQ(neg, Int::Compute(Int::UnaryOp::kNeg, Int(int64_t(num))).AsInt64());
  }
  for (int64_t num : std::array<int64_t, 4>{0, 1, 2, 42}) {
    EXPECT_FALSE(Int::CanCompute(Int::UnaryOp::kNeg, Int(uint8_t(num))));
    EXPECT_FALSE(Int::CanCompute(Int::UnaryOp::kNeg, Int(uint16_t(num))));
    EXPECT_FALSE(Int::CanCompute(Int::UnaryOp::kNeg, Int(uint32_t(num))));
    EXPECT_FALSE(Int::CanCompute(Int::UnaryOp::kNeg, Int(uint64_t(num))));
  }
  EXPECT_FALSE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int8_t{-128})));
  EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int8_t{-127})));
  EXPECT_EQ(127, Int::Compute(Int::UnaryOp::kNeg, Int(int8_t{-127})).AsInt64());
  EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int8_t{127})));
  EXPECT_EQ(-127, Int::Compute(Int::UnaryOp::kNeg, Int(int8_t{127})).AsInt64());

  EXPECT_FALSE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int16_t{-32768})));
  EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int16_t{-32767})));
  EXPECT_EQ(32767, Int::Compute(Int::UnaryOp::kNeg, Int(int16_t{-32767})).AsInt64());
  EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int16_t{32767})));
  EXPECT_EQ(-32767, Int::Compute(Int::UnaryOp::kNeg, Int(int16_t{32767})).AsInt64());

  EXPECT_FALSE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int32_t{-2147483648})));
  EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int32_t{-2147483647})));
  EXPECT_EQ(2147483647, Int::Compute(Int::UnaryOp::kNeg, Int(int32_t{-2147483647})).AsInt64());
  EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int32_t{2147483647})));
  EXPECT_EQ(-2147483647, Int::Compute(Int::UnaryOp::kNeg, Int(int32_t{2147483647})).AsInt64());

  EXPECT_FALSE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int64_t{-9223372036854775807 - 1})));
  EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int64_t{-9223372036854775807})));
  EXPECT_EQ(9223372036854775807,
            Int::Compute(Int::UnaryOp::kNeg, Int(int64_t{-9223372036854775807})).AsInt64());
  EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNeg, Int(int64_t{9223372036854775807})));
  EXPECT_EQ(-9223372036854775807,
            Int::Compute(Int::UnaryOp::kNeg, Int(int64_t{9223372036854775807})).AsInt64());
}

TEST(IntTest, HandlesNotUnaryOp) {
  struct SignedTestCase {
    int64_t num;
    int64_t nott;
  };
  for (auto [num, nott] : std::array<SignedTestCase, 7>{
           SignedTestCase{0, -1}, SignedTestCase{1, -2}, SignedTestCase{2, -3},
           SignedTestCase{42, -43}, SignedTestCase{-42, 41}, SignedTestCase{-2, 1},
           SignedTestCase{-1, 0}}) {
    EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNot, Int(int8_t(num))));
    EXPECT_EQ(IntType::kI8, Int::Compute(Int::UnaryOp::kNot, Int(int8_t(num))).type());
    EXPECT_EQ(nott, Int::Compute(Int::UnaryOp::kNot, Int(int8_t(num))).AsInt64());
    EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNot, Int(int16_t(num))));
    EXPECT_EQ(IntType::kI16, Int::Compute(Int::UnaryOp::kNot, Int(int16_t(num))).type());
    EXPECT_EQ(nott, Int::Compute(Int::UnaryOp::kNot, Int(int16_t(num))).AsInt64());
    EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNot, Int(int32_t(num))));
    EXPECT_EQ(IntType::kI32, Int::Compute(Int::UnaryOp::kNot, Int(int32_t(num))).type());
    EXPECT_EQ(nott, Int::Compute(Int::UnaryOp::kNot, Int(int32_t(num))).AsInt64());
    EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNot, Int(int64_t(num))));
    EXPECT_EQ(IntType::kI64, Int::Compute(Int::UnaryOp::kNot, Int(int64_t(num))).type());
    EXPECT_EQ(nott, Int::Compute(Int::UnaryOp::kNot, Int(int64_t(num))).AsInt64());
  }
  struct UnsignedTestCase {
    uint64_t num;
    uint64_t nott;
  };
  for (auto [num, nott] : std::array<UnsignedTestCase, 8>{
           UnsignedTestCase{0, 255}, UnsignedTestCase{1, 254}, UnsignedTestCase{2, 253},
           UnsignedTestCase{42, 213}, UnsignedTestCase{213, 42}, UnsignedTestCase{253, 2},
           UnsignedTestCase{254, 1}, UnsignedTestCase{255, 0}}) {
    EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNot, Int(uint8_t(num))));
    EXPECT_EQ(IntType::kU8, Int::Compute(Int::UnaryOp::kNot, Int(uint8_t(num))).type());
    EXPECT_EQ(nott, Int::Compute(Int::UnaryOp::kNot, Int(uint8_t(num))).AsUint64());
  }
  for (auto [num, nott] : std::array<UnsignedTestCase, 10>{
           UnsignedTestCase{0, 65535}, UnsignedTestCase{1, 65534}, UnsignedTestCase{2, 65533},
           UnsignedTestCase{42, 65493}, UnsignedTestCase{31148, 34387},
           UnsignedTestCase{34387, 31148}, UnsignedTestCase{65493, 42}, UnsignedTestCase{65533, 2},
           UnsignedTestCase{65534, 1}, UnsignedTestCase{65535, 0}}) {
    EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNot, Int(uint16_t(num))));
    EXPECT_EQ(IntType::kU16, Int::Compute(Int::UnaryOp::kNot, Int(uint16_t(num))).type());
    EXPECT_EQ(nott, Int::Compute(Int::UnaryOp::kNot, Int(uint16_t(num))).AsUint64());
  }
  for (auto [num, nott] : std::array<UnsignedTestCase, 10>{
           UnsignedTestCase{0, 4294967295}, UnsignedTestCase{1, 4294967294},
           UnsignedTestCase{2, 4294967293}, UnsignedTestCase{42, 4294967253},
           UnsignedTestCase{2041351149, 2253616146}, UnsignedTestCase{2253616146, 2041351149},
           UnsignedTestCase{4294967253, 42}, UnsignedTestCase{4294967293, 2},
           UnsignedTestCase{4294967294, 1}, UnsignedTestCase{4294967295, 0}}) {
    EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNot, Int(uint32_t(num))));
    EXPECT_EQ(IntType::kU32, Int::Compute(Int::UnaryOp::kNot, Int(uint32_t(num))).type());
    EXPECT_EQ(nott, Int::Compute(Int::UnaryOp::kNot, Int(uint32_t(num))).AsUint64());
  }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicitly-unsigned-literal"
  for (auto [num, nott] : std::array<UnsignedTestCase, 10>{
           UnsignedTestCase{0, 18446744073709551615}, UnsignedTestCase{1, 18446744073709551614},
           UnsignedTestCase{2, 18446744073709551613}, UnsignedTestCase{42, 18446744073709551573},
           UnsignedTestCase{8767536424969262077, 9679207648740289538},
           UnsignedTestCase{9679207648740289538, 8767536424969262077},
           UnsignedTestCase{18446744073709551573, 42}, UnsignedTestCase{18446744073709551613, 2},
           UnsignedTestCase{18446744073709551614, 1}, UnsignedTestCase{18446744073709551615, 0}}) {
    EXPECT_TRUE(Int::CanCompute(Int::UnaryOp::kNot, Int(uint64_t(num))));
    EXPECT_EQ(IntType::kU64, Int::Compute(Int::UnaryOp::kNot, Int(uint64_t(num))).type());
    EXPECT_EQ(nott, Int::Compute(Int::UnaryOp::kNot, Int(uint64_t(num))).AsUint64());
  }
#pragma clang diagnostic pop
}

TEST(IntTest, HandlesComparisons) {
  std::array<int64_t, 7> signed_nums{-42, -2, -1, 0, 1, 2, 42};
  for (std::size_t i = 0; i < signed_nums.size(); i++) {
    int64_t num_a = signed_nums.at(i);
    for (Int::CompareOp eq_op : std::array<Int::CompareOp, 3>{
             Int::CompareOp::kEq, Int::CompareOp::kLeq, Int::CompareOp::kGeq}) {
      EXPECT_TRUE(Int::Compare(Int(int8_t(num_a)), eq_op, Int(int8_t(num_a))));
      EXPECT_TRUE(Int::Compare(Int(int16_t(num_a)), eq_op, Int(int16_t(num_a))));
      EXPECT_TRUE(Int::Compare(Int(int32_t(num_a)), eq_op, Int(int32_t(num_a))));
      EXPECT_TRUE(Int::Compare(Int(int64_t(num_a)), eq_op, Int(int64_t(num_a))));
    }
    for (Int::CompareOp neq_op : std::array<Int::CompareOp, 3>{
             Int::CompareOp::kNeq, Int::CompareOp::kLss, Int::CompareOp::kGtr}) {
      EXPECT_FALSE(Int::Compare(Int(int8_t(num_a)), neq_op, Int(int8_t(num_a))));
      EXPECT_FALSE(Int::Compare(Int(int16_t(num_a)), neq_op, Int(int16_t(num_a))));
      EXPECT_FALSE(Int::Compare(Int(int32_t(num_a)), neq_op, Int(int32_t(num_a))));
      EXPECT_FALSE(Int::Compare(Int(int64_t(num_a)), neq_op, Int(int64_t(num_a))));
    }
    for (std::size_t j = 0; j < signed_nums.size(); j++) {
      if (i == j) {
        continue;
      }
      int64_t num_b = signed_nums.at(j);
      EXPECT_FALSE(Int::Compare(Int(int8_t(num_a)), Int::CompareOp::kEq, Int(int8_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(int8_t(num_a)), Int::CompareOp::kNeq, Int(int8_t(num_b))));
      EXPECT_FALSE(Int::Compare(Int(int16_t(num_a)), Int::CompareOp::kEq, Int(int16_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(int16_t(num_a)), Int::CompareOp::kNeq, Int(int16_t(num_b))));
      EXPECT_FALSE(Int::Compare(Int(int32_t(num_a)), Int::CompareOp::kEq, Int(int32_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(int32_t(num_a)), Int::CompareOp::kNeq, Int(int32_t(num_b))));
      EXPECT_FALSE(Int::Compare(Int(int64_t(num_a)), Int::CompareOp::kEq, Int(int64_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(int64_t(num_a)), Int::CompareOp::kNeq, Int(int64_t(num_b))));
    }
  }
  for (std::size_t i = 0; i < signed_nums.size() - 1; i++) {
    int64_t num_a = signed_nums.at(i);
    int64_t num_b = signed_nums.at(i + 1);
    for (Int::CompareOp op : std::array<Int::CompareOp, 3>{
             Int::CompareOp::kLss, Int::CompareOp::kLeq, Int::CompareOp::kNeq}) {
      EXPECT_TRUE(Int::Compare(Int(int8_t(num_a)), op, Int(int8_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(int16_t(num_a)), op, Int(int16_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(int32_t(num_a)), op, Int(int32_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(int64_t(num_a)), op, Int(int64_t(num_b))));
    }
    for (Int::CompareOp op : std::array<Int::CompareOp, 3>{
             Int::CompareOp::kEq, Int::CompareOp::kGeq, Int::CompareOp::kGtr}) {
      EXPECT_FALSE(Int::Compare(Int(int8_t(num_a)), op, Int(int8_t(num_b))));
      EXPECT_FALSE(Int::Compare(Int(int16_t(num_a)), op, Int(int16_t(num_b))));
      EXPECT_FALSE(Int::Compare(Int(int32_t(num_a)), op, Int(int32_t(num_b))));
      EXPECT_FALSE(Int::Compare(Int(int64_t(num_a)), op, Int(int64_t(num_b))));
    }
    for (Int::CompareOp op : std::array<Int::CompareOp, 3>{
             Int::CompareOp::kLss, Int::CompareOp::kLeq, Int::CompareOp::kEq}) {
      EXPECT_FALSE(Int::Compare(Int(int8_t(num_b)), op, Int(int8_t(num_a))));
      EXPECT_FALSE(Int::Compare(Int(int16_t(num_b)), op, Int(int16_t(num_a))));
      EXPECT_FALSE(Int::Compare(Int(int32_t(num_b)), op, Int(int32_t(num_a))));
      EXPECT_FALSE(Int::Compare(Int(int64_t(num_b)), op, Int(int64_t(num_a))));
    }
    for (Int::CompareOp op : std::array<Int::CompareOp, 3>{
             Int::CompareOp::kNeq, Int::CompareOp::kGeq, Int::CompareOp::kGtr}) {
      EXPECT_TRUE(Int::Compare(Int(int8_t(num_b)), op, Int(int8_t(num_a))));
      EXPECT_TRUE(Int::Compare(Int(int16_t(num_b)), op, Int(int16_t(num_a))));
      EXPECT_TRUE(Int::Compare(Int(int32_t(num_b)), op, Int(int32_t(num_a))));
      EXPECT_TRUE(Int::Compare(Int(int64_t(num_b)), op, Int(int64_t(num_a))));
    }
  }

  std::array<uint64_t, 4> unsigned_nums{0, 1, 2, 42};
  for (std::size_t i = 0; i < unsigned_nums.size(); i++) {
    uint64_t num_a = unsigned_nums.at(i);
    for (Int::CompareOp eq_op : std::array<Int::CompareOp, 3>{
             Int::CompareOp::kEq, Int::CompareOp::kLeq, Int::CompareOp::kGeq}) {
      EXPECT_TRUE(Int::Compare(Int(uint8_t(num_a)), eq_op, Int(uint8_t(num_a))));
      EXPECT_TRUE(Int::Compare(Int(uint16_t(num_a)), eq_op, Int(uint16_t(num_a))));
      EXPECT_TRUE(Int::Compare(Int(uint32_t(num_a)), eq_op, Int(uint32_t(num_a))));
      EXPECT_TRUE(Int::Compare(Int(uint64_t(num_a)), eq_op, Int(uint64_t(num_a))));
    }
    for (Int::CompareOp neq_op : std::array<Int::CompareOp, 3>{
             Int::CompareOp::kNeq, Int::CompareOp::kLss, Int::CompareOp::kGtr}) {
      EXPECT_FALSE(Int::Compare(Int(uint8_t(num_a)), neq_op, Int(uint8_t(num_a))));
      EXPECT_FALSE(Int::Compare(Int(uint16_t(num_a)), neq_op, Int(uint16_t(num_a))));
      EXPECT_FALSE(Int::Compare(Int(uint32_t(num_a)), neq_op, Int(uint32_t(num_a))));
      EXPECT_FALSE(Int::Compare(Int(uint64_t(num_a)), neq_op, Int(uint64_t(num_a))));
    }
    for (std::size_t j = 0; j < unsigned_nums.size(); j++) {
      if (i == j) {
        continue;
      }
      uint64_t num_b = unsigned_nums.at(j);
      EXPECT_FALSE(Int::Compare(Int(uint8_t(num_a)), Int::CompareOp::kEq, Int(uint8_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(uint8_t(num_a)), Int::CompareOp::kNeq, Int(uint8_t(num_b))));
      EXPECT_FALSE(Int::Compare(Int(uint16_t(num_a)), Int::CompareOp::kEq, Int(uint16_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(uint16_t(num_a)), Int::CompareOp::kNeq, Int(uint16_t(num_b))));
      EXPECT_FALSE(Int::Compare(Int(uint32_t(num_a)), Int::CompareOp::kEq, Int(uint32_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(uint32_t(num_a)), Int::CompareOp::kNeq, Int(uint32_t(num_b))));
      EXPECT_FALSE(Int::Compare(Int(uint64_t(num_a)), Int::CompareOp::kEq, Int(uint64_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(uint64_t(num_a)), Int::CompareOp::kNeq, Int(uint64_t(num_b))));
    }
  }
  for (std::size_t i = 0; i < unsigned_nums.size() - 1; i++) {
    uint64_t num_a = unsigned_nums.at(i);
    uint64_t num_b = unsigned_nums.at(i + 1);
    for (Int::CompareOp op : std::array<Int::CompareOp, 3>{
             Int::CompareOp::kLss, Int::CompareOp::kLeq, Int::CompareOp::kNeq}) {
      EXPECT_TRUE(Int::Compare(Int(uint8_t(num_a)), op, Int(uint8_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(uint16_t(num_a)), op, Int(uint16_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(uint32_t(num_a)), op, Int(uint32_t(num_b))));
      EXPECT_TRUE(Int::Compare(Int(uint64_t(num_a)), op, Int(uint64_t(num_b))));
    }
    for (Int::CompareOp op : std::array<Int::CompareOp, 3>{
             Int::CompareOp::kEq, Int::CompareOp::kGeq, Int::CompareOp::kGtr}) {
      EXPECT_FALSE(Int::Compare(Int(uint8_t(num_a)), op, Int(uint8_t(num_b))));
      EXPECT_FALSE(Int::Compare(Int(uint16_t(num_a)), op, Int(uint16_t(num_b))));
      EXPECT_FALSE(Int::Compare(Int(uint32_t(num_a)), op, Int(uint32_t(num_b))));
      EXPECT_FALSE(Int::Compare(Int(uint64_t(num_a)), op, Int(uint64_t(num_b))));
    }
    for (Int::CompareOp op : std::array<Int::CompareOp, 3>{
             Int::CompareOp::kLss, Int::CompareOp::kLeq, Int::CompareOp::kEq}) {
      EXPECT_FALSE(Int::Compare(Int(uint8_t(num_b)), op, Int(uint8_t(num_a))));
      EXPECT_FALSE(Int::Compare(Int(uint16_t(num_b)), op, Int(uint16_t(num_a))));
      EXPECT_FALSE(Int::Compare(Int(uint32_t(num_b)), op, Int(uint32_t(num_a))));
      EXPECT_FALSE(Int::Compare(Int(uint64_t(num_b)), op, Int(uint64_t(num_a))));
    }
    for (Int::CompareOp op : std::array<Int::CompareOp, 3>{
             Int::CompareOp::kNeq, Int::CompareOp::kGeq, Int::CompareOp::kGtr}) {
      EXPECT_TRUE(Int::Compare(Int(uint8_t(num_b)), op, Int(uint8_t(num_a))));
      EXPECT_TRUE(Int::Compare(Int(uint16_t(num_b)), op, Int(uint16_t(num_a))));
      EXPECT_TRUE(Int::Compare(Int(uint32_t(num_b)), op, Int(uint32_t(num_a))));
      EXPECT_TRUE(Int::Compare(Int(uint64_t(num_b)), op, Int(uint64_t(num_a))));
    }
  }
}

TEST(IntTest, ToStringConvertsCorrectly) {
  EXPECT_EQ(Int(int8_t{0}).ToString(), "0");
  EXPECT_EQ(Int(int16_t{0}).ToString(), "0");
  EXPECT_EQ(Int(int32_t{0}).ToString(), "0");
  EXPECT_EQ(Int(int64_t{0}).ToString(), "0");
  EXPECT_EQ(Int(uint8_t{0}).ToString(), "0");
  EXPECT_EQ(Int(uint16_t{0}).ToString(), "0");
  EXPECT_EQ(Int(uint32_t{0}).ToString(), "0");
  EXPECT_EQ(Int(uint64_t{0}).ToString(), "0");

  EXPECT_EQ(Int(int8_t{1}).ToString(), "1");
  EXPECT_EQ(Int(int16_t{1}).ToString(), "1");
  EXPECT_EQ(Int(int32_t{1}).ToString(), "1");
  EXPECT_EQ(Int(int64_t{1}).ToString(), "1");
  EXPECT_EQ(Int(uint8_t{1}).ToString(), "1");
  EXPECT_EQ(Int(uint16_t{1}).ToString(), "1");
  EXPECT_EQ(Int(uint32_t{1}).ToString(), "1");
  EXPECT_EQ(Int(uint64_t{1}).ToString(), "1");

  EXPECT_EQ(Int(int8_t{127}).ToString(), "127");
  EXPECT_EQ(Int(int16_t{127}).ToString(), "127");
  EXPECT_EQ(Int(int32_t{127}).ToString(), "127");
  EXPECT_EQ(Int(int64_t{127}).ToString(), "127");
  EXPECT_EQ(Int(uint8_t{127}).ToString(), "127");
  EXPECT_EQ(Int(uint16_t{127}).ToString(), "127");
  EXPECT_EQ(Int(uint32_t{127}).ToString(), "127");
  EXPECT_EQ(Int(uint64_t{127}).ToString(), "127");

  EXPECT_EQ(Int(int8_t{-1}).ToString(), "-1");
  EXPECT_EQ(Int(int16_t{-1}).ToString(), "-1");
  EXPECT_EQ(Int(int32_t{-1}).ToString(), "-1");
  EXPECT_EQ(Int(int64_t{-1}).ToString(), "-1");

  EXPECT_EQ(Int(int8_t{-128}).ToString(), "-128");
  EXPECT_EQ(Int(int16_t{-128}).ToString(), "-128");
  EXPECT_EQ(Int(int32_t{-128}).ToString(), "-128");
  EXPECT_EQ(Int(int64_t{-128}).ToString(), "-128");
}

TEST(ToI64Test, RejectsEmpty) { EXPECT_EQ(ToI64(""), std::nullopt); }

TEST(ToI64Test, RejectsWhitespace) {
  EXPECT_EQ(ToI64("\t"), std::nullopt);
  EXPECT_EQ(ToI64("\n"), std::nullopt);
  EXPECT_EQ(ToI64(" "), std::nullopt);
  EXPECT_EQ(ToI64(" \t \t"), std::nullopt);
  EXPECT_EQ(ToI64("\t \n\t"), std::nullopt);
}

TEST(ToI64Test, RejectsInvalidStrings) {
  EXPECT_EQ(ToI64("abc"), std::nullopt);
  EXPECT_EQ(ToI64("+-0"), std::nullopt);
  EXPECT_EQ(ToI64("x17"), std::nullopt);
  EXPECT_EQ(ToI64("----"), std::nullopt);
  EXPECT_EQ(ToI64("X22"), std::nullopt);
  EXPECT_EQ(ToI64("&"), std::nullopt);
  EXPECT_EQ(ToI64("&42"), std::nullopt);
  EXPECT_EQ(ToI64("*"), std::nullopt);
  EXPECT_EQ(ToI64("*123"), std::nullopt);
}

TEST(ToI64Test, HandlesValidStrings) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicitly-unsigned-literal"
  EXPECT_THAT(ToI64("0"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 0))));
  EXPECT_THAT(ToI64("0000"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 0))));
  EXPECT_THAT(ToI64("0x0"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 0))));
  EXPECT_THAT(ToI64("+0"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 0))));
  EXPECT_THAT(ToI64("-0"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 0))));
  EXPECT_THAT(ToI64("1"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 1))));
  EXPECT_THAT(ToI64("00001"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 1))));
  EXPECT_THAT(ToI64("0x0001"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 1))));
  EXPECT_THAT(ToI64("+1"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 1))));
  EXPECT_THAT(ToI64("+0x1"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 1))));
  EXPECT_THAT(ToI64("-1"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, -1))));
  EXPECT_THAT(ToI64("-0x1"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, -1))));
  EXPECT_THAT(ToI64("-00001"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, -1))));
  EXPECT_THAT(ToI64("42"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 42))));
  EXPECT_THAT(ToI64("+42"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 42))));
  EXPECT_THAT(ToI64("-42"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, -42))));
  EXPECT_THAT(ToI64("042"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 34))));
  EXPECT_THAT(ToI64("+042"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 34))));
  EXPECT_THAT(ToI64("-042"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, -34))));
  EXPECT_THAT(ToI64("0x42"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 66))));
  EXPECT_THAT(ToI64("+0x42"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, 66))));
  EXPECT_THAT(ToI64("-0x42"),
              Optional(AllOf(Property(&Int::type, IntType::kI64), Property(&Int::AsInt64, -66))));
  EXPECT_THAT(ToI64("9223372036854775807"),
              Optional(AllOf(Property(&Int::type, IntType::kI64),
                             Property(&Int::AsInt64, 9223372036854775807))));
  EXPECT_THAT(ToI64("+9223372036854775807"),
              Optional(AllOf(Property(&Int::type, IntType::kI64),
                             Property(&Int::AsInt64, 9223372036854775807))));
  EXPECT_THAT(ToI64("-9223372036854775808"),
              Optional(AllOf(Property(&Int::type, IntType::kI64),
                             Property(&Int::AsInt64, -9223372036854775808))));
  EXPECT_THAT(ToI64("0x7fffffffffffffff"),
              Optional(AllOf(Property(&Int::type, IntType::kI64),
                             Property(&Int::AsInt64, 9223372036854775807))));
  EXPECT_THAT(ToI64("+0x7fffffffffffffff"),
              Optional(AllOf(Property(&Int::type, IntType::kI64),
                             Property(&Int::AsInt64, 9223372036854775807))));
  EXPECT_THAT(ToI64("-0x8000000000000000"),
              Optional(AllOf(Property(&Int::type, IntType::kI64),
                             Property(&Int::AsInt64, -9223372036854775808))));
#pragma clang diagnostic pop
}

TEST(ToI64Test, RejectsOverflow) {
  EXPECT_EQ(ToI64("9223372036854775808"), std::nullopt);
  EXPECT_EQ(ToI64("+9223372036854775808"), std::nullopt);
  EXPECT_EQ(ToI64("-9223372036854775809"), std::nullopt);

  EXPECT_EQ(ToI64("0x8000000000000000"), std::nullopt);
  EXPECT_EQ(ToI64("+0x8000000000000000"), std::nullopt);
  EXPECT_EQ(ToI64("-0x8000000000000001"), std::nullopt);
}

TEST(ToU64Test, RejectsEmpty) { EXPECT_EQ(ToU64(""), std::nullopt); }

TEST(ToU64Test, RejectsWhitespace) {
  EXPECT_EQ(ToU64("\t"), std::nullopt);
  EXPECT_EQ(ToU64("\n"), std::nullopt);
  EXPECT_EQ(ToU64(" "), std::nullopt);
  EXPECT_EQ(ToU64(" \t \t"), std::nullopt);
  EXPECT_EQ(ToU64("\t \n\t"), std::nullopt);
}

TEST(ToU64Test, RejectsInvalidStrings) {
  EXPECT_EQ(ToU64("abc"), std::nullopt);
  EXPECT_EQ(ToU64("+-0"), std::nullopt);
  EXPECT_EQ(ToU64("x17"), std::nullopt);
  EXPECT_EQ(ToU64("----"), std::nullopt);
  EXPECT_EQ(ToU64("X22"), std::nullopt);
  EXPECT_EQ(ToU64("&"), std::nullopt);
  EXPECT_EQ(ToU64("&42"), std::nullopt);
  EXPECT_EQ(ToU64("*"), std::nullopt);
  EXPECT_EQ(ToU64("*123"), std::nullopt);
}

TEST(ToU64Test, HandlesValidStrings) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicitly-unsigned-literal"
  EXPECT_THAT(ToU64("0"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 0))));
  EXPECT_THAT(ToU64("0000"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 0))));
  EXPECT_THAT(ToU64("0x0"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 0))));
  EXPECT_THAT(ToU64("+0"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 0))));
  EXPECT_THAT(ToU64("1"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 1))));
  EXPECT_THAT(ToU64("00001"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 1))));
  EXPECT_THAT(ToU64("0x0001"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 1))));
  EXPECT_THAT(ToU64("+1"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 1))));
  EXPECT_THAT(ToU64("+0x1"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 1))));
  EXPECT_THAT(ToU64("42"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 42))));
  EXPECT_THAT(ToU64("+42"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 42))));
  EXPECT_THAT(ToU64("042"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 34))));
  EXPECT_THAT(ToU64("+042"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 34))));
  EXPECT_THAT(ToU64("0x42"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 66))));
  EXPECT_THAT(ToU64("+0x42"),
              Optional(AllOf(Property(&Int::type, IntType::kU64), Property(&Int::AsUint64, 66))));
  EXPECT_THAT(ToU64("18446744073709551615"),
              Optional(AllOf(Property(&Int::type, IntType::kU64),
                             Property(&Int::AsUint64, 18446744073709551615))));
  EXPECT_THAT(ToU64("+18446744073709551615"),
              Optional(AllOf(Property(&Int::type, IntType::kU64),
                             Property(&Int::AsUint64, 18446744073709551615))));
  EXPECT_THAT(ToU64("0xffffffffffffffff"),
              Optional(AllOf(Property(&Int::type, IntType::kU64),
                             Property(&Int::AsUint64, 18446744073709551615))));
  EXPECT_THAT(ToU64("+0xffffffffffffffff"),
              Optional(AllOf(Property(&Int::type, IntType::kU64),
                             Property(&Int::AsUint64, 18446744073709551615))));
#pragma clang diagnostic pop
}

TEST(ToU64Test, RejectsNegativeNumbers) {
  EXPECT_EQ(ToU64("-0"), std::nullopt);
  EXPECT_EQ(ToU64("-1"), std::nullopt);
  EXPECT_EQ(ToU64("-42"), std::nullopt);
  EXPECT_EQ(ToU64("-00"), std::nullopt);
  EXPECT_EQ(ToU64("-01"), std::nullopt);
  EXPECT_EQ(ToU64("-042"), std::nullopt);
  EXPECT_EQ(ToU64("-0x0"), std::nullopt);
  EXPECT_EQ(ToU64("-0x1"), std::nullopt);
  EXPECT_EQ(ToU64("-0x42"), std::nullopt);
}

TEST(ToU64Test, RejectsOverflow) {
  EXPECT_EQ(ToU64("18446744073709551616"), std::nullopt);
  EXPECT_EQ(ToU64("+18446744073709551616"), std::nullopt);

  EXPECT_EQ(ToU64("0x10000000000000000"), std::nullopt);
  EXPECT_EQ(ToU64("+0x10000000000000000"), std::nullopt);
}

}  // namespace common::atomics
