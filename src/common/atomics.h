//
//  atomics.h
//  Katara
//
//  Created by Arne Philipeit on 6/26/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef common_atomics_h
#define common_atomics_h

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace common {

enum class IntType {
  kI8,
  kI16,
  kI32,
  kI64,
  kU8,
  kU16,
  kU32,
  kU64,
};

constexpr int8_t BitSizeOf(IntType type);

constexpr bool IsSigned(IntType type);
constexpr bool IsUnsigned(IntType type) { return !IsSigned(type); }

constexpr IntType ToSigned(IntType type);
constexpr IntType ToUnsigned(IntType type);

std::optional<IntType> ToIntType(std::string_view str);
std::string ToString(IntType type);

class Bool {
 public:
  enum class BinaryOp {
    kEq,
    kNeq,
    kAnd,
    kOr,
  };

  static constexpr class Int ConvertTo(IntType type, bool a);

  static constexpr bool Compute(bool a, BinaryOp op, bool b);

  static std::string ToString(bool a);
};

std::optional<Bool::BinaryOp> ToBoolBinaryOp(std::string_view str);
std::string ToString(Bool::BinaryOp op);

class Int {
 public:
  enum class UnaryOp {
    kNeg,
    kNot,
  };
  enum class CompareOp {
    kEq,
    kNeq,
    kLss,
    kLeq,
    kGeq,
    kGtr,
  };
  enum class BinaryOp {
    kAdd,
    kSub,
    kMul,
    kDiv,
    kRem,
    kAnd,
    kOr,
    kXor,
    kAndNot,
  };
  enum class ShiftOp {
    kLeft,
    kRight,
  };

  constexpr explicit Int(int8_t value) : value_(value) {}
  constexpr explicit Int(int16_t value) : value_(value) {}
  constexpr explicit Int(int32_t value) : value_(value) {}
  constexpr explicit Int(int64_t value) : value_(value) {}
  constexpr explicit Int(uint8_t value) : value_(value) {}
  constexpr explicit Int(uint16_t value) : value_(value) {}
  constexpr explicit Int(uint32_t value) : value_(value) {}
  constexpr explicit Int(uint64_t value) : value_(value) {}

  constexpr IntType type() const { return IntType(value_.index()); }

  constexpr bool IsZero() const;
  constexpr bool IsNotZero() const { return !IsZero(); }

  constexpr bool IsOne() const;
  constexpr bool IsMinusOne() const;

  constexpr bool IsMin() const;
  constexpr bool IsMax() const;

  constexpr bool IsLessThanZero() const;
  constexpr bool IsLessThanOrEqualToZero() const;
  constexpr bool IsGreaterThanOrEqualToZero() const;
  constexpr bool IsGreaterThanZero() const;

  constexpr bool IsRepresentableAsInt64() const;
  constexpr int64_t AsInt64() const;

  constexpr bool IsRepresentableAsUint64() const { return IsGreaterThanOrEqualToZero(); }
  constexpr uint64_t AsUint64() const;

  constexpr bool CanConvertToUnsigned() const { return CanConvertTo(ToUnsigned(type())); }
  constexpr Int ConvertToUnsigned() const { return ConvertTo(ToUnsigned(type())); }

  constexpr bool CanConvertTo(IntType result_type) const;
  constexpr Int ConvertTo(IntType result_type) const;

  constexpr bool ConvertToBool() const { return IsNotZero(); }

  constexpr static bool CanCompute(UnaryOp op, Int a);
  constexpr static Int Compute(UnaryOp op, Int a);

  constexpr static bool CanCompare(Int a, Int b) { return a.type() == b.type(); }
  constexpr static bool Compare(Int a, CompareOp op, Int b);

  constexpr static bool CanCompute(Int a, Int b) { return a.type() == b.type(); }
  constexpr static Int Compute(Int a, BinaryOp op, Int b);

  constexpr static Int Shift(Int a, ShiftOp op, Int b);

  std::string ToString() const;

 private:
  typedef std::variant<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>
      int_t;

  int_t value_;
};

constexpr Int::CompareOp Flipped(Int::CompareOp op);

std::optional<Int::UnaryOp> ToIntUnaryOp(std::string_view str);
std::string ToString(Int::UnaryOp op);

std::optional<Int::CompareOp> ToIntCompareOp(std::string_view str);
std::string ToString(Int::CompareOp op);

std::optional<Int::BinaryOp> ToIntBinaryOp(std::string_view str);
std::string ToString(Int::BinaryOp op);

std::optional<Int::ShiftOp> ToIntShiftOp(std::string_view str);
std::string ToString(Int::ShiftOp op);

constexpr int8_t BitSizeOf(IntType type) {
  switch (type) {
    case IntType::kI8:
    case IntType::kU8:
      return 8;
    case IntType::kI16:
    case IntType::kU16:
      return 16;
    case IntType::kI32:
    case IntType::kU32:
      return 32;
    case IntType::kI64:
    case IntType::kU64:
      return 64;
  }
}

constexpr bool IsSigned(IntType type) {
  switch (type) {
    case IntType::kI8:
    case IntType::kI16:
    case IntType::kI32:
    case IntType::kI64:
      return true;
    case IntType::kU8:
    case IntType::kU16:
    case IntType::kU32:
    case IntType::kU64:
      return false;
  }
}

constexpr IntType ToSigned(IntType type) {
  switch (type) {
    case IntType::kI8:
    case IntType::kU8:
      return IntType::kI8;
    case IntType::kI16:
    case IntType::kU16:
      return IntType::kI16;
    case IntType::kI32:
    case IntType::kU32:
      return IntType::kI32;
    case IntType::kI64:
    case IntType::kU64:
      return IntType::kI64;
  }
}

constexpr IntType ToUnsigned(IntType type) {
  switch (type) {
    case IntType::kI8:
    case IntType::kU8:
      return IntType::kU8;
    case IntType::kI16:
    case IntType::kU16:
      return IntType::kU16;
    case IntType::kI32:
    case IntType::kU32:
      return IntType::kU32;
    case IntType::kI64:
    case IntType::kU64:
      return IntType::kU64;
  }
}

constexpr Int Bool::ConvertTo(IntType type, bool a) {
  switch (type) {
    case IntType::kI8:
      return a ? Int(int8_t{0}) : Int(int8_t{1});
    case IntType::kI16:
      return a ? Int(int16_t{0}) : Int(int16_t{1});
    case IntType::kI32:
      return a ? Int(int32_t{0}) : Int(int32_t{1});
    case IntType::kI64:
      return a ? Int(int64_t{0}) : Int(int64_t{1});
    case IntType::kU8:
      return a ? Int(uint8_t{0}) : Int(uint8_t{1});
    case IntType::kU16:
      return a ? Int(uint16_t{0}) : Int(uint16_t{1});
    case IntType::kU32:
      return a ? Int(uint32_t{0}) : Int(uint32_t{1});
    case IntType::kU64:
      return a ? Int(uint64_t{0}) : Int(uint64_t{1});
  }
}

constexpr bool Bool::Compute(bool a, BinaryOp op, bool b) {
  switch (op) {
    case BinaryOp::kEq:
      return a == b;
    case BinaryOp::kNeq:
      return a != b;
    case BinaryOp::kAnd:
      return a && b;
    case BinaryOp::kOr:
      return a || b;
  }
}

constexpr bool Int::IsZero() const {
  return std::visit([](auto&& value) { return value == 0; }, value_);
}

constexpr bool Int::IsOne() const {
  return std::visit([](auto&& value) { return value == 1; }, value_);
}

constexpr bool Int::IsMinusOne() const {
  if (IsUnsigned(type())) {
    return false;
  }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
  return std::visit([](auto&& value) { return value == -1; }, value_);
#pragma clang diagnostic pop
}

constexpr bool Int::IsMin() const {
  return std::visit(
      [](auto&& value) {
        return value == std::numeric_limits<std::decay_t<decltype(value)>>::min();
      },
      value_);
}

constexpr bool Int::IsMax() const {
  return std::visit(
      [](auto&& value) {
        return value == std::numeric_limits<std::decay_t<decltype(value)>>::max();
      },
      value_);
}

constexpr bool Int::IsLessThanZero() const {
  return std::visit([](auto&& value) { return value < 0; }, value_);
}

constexpr bool Int::IsLessThanOrEqualToZero() const {
  return std::visit([](auto&& value) { return value <= 0; }, value_);
}

constexpr bool Int::IsGreaterThanOrEqualToZero() const {
  return std::visit([](auto&& value) { return value >= 0; }, value_);
}

constexpr bool Int::IsGreaterThanZero() const {
  return std::visit([](auto&& value) { return value > 0; }, value_);
}

constexpr bool Int::IsRepresentableAsInt64() const {
  switch (type()) {
    case IntType::kI8:
    case IntType::kI16:
    case IntType::kI32:
    case IntType::kI64:
    case IntType::kU8:
    case IntType::kU16:
    case IntType::kU32:
      return true;
    case IntType::kU64:
      return std::get<uint64_t>(value_) <= uint64_t{std::numeric_limits<int64_t>::max()};
  }
}

constexpr int64_t Int::AsInt64() const {
  return std::visit([](auto&& value) { return static_cast<int64_t>(value); }, value_);
}

constexpr uint64_t Int::AsUint64() const {
  return std::visit([](auto&& value) { return static_cast<uint64_t>(value); }, value_);
}

constexpr bool Int::CanConvertTo(IntType result_type) const {
  if (IsLessThanZero() && IsUnsigned(result_type)) {
    return false;
  }
  return std::visit(
      [result_type](auto&& value) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
        switch (result_type) {
          case IntType::kI8:
            return value >= std::numeric_limits<int8_t>::min() &&
                   value <= std::numeric_limits<int8_t>::max();
          case IntType::kI16:
            return value >= std::numeric_limits<int16_t>::min() &&
                   value <= std::numeric_limits<int16_t>::max();
          case IntType::kI32:
            return value >= std::numeric_limits<int32_t>::min() &&
                   value <= std::numeric_limits<int32_t>::max();
          case IntType::kI64:
            return value >= std::numeric_limits<int64_t>::min() &&
                   value <= std::numeric_limits<int64_t>::max();
          case IntType::kU8:
            return value <= std::numeric_limits<uint8_t>::max();
          case IntType::kU16:
            return value <= std::numeric_limits<uint16_t>::max();
          case IntType::kU32:
            return value <= std::numeric_limits<uint32_t>::max();
          case IntType::kU64:
            return true;
        }
#pragma clang diagnostic pop
      },
      value_);
}

