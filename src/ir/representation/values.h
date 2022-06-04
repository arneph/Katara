//
//  values.h
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_values_h
#define ir_values_h

#include <memory>
#include <ostream>
#include <string>

#include "src/common/atomics/atomics.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/types.h"

namespace ir {

class Value : public Object {
 public:
  enum class Kind {
    kConstant,
    kComputed,
    kInherited,
  };

  constexpr virtual ~Value() {}

  constexpr Object::Kind object_kind() const final { return Object::Kind::kValue; }
  constexpr virtual const Type* type() const = 0;
  constexpr virtual Kind kind() const = 0;

  virtual void WriteRefStringWithType(std::ostream& os) const;
  std::string RefStringWithType() const;

  constexpr virtual bool operator==(const Value& that) const = 0;
};

constexpr bool IsEqual(const Value* value_a, const Value* value_b) {
  if (value_a == value_b) return true;
  if (value_a == nullptr || value_b == nullptr) return false;
  return *value_a == *value_b;
}

class Constant : public Value {
 public:
  constexpr virtual ~Constant() {}

  constexpr Value::Kind kind() const final { return Value::Kind::kConstant; };
};

class BoolConstant : public Constant {
 public:
  bool value() const { return value_; }
  const Type* type() const override { return bool_type(); }

  void WriteRefString(std::ostream& os) const override { os << (value_ ? "#t" : "#f"); }
  void WriteRefStringWithType(std::ostream& os) const override { WriteRefString(os); }

  bool operator==(const Value& that) const override;

 private:
  BoolConstant(bool value) : value_(value) {}

  bool value_;

  friend std::shared_ptr<BoolConstant> False();
  friend std::shared_ptr<BoolConstant> True();
};

std::shared_ptr<BoolConstant> False();
std::shared_ptr<BoolConstant> True();
std::shared_ptr<BoolConstant> ToBoolConstant(bool value);

class IntConstant : public Constant {
 public:
  common::Int value() const { return value_; }
  common::IntType int_type() const { return value_.type(); }
  const Type* type() const override { return IntTypeFor(value_.type()); }

  void WriteRefString(std::ostream& os) const override { os << "#" << value().ToString(); }

  bool operator==(const Value& that) const override;

 private:
  IntConstant(common::Int value) : value_(value) {}

  common::Int value_;

  friend std::shared_ptr<IntConstant> MakeIntConstant(common::Int value);
};

std::shared_ptr<IntConstant> I8Zero();
std::shared_ptr<IntConstant> I16Zero();
std::shared_ptr<IntConstant> I32Zero();
std::shared_ptr<IntConstant> I64Zero();
std::shared_ptr<IntConstant> I64One();
std::shared_ptr<IntConstant> I64Eight();

std::shared_ptr<IntConstant> U8Zero();
std::shared_ptr<IntConstant> U16Zero();
std::shared_ptr<IntConstant> U32Zero();
std::shared_ptr<IntConstant> U64Zero();

std::shared_ptr<IntConstant> ZeroWithType(common::IntType type);
std::shared_ptr<IntConstant> ToIntConstant(common::Int value);

class PointerConstant : public Constant {
 public:
  int64_t value() const { return value_; }
  const Type* type() const override { return pointer_type(); }

  void WriteRefString(std::ostream& os) const override;
  void WriteRefStringWithType(std::ostream& os) const override { WriteRefString(os); }

  bool operator==(const Value& that) const override;

 private:
  PointerConstant(int64_t value) : value_(value) {}

  int64_t value_;

  friend std::shared_ptr<PointerConstant> MakePointerConstant(int64_t value);
};

std::shared_ptr<PointerConstant> NilPointer();
std::shared_ptr<PointerConstant> ToPointerConstant(int64_t value);

class FuncConstant : public Constant {
 public:
  func_num_t value() const { return value_; }
  const Type* type() const override { return func_type(); }

  void WriteRefString(std::ostream& os) const override { os << "@" << value(); }
  void WriteRefStringWithType(std::ostream& os) const override { WriteRefString(os); }

  bool operator==(const Value& that) const override;

 private:
  FuncConstant(func_num_t value) : value_(value) {}

  func_num_t value_;

  friend std::shared_ptr<FuncConstant> MakeFuncConstant(func_num_t value);
};

std::shared_ptr<FuncConstant> NilFunc();
std::shared_ptr<FuncConstant> ToFuncConstant(func_num_t value);

class Computed : public Value {
 public:
  constexpr Computed(const Type* type, value_num_t vnum) : type_(type), number_(vnum) {}

  constexpr const Type* type() const override { return type_; }
  constexpr void set_type(const Type* type) { type_ = type; }
  constexpr value_num_t number() const { return number_; }

  constexpr Value::Kind kind() const final { return Value::Kind::kComputed; }

  void WriteRefString(std::ostream& os) const override { os << "%" << number_; }

  bool operator==(const Value& that) const override;

 private:
  const Type* type_;
  value_num_t number_;
};

class InheritedValue : public Value {
 public:
  InheritedValue(std::shared_ptr<Value> value, block_num_t origin)
      : value_(value), origin_(origin) {}

  const Type* type() const override { return value_->type(); }
  std::shared_ptr<Value> value() const { return value_; }
  block_num_t origin() const { return origin_; }

  constexpr Value::Kind kind() const final { return Value::Kind::kInherited; }

  void WriteRefString(std::ostream& os) const override;

  bool operator==(const Value& that) const override;

 private:
  std::shared_ptr<Value> value_;
  block_num_t origin_;
};

}  // namespace ir

#endif /* ir_values_h */
