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

#include "src/ir/representation/num_types.h"
#include "src/ir/representation/types.h"

namespace ir {

enum class ValueKind {
  kConstant,
  kComputed,
  kInherited,

  kLangStringConstant,
};

class Value {
 public:
  Value(Type* type) : type_(type) {}
  virtual ~Value() {}

  Type* type() const { return type_; }

  virtual ValueKind value_kind() const = 0;
  virtual std::string ToString() const = 0;
  virtual std::string ToStringWithType() const { return ToString() + ":" + type_->ToString(); }

 private:
  Type* type_;
};

class Constant : public Value {
 public:
  Constant(Atomic* type, int64_t value) : Value(type), value_(value) {}

  Atomic* atomic_type() const { return static_cast<Atomic*>(type()); }
  int64_t value() const { return value_; }

  ValueKind value_kind() const override { return ValueKind::kConstant; }
  std::string ToString() const override;
  std::string ToStringWithType() const override;

 private:
  int64_t value_;
};

class Computed : public Value {
 public:
  Computed(Type* type, value_num_t vnum) : Value(type), number_(vnum) {}

  value_num_t number() const { return number_; }

  ValueKind value_kind() const override { return ValueKind::kComputed; }
  std::string ToString() const override { return "%" + std::to_string(number_); }

 private:
  value_num_t number_;
};

class InheritedValue : public Value {
 public:
  InheritedValue(std::shared_ptr<Value> value, block_num_t origin)
      : Value(value->type()), value_(value), origin_(origin) {}

  std::shared_ptr<Value> value() const { return value_; }
  block_num_t origin() const { return origin_; }

  ValueKind value_kind() const override { return ValueKind::kInherited; }
  std::string ToString() const override {
    return value_->ToString() + "{" + std::to_string(origin_) + "}";
  }

 private:
  std::shared_ptr<Value> value_;
  block_num_t origin_;
};

}  // namespace ir

#endif /* ir_values_h */