constexpr Int Int::ConvertTo(IntType result_type) const {
  return std::visit(
      [result_type](auto&& value) {
        switch (result_type) {
          case IntType::kI8:
            return Int(static_cast<int8_t>(value));
          case IntType::kI16:
            return Int(static_cast<int16_t>(value));
          case IntType::kI32:
            return Int(static_cast<int32_t>(value));
          case IntType::kI64:
            return Int(static_cast<int64_t>(value));
          case IntType::kU8:
            return Int(static_cast<uint8_t>(value));
          case IntType::kU16:
            return Int(static_cast<uint16_t>(value));
          case IntType::kU32:
            return Int(static_cast<uint32_t>(value));
          case IntType::kU64:
            return Int(static_cast<uint64_t>(value));
        }
      },
      value_);
}

constexpr bool Int::CanCompute(UnaryOp op, Int a) {
  switch (op) {
    case UnaryOp::kNot:
      return true;
    case UnaryOp::kNeg:
      return IsSigned(a.type()) && !a.IsMin();
  }
}

constexpr Int Int::Compute(UnaryOp op, Int a) {
  switch (op) {
    case UnaryOp::kNeg:
      return std::visit(
          [](auto&& value) { return Int(static_cast<std::decay_t<decltype(value)>>(-value)); },
          a.value_);
    case UnaryOp::kNot:
      return std::visit(
          [](auto&& value) { return Int(static_cast<std::decay_t<decltype(value)>>(~value)); },
          a.value_);
  }
}

