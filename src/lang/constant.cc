//
//  constants.cc
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "constant.h"

#include <limits>

namespace lang {
namespace constant {

Value::Value(Kind kind) : kind_(kind), sign_(kPlus), abs_(0) {}
Value::Value(bool b) : kind_(kBool), sign_(kPlus), abs_(b) {}
Value::Value(uint64_t x) : kind_(kInt), sign_(kPlus), abs_(x) {}
Value::Value(int64_t x) : kind_(kInt), sign_(x >= 0), abs_(uint64_t(abs(x))) {}

Kind Value::kind() const {
    return kind_;
}

bool Value::AsBool() const {
    return abs_ != 0;
}

bool Value::IsPreciseUint64() const {
    return sign_ == kPlus;
}

uint64_t Value::AsUint64() const {
    return abs_;
}

bool Value::IsPreciseInt64() const {
    if (sign_ == kPlus) {
        return abs_ <= +std::numeric_limits<int64_t>::max();
    } else {
        return abs_ <= -std::numeric_limits<int64_t>::min();
    }
}

int64_t Value::AsInt64() const {
    if (sign_ == kPlus) {
        return +1 * int64_t(abs_);
    } else {
        return -1 * int64_t(abs_);
    }
}

std::string Value::ToString() const {
    switch (kind_) {
        case kBool:
            return (abs_) ? "true" : "false";
        case kInt:
            return ((sign_) ? "-" : "") + std::to_string(abs_);
    }
}

}
}
