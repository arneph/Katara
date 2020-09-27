//
//  constant.h
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_constant_h
#define lang_constant_h

#include <memory>
#include <string>
#include <variant>

#include "lang/token.h"

namespace lang {
namespace constant {

typedef std::variant<bool,
                     int8_t, uint8_t,
                     int16_t, uint16_t,
                     int32_t, uint32_t,
                     int64_t, uint64_t,
                     std::string> value_t;

class Value {
public:
    Value(value_t value) : value_(value) {}
    
    std::string ToString() const;
    
    value_t value_;
};

bool Compare(Value x, token::Token op, Value y);
Value BinaryOp(Value x, token::Token op, Value y);
Value ShiftOp(Value x, token::Token op, Value y);
Value UnaryOp(token::Token op, Value x);

}
}

#endif /* lang_constant_h */
