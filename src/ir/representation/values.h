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
#include <string>

#include "src/common/atomics/atomics.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/types.h"

namespace ir {

class Value {
 public:
  enum class Kind {
    kConstant,
    kComputed,
    kInherited,
  };

  constexpr virtual ~Value() {}

  constexpr virtual const Type* type() const = 0;
  constexpr virtual Kind kind() const = 0;

  virtual std::string ToString() const = 0;
  virtual std::string ToStringWithType() const { return ToString() + ":" + type()->ToString(); }
};

class Constant : public Value {
 public:
  constexpr virtual ~Constant() {}

  constexpr Value::Kind kind() const final { return Value::Kind::kConstant; };
};

class BoolConstant : public Constant {
 public:
  bool value() const { return value_; }
  const Type* type() const override { return bool_type(); }

  std::string ToString() const override { return value_ ? "#t" : "#f"; }
  std::string ToStringWithType() const override { return ToString(); }

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

  std::string ToString() const override { return "#" + value().ToString(); }

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

  std::string ToString() const override;
  std::string ToStringWithType() const override { return ToString(); }

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

  std::string ToString() const override { return "@" + std::to_string(value()); }
  std::string ToStringWithType() const override { return ToString(); }

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
  constexpr value_num_t number() const { return number_; }

  constexpr Value::Kind kind() const final { return Value::Kind::kComputed; }

  std::string ToString() const override { return "%" + std::to_string(number_); }

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

  std::string ToString() const override {
    return value_->ToString() + "{" + std::to_string(origin_) + "}";
  }

 private:
  std::shared_ptr<Value> value_;
  block_num_t origin_;
};

}  // namespace ir

#endif /* ir_values_h */
