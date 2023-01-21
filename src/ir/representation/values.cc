//
//  values.cc
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "values.h"

#include <iomanip>
#include <sstream>

namespace ir {

using ::common::atomics::Int;

void Value::WriteRefStringWithType(std::ostream& os) const {
  WriteRefString(os);
  os << ":";
  type()->WriteRefString(os);
}

std::string Value::RefStringWithType() const {
  std::stringstream ss;
  WriteRefStringWithType(ss);
  return ss.str();
}

bool BoolConstant::operator==(const Value& that) const {
  if (that.kind() != Value::Kind::kConstant) return false;
  if (that.type() != bool_type()) return false;
  return value() == static_cast<const BoolConstant&>(that).value();
}

std::shared_ptr<BoolConstant> False() {
  static auto kFalse = std::shared_ptr<BoolConstant>(new BoolConstant(false));
  return kFalse;
}

std::shared_ptr<BoolConstant> True() {
  static auto kTrue = std::shared_ptr<BoolConstant>(new BoolConstant(true));
  return kTrue;
}

std::shared_ptr<BoolConstant> ToBoolConstant(bool value) { return value ? True() : False(); }

bool IntConstant::operator==(const Value& that) const {
  if (that.kind() != Value::Kind::kConstant) return false;
  if (that.type() != type()) return false;
  return Int::Compare(value(), Int::CompareOp::kEq, static_cast<const IntConstant&>(that).value());
}

std::shared_ptr<IntConstant> MakeIntConstant(Int value) {
  return std::shared_ptr<IntConstant>(new IntConstant(value));
}

std::shared_ptr<IntConstant> I8Zero() {
  static auto kI8Zero = MakeIntConstant(Int(int8_t{0}));
  return kI8Zero;
}

std::shared_ptr<IntConstant> I16Zero() {
  static auto kI16Zero = MakeIntConstant(Int(int16_t{0}));
  return kI16Zero;
}

std::shared_ptr<IntConstant> I32Zero() {
  static auto kI32Zero = MakeIntConstant(Int(int32_t{0}));
  return kI32Zero;
}

std::shared_ptr<IntConstant> I64Zero() {
  static auto kI64Zero = MakeIntConstant(Int(int64_t{0}));
  return kI64Zero;
}

std::shared_ptr<IntConstant> I64One() {
  static auto kI64One = MakeIntConstant(Int(int64_t{1}));
  return kI64One;
}

std::shared_ptr<IntConstant> I64Eight() {
  static auto kI64Eight = MakeIntConstant(Int(int64_t{8}));
  return kI64Eight;
}

std::shared_ptr<IntConstant> U8Zero() {
  static auto kU8Zero = MakeIntConstant(Int(uint8_t{0}));
  return kU8Zero;
}

std::shared_ptr<IntConstant> U16Zero() {
  static auto kU16Zero = MakeIntConstant(Int(uint16_t{0}));
  return kU16Zero;
}

std::shared_ptr<IntConstant> U32Zero() {
  static auto kU32Zero = MakeIntConstant(Int(uint32_t{0}));
  return kU32Zero;
}

std::shared_ptr<IntConstant> U64Zero() {
  static auto kU64Zero = MakeIntConstant(Int(uint64_t{0}));
  return kU64Zero;
}

std::shared_ptr<IntConstant> ZeroWithType(common::atomics::IntType type) {
  switch (type) {
    case common::atomics::IntType::kI8:
      return ir::I8Zero();
    case common::atomics::IntType::kI16:
      return ir::I16Zero();
    case common::atomics::IntType::kI32:
      return ir::I32Zero();
    case common::atomics::IntType::kI64:
      return ir::I64Zero();
    case common::atomics::IntType::kU8:
      return ir::U8Zero();
    case common::atomics::IntType::kU16:
      return ir::U16Zero();
    case common::atomics::IntType::kU32:
      return ir::U32Zero();
    case common::atomics::IntType::kU64:
      return ir::U64Zero();
  }
}

std::shared_ptr<IntConstant> ToIntConstant(Int value) {
  if (value.IsZero()) {
    return ZeroWithType(value.type());

  } else if (value.type() == common::atomics::IntType::kI64) {
    switch (value.AsInt64()) {
      case 1:
        return I64One();
      case 8:
        return I64Eight();
      default:
        break;
    }
  }
  return MakeIntConstant(value);
}

void PointerConstant::WriteRefString(std::ostream& os) const {
  if (value_ == 0) {
    os << "0x0";
  } else {
    os << std::hex << std::showbase << value_ << std::dec << std::noshowbase;
  }
}

bool PointerConstant::operator==(const Value& that) const {
  if (that.kind() != Value::Kind::kConstant) return false;
  if (that.type() != pointer_type()) return false;
  return value() == static_cast<const PointerConstant&>(that).value();
}

std::shared_ptr<PointerConstant> MakePointerConstant(int64_t value) {
  return std::shared_ptr<PointerConstant>(new PointerConstant(value));
}

std::shared_ptr<PointerConstant> NilPointer() {
  static auto kNilPointer = MakePointerConstant(0);
  return kNilPointer;
}

std::shared_ptr<PointerConstant> ToPointerConstant(int64_t value) {
  if (value == 0) {
    return NilPointer();
  }
  return MakePointerConstant(value);
}

bool FuncConstant::operator==(const Value& that) const {
  if (that.kind() != Value::Kind::kConstant) return false;
  if (that.type() != func_type()) return false;
  return value() == static_cast<const FuncConstant&>(that).value();
}

std::shared_ptr<FuncConstant> MakeFuncConstant(func_num_t value) {
  return std::shared_ptr<FuncConstant>(new FuncConstant(value));
}

std::shared_ptr<FuncConstant> NilFunc() {
  static auto kNilFunc = MakeFuncConstant(kNoFuncNum);
  return kNilFunc;
}

std::shared_ptr<FuncConstant> ToFuncConstant(func_num_t value) {
  if (value == kNoFuncNum) {
    return NilFunc();
  }
  return MakeFuncConstant(value);
}

bool Computed::operator==(const Value& that_value) const {
  if (that_value.kind() != Value::Kind::kComputed) return false;
  auto that = static_cast<const Computed&>(that_value);
  if (number() != that.number()) return false;
  return IsEqual(type(), that.type());
}

void InheritedValue::WriteRefString(std::ostream& os) const {
  value_->WriteRefString(os);
  os << "{" << origin_ << "}";
}

bool InheritedValue::operator==(const Value& that_value) const {
  if (that_value.kind() != Value::Kind::kInherited) return false;
  auto that = static_cast<const InheritedValue&>(that_value);
  if (origin() != that.origin()) return false;
  return IsEqual(value().get(), that.value().get());
}

}  // namespace ir