constexpr bool Int::Compare(Int a, CompareOp op, Int b) {
  return std::visit(
      [op, b](auto&& a_val) {
        using T = std::decay_t<decltype(a_val)>;
        auto&& b_val = std::get<T>(b.value_);
        switch (op) {
          case CompareOp::kEq:
            return a_val == b_val;
          case CompareOp::kNeq:
            return a_val != b_val;
          case CompareOp::kLss:
            return a_val < b_val;
          case CompareOp::kLeq:
            return a_val <= b_val;
          case CompareOp::kGeq:
            return a_val >= b_val;
          case CompareOp::kGtr:
            return a_val > b_val;
        }
      },
      a.value_);
}

constexpr Int Int::Compute(Int a, BinaryOp op, Int b) {
  return std::visit(
      [op, b](auto&& a_val) {
        using T = std::decay_t<decltype(a_val)>;
        auto&& b_val = std::get<T>(b.value_);
        switch (op) {
          case BinaryOp::kAdd:
            return Int(static_cast<T>(a_val + b_val));
          case BinaryOp::kSub:
            return Int(static_cast<T>(a_val - b_val));
          case BinaryOp::kMul:
            return Int(static_cast<T>(a_val * b_val));
          case BinaryOp::kDiv:
            return Int(static_cast<T>(a_val / b_val));
          case BinaryOp::kRem:
            return Int(static_cast<T>(a_val % b_val));
          case BinaryOp::kAnd:
            return Int(static_cast<T>(a_val & b_val));
          case BinaryOp::kOr:
            return Int(static_cast<T>(a_val | b_val));
          case BinaryOp::kXor:
            return Int(static_cast<T>(a_val ^ b_val));
          case BinaryOp::kAndNot:
            return Int(static_cast<T>(a_val & ~b_val));
        }
      },
      a.value_);
}

constexpr Int Int::Shift(Int a, ShiftOp op, Int b) {
  return std::visit(
      [op](auto&& a_val, auto&& b_val) {
        switch (op) {
          case ShiftOp::kLeft:
            return Int(static_cast<std::decay_t<decltype(a_val)>>(a_val << b_val));
          case ShiftOp::kRight:
            return Int(static_cast<std::decay_t<decltype(a_val)>>(a_val >> b_val));
        }
      },
      a.value_, b.value_);
}

constexpr Int::CompareOp Flipped(Int::CompareOp op) {
  switch (op) {
    case Int::CompareOp::kEq:
      return Int::CompareOp::kEq;
    case Int::CompareOp::kNeq:
      return Int::CompareOp::kNeq;
    case Int::CompareOp::kLss:
      return Int::CompareOp::kGtr;
    case Int::CompareOp::kLeq:
      return Int::CompareOp::kGeq;
    case Int::CompareOp::kGeq:
      return Int::CompareOp::kLeq;
    case Int::CompareOp::kGtr:
      return Int::CompareOp::kLss;
  }
}

}  // namespace common

#endif /* common_atomics_h */
