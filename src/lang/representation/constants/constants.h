//
//  constants.h
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_constants_h
#define lang_constants_h

#include <memory>
#include <string>
#include <variant>

#include "src/common/atomics/atomics.h"
#include "src/lang/representation/tokens/tokens.h"

namespace lang {
namespace constants {

class Value {
 public:
  enum class Kind {
    kBool,
    kInt,
    kString,
  };

  explicit Value(bool value) : value_(value) {}
  explicit Value(common::atomics::Int value) : value_(value) {}
  explicit Value(std::string value) : value_(value) {}

  Kind kind() const { return Kind(value_.index()); }
  bool AsBool() const { return std::get<bool>(value_); }
  common::atomics::Int AsInt() const { return std::get<common::atomics::Int>(value_); }
  std::string AsString() const { return std::get<std::string>(value_); }

  std::string ToString() const;

 private:
  typedef std::variant<bool, common::atomics::Int, std::string> value_t;

  value_t value_;
};

bool Compare(Value x, tokens::Token op, Value y);
Value BinaryOp(Value x, tokens::Token op, Value y);
Value ShiftOp(Value x, tokens::Token op, Value y);
Value UnaryOp(tokens::Token op, Value x);

}  // namespace constants
}  // namespace lang

#endif /* lang_constants_h */
