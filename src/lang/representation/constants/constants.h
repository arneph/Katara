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

#include "lang/representation/tokens/tokens.h"

namespace lang {
namespace constants {

typedef std::variant<bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t,
                     std::string>
    value_t;

class Value {
 public:
  Value(value_t value) : value_(value) {}

  bool CanConvertToArraySize() const;
  uint64_t ConvertToArraySize() const;

  std::string ToString() const;

  value_t value_;
};

bool Compare(Value x, tokens::Token op, Value y);
Value BinaryOp(Value x, tokens::Token op, Value y);
Value ShiftOp(Value x, tokens::Token op, Value y);
Value UnaryOp(tokens::Token op, Value x);

template <typename ConvertedType>
Value Convert(Value x) {
  switch (x.value_.index()) {
    case 1:
      return Value(static_cast<ConvertedType>(std::get<int8_t>(x.value_)));
    case 2:
      return Value(static_cast<ConvertedType>(std::get<uint8_t>(x.value_)));
    case 3:
      return Value(static_cast<ConvertedType>(std::get<int16_t>(x.value_)));
    case 4:
      return Value(static_cast<ConvertedType>(std::get<uint16_t>(x.value_)));
    case 5:
      return Value(static_cast<ConvertedType>(std::get<int32_t>(x.value_)));
    case 6:
      return Value(static_cast<ConvertedType>(std::get<uint32_t>(x.value_)));
    case 7:
      return Value(static_cast<ConvertedType>(std::get<int64_t>(x.value_)));
    case 8:
      return Value(static_cast<ConvertedType>(std::get<uint64_t>(x.value_)));
    default:
      throw "unexpected value_t";
  }
}

template <>
Value Convert<bool>(Value x);

template <>
Value Convert<std::string>(Value x);

}  // namespace constants
}  // namespace lang

#endif /* lang_constants_h */
