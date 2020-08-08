//
//  constants.cc
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "constant.h"

namespace lang {
namespace constant {

std::string Value::ToString() const {
    switch (value_.index()) {
        case 0:
            return std::get<bool>(value_) ? "true" : "false";
        case 1:
            return std::to_string(std::get<int8_t>(value_));
        case 2:
            return std::to_string(std::get<uint8_t>(value_));
        case 3:
            return std::to_string(std::get<int16_t>(value_));
        case 4:
            return std::to_string(std::get<uint16_t>(value_));
        case 5:
            return std::to_string(std::get<int32_t>(value_));
        case 6:
            return std::to_string(std::get<uint32_t>(value_));
        case 7:
            return std::to_string(std::get<int64_t>(value_));
        case 8:
            return std::to_string(std::get<uint64_t>(value_));
        default:
            throw "unexpected value_t";
    }
}

bool Compare(Value x, token::Token op, Value y) {
    if (x.value_.index() != y.value_.index()) {
        throw "can not compare constants of different types";
    }
    switch (x.value_.index()) {
        case 0:
            switch (op) {
                case token::kEql:
                    return std::get<bool>(x.value_) == std::get<bool>(y.value_);
                case token::kNeq:
                    return std::get<bool>(x.value_) != std::get<bool>(y.value_);
                default:
                    throw "unexpected compare op";
            }
        case 1:
            switch (op) {
                case token::kEql:
                    return std::get<int8_t>(x.value_) == std::get<int8_t>(y.value_);
                case token::kNeq:
                    return std::get<int8_t>(x.value_) != std::get<int8_t>(y.value_);
                case token::kLss:
                    return std::get<int8_t>(x.value_) < std::get<int8_t>(y.value_);
                case token::kLeq:
                    return std::get<int8_t>(x.value_) <= std::get<int8_t>(y.value_);
                case token::kGeq:
                    return std::get<int8_t>(x.value_) >= std::get<int8_t>(y.value_);
                case token::kGtr:
                    return std::get<int8_t>(x.value_) > std::get<int8_t>(y.value_);
                default:
                    throw "unexpected compare op";
            }
        case 2:
            switch (op) {
                case token::kEql:
                    return std::get<uint8_t>(x.value_) == std::get<uint8_t>(y.value_);
                case token::kNeq:
                    return std::get<uint8_t>(x.value_) != std::get<uint8_t>(y.value_);
                case token::kLss:
                    return std::get<uint8_t>(x.value_) < std::get<uint8_t>(y.value_);
                case token::kLeq:
                    return std::get<uint8_t>(x.value_) <= std::get<uint8_t>(y.value_);
                case token::kGeq:
                    return std::get<uint8_t>(x.value_) >= std::get<uint8_t>(y.value_);
                case token::kGtr:
                    return std::get<uint8_t>(x.value_) > std::get<uint8_t>(y.value_);
                default:
                    throw "unexpected compare op";
            }
        case 3:
            switch (op) {
                case token::kEql:
                    return std::get<int16_t>(x.value_) == std::get<int16_t>(y.value_);
                case token::kNeq:
                    return std::get<int16_t>(x.value_) != std::get<int16_t>(y.value_);
                case token::kLss:
                    return std::get<int16_t>(x.value_) < std::get<int16_t>(y.value_);
                case token::kLeq:
                    return std::get<int16_t>(x.value_) <= std::get<int16_t>(y.value_);
                case token::kGeq:
                    return std::get<int16_t>(x.value_) >= std::get<int16_t>(y.value_);
                case token::kGtr:
                    return std::get<int16_t>(x.value_) > std::get<int16_t>(y.value_);
                default:
                    throw "unexpected compare op";
            }
        case 4:
            switch (op) {
                case token::kEql:
                    return std::get<uint16_t>(x.value_) == std::get<uint16_t>(y.value_);
                case token::kNeq:
                    return std::get<uint16_t>(x.value_) != std::get<uint16_t>(y.value_);
                case token::kLss:
                    return std::get<uint16_t>(x.value_) < std::get<uint16_t>(y.value_);
                case token::kLeq:
                    return std::get<uint16_t>(x.value_) <= std::get<uint16_t>(y.value_);
                case token::kGeq:
                    return std::get<uint16_t>(x.value_) >= std::get<uint16_t>(y.value_);
                case token::kGtr:
                    return std::get<uint16_t>(x.value_) > std::get<uint16_t>(y.value_);
                default:
                    throw "unexpected compare op";
            }
        case 5:
            switch (op) {
                case token::kEql:
                    return std::get<int32_t>(x.value_) == std::get<int32_t>(y.value_);
                case token::kNeq:
                    return std::get<int32_t>(x.value_) != std::get<int32_t>(y.value_);
                case token::kLss:
                    return std::get<int32_t>(x.value_) < std::get<int32_t>(y.value_);
                case token::kLeq:
                    return std::get<int32_t>(x.value_) <= std::get<int32_t>(y.value_);
                case token::kGeq:
                    return std::get<int32_t>(x.value_) >= std::get<int32_t>(y.value_);
                case token::kGtr:
                    return std::get<int32_t>(x.value_) > std::get<int32_t>(y.value_);
                default:
                    throw "unexpected compare op";
            }
        case 6:
            switch (op) {
                case token::kEql:
                    return std::get<uint32_t>(x.value_) == std::get<uint32_t>(y.value_);
                case token::kNeq:
                    return std::get<uint32_t>(x.value_) != std::get<uint32_t>(y.value_);
                case token::kLss:
                    return std::get<uint32_t>(x.value_) < std::get<uint32_t>(y.value_);
                case token::kLeq:
                    return std::get<uint32_t>(x.value_) <= std::get<uint32_t>(y.value_);
                case token::kGeq:
                    return std::get<uint32_t>(x.value_) >= std::get<uint32_t>(y.value_);
                case token::kGtr:
                    return std::get<uint32_t>(x.value_) > std::get<uint32_t>(y.value_);
                default:
                    throw "unexpected compare op";
            }
        case 7:
            switch (op) {
                case token::kEql:
                    return std::get<int64_t>(x.value_) == std::get<int64_t>(y.value_);
                case token::kNeq:
                    return std::get<int64_t>(x.value_) != std::get<int64_t>(y.value_);
                case token::kLss:
                    return std::get<int64_t>(x.value_) < std::get<int64_t>(y.value_);
                case token::kLeq:
                    return std::get<int64_t>(x.value_) <= std::get<int64_t>(y.value_);
                case token::kGeq:
                    return std::get<int64_t>(x.value_) >= std::get<int64_t>(y.value_);
                case token::kGtr:
                    return std::get<int64_t>(x.value_) > std::get<int64_t>(y.value_);
                default:
                    throw "unexpected compare op";
            }
        case 8:
            switch (op) {
                case token::kEql:
                    return std::get<uint64_t>(x.value_) == std::get<uint64_t>(y.value_);
                case token::kNeq:
                    return std::get<uint64_t>(x.value_) != std::get<uint64_t>(y.value_);
                case token::kLss:
                    return std::get<uint64_t>(x.value_) < std::get<uint64_t>(y.value_);
                case token::kLeq:
                    return std::get<uint64_t>(x.value_) <= std::get<uint64_t>(y.value_);
                case token::kGeq:
                    return std::get<uint64_t>(x.value_) >= std::get<uint64_t>(y.value_);
                case token::kGtr:
                    return std::get<uint64_t>(x.value_) > std::get<uint64_t>(y.value_);
                default:
                    throw "unexpected compare op";
            }
        default:
            throw "unexpected value_t";
    }
}

Value BinaryOp(Value x, token::Token op, Value y) {
    if (x.value_.index() != y.value_.index()) {
        throw "can not apply binary op to constants of different types";
    }
    switch (x.value_.index()) {
        case 0:
            switch (op) {
                case token::kLAnd:
                    return Value(std::get<bool>(x.value_) && std::get<bool>(y.value_));
                case token::kLOr:
                    return Value(std::get<bool>(x.value_) || std::get<bool>(y.value_));
                default:
                    throw "unexpected binary op";
            }
        case 1:
            switch (op) {
                case token::kAdd:
                    return Value(std::get<int8_t>(x.value_) + std::get<int8_t>(y.value_));
                case token::kSub:
                    return Value(std::get<int8_t>(x.value_) - std::get<int8_t>(y.value_));
                case token::kMul:
                    return Value(std::get<int8_t>(x.value_) * std::get<int8_t>(y.value_));
                case token::kQuo:
                    return Value(std::get<int8_t>(x.value_) / std::get<int8_t>(y.value_));
                case token::kRem:
                    return Value(std::get<int8_t>(x.value_) % std::get<int8_t>(y.value_));
                case token::kAnd:
                    return Value(std::get<int8_t>(x.value_) & std::get<int8_t>(y.value_));
                case token::kOr:
                    return Value(std::get<int8_t>(x.value_) | std::get<int8_t>(y.value_));
                case token::kXor:
                    return Value(std::get<int8_t>(x.value_) ^ std::get<int8_t>(y.value_));
                case token::kAndNot:
                    return Value(std::get<int8_t>(x.value_) & ~std::get<int8_t>(y.value_));
                default:
                    throw "unexpected binary op";
            }
        case 2:
            switch (op) {
                case token::kAdd:
                    return Value(std::get<uint8_t>(x.value_) + std::get<uint8_t>(y.value_));
                case token::kSub:
                    return Value(std::get<uint8_t>(x.value_) - std::get<uint8_t>(y.value_));
                case token::kMul:
                    return Value(std::get<uint8_t>(x.value_) * std::get<uint8_t>(y.value_));
                case token::kQuo:
                    return Value(std::get<uint8_t>(x.value_) / std::get<uint8_t>(y.value_));
                case token::kRem:
                    return Value(std::get<uint8_t>(x.value_) % std::get<uint8_t>(y.value_));
                case token::kAnd:
                    return Value(std::get<uint8_t>(x.value_) & std::get<uint8_t>(y.value_));
                case token::kOr:
                    return Value(std::get<uint8_t>(x.value_) | std::get<uint8_t>(y.value_));
                case token::kXor:
                    return Value(std::get<uint8_t>(x.value_) ^ std::get<uint8_t>(y.value_));
                case token::kAndNot:
                    return Value(std::get<uint8_t>(x.value_) & ~std::get<uint8_t>(y.value_));
                default:
                    throw "unexpected binary op";
            }
        case 3:
            switch (op) {
                case token::kAdd:
                    return Value(std::get<int16_t>(x.value_) + std::get<int16_t>(y.value_));
                case token::kSub:
                    return Value(std::get<int16_t>(x.value_) - std::get<int16_t>(y.value_));
                case token::kMul:
                    return Value(std::get<int16_t>(x.value_) * std::get<int16_t>(y.value_));
                case token::kQuo:
                    return Value(std::get<int16_t>(x.value_) / std::get<int16_t>(y.value_));
                case token::kRem:
                    return Value(std::get<int16_t>(x.value_) % std::get<int16_t>(y.value_));
                case token::kAnd:
                    return Value(std::get<int16_t>(x.value_) & std::get<int16_t>(y.value_));
                case token::kOr:
                    return Value(std::get<int16_t>(x.value_) | std::get<int16_t>(y.value_));
                case token::kXor:
                    return Value(std::get<int16_t>(x.value_) ^ std::get<int16_t>(y.value_));
                case token::kAndNot:
                    return Value(std::get<int16_t>(x.value_) & ~std::get<int16_t>(y.value_));
                default:
                    throw "unexpected binary op";
            }
        case 4:
            switch (op) {
                case token::kAdd:
                    return Value(std::get<uint16_t>(x.value_) + std::get<uint16_t>(y.value_));
                case token::kSub:
                    return Value(std::get<uint16_t>(x.value_) - std::get<uint16_t>(y.value_));
                case token::kMul:
                    return Value(std::get<uint16_t>(x.value_) * std::get<uint16_t>(y.value_));
                case token::kQuo:
                    return Value(std::get<uint16_t>(x.value_) / std::get<uint16_t>(y.value_));
                case token::kRem:
                    return Value(std::get<uint16_t>(x.value_) % std::get<uint16_t>(y.value_));
                case token::kAnd:
                    return Value(std::get<uint16_t>(x.value_) & std::get<uint16_t>(y.value_));
                case token::kOr:
                    return Value(std::get<uint16_t>(x.value_) | std::get<uint16_t>(y.value_));
                case token::kXor:
                    return Value(std::get<uint16_t>(x.value_) ^ std::get<uint16_t>(y.value_));
                case token::kAndNot:
                    return Value(std::get<uint16_t>(x.value_) & ~std::get<uint16_t>(y.value_));
                default:
                    throw "unexpected binary op";
            }
        case 5:
            switch (op) {
                case token::kAdd:
                    return Value(std::get<int32_t>(x.value_) + std::get<int32_t>(y.value_));
                case token::kSub:
                    return Value(std::get<int32_t>(x.value_) - std::get<int32_t>(y.value_));
                case token::kMul:
                    return Value(std::get<int32_t>(x.value_) * std::get<int32_t>(y.value_));
                case token::kQuo:
                    return Value(std::get<int32_t>(x.value_) / std::get<int32_t>(y.value_));
                case token::kRem:
                    return Value(std::get<int32_t>(x.value_) % std::get<int32_t>(y.value_));
                case token::kAnd:
                    return Value(std::get<int32_t>(x.value_) & std::get<int32_t>(y.value_));
                case token::kOr:
                    return Value(std::get<int32_t>(x.value_) | std::get<int32_t>(y.value_));
                case token::kXor:
                    return Value(std::get<int32_t>(x.value_) ^ std::get<int32_t>(y.value_));
                case token::kAndNot:
                    return Value(std::get<int32_t>(x.value_) & ~std::get<int32_t>(y.value_));
                default:
                    throw "unexpected binary op";
            }
        case 6:
            switch (op) {
                case token::kAdd:
                    return Value(std::get<uint32_t>(x.value_) + std::get<uint32_t>(y.value_));
                case token::kSub:
                    return Value(std::get<uint32_t>(x.value_) - std::get<uint32_t>(y.value_));
                case token::kMul:
                    return Value(std::get<uint32_t>(x.value_) * std::get<uint32_t>(y.value_));
                case token::kQuo:
                    return Value(std::get<uint32_t>(x.value_) / std::get<uint32_t>(y.value_));
                case token::kRem:
                    return Value(std::get<uint32_t>(x.value_) % std::get<uint32_t>(y.value_));
                case token::kAnd:
                    return Value(std::get<uint32_t>(x.value_) & std::get<uint32_t>(y.value_));
                case token::kOr:
                    return Value(std::get<uint32_t>(x.value_) | std::get<uint32_t>(y.value_));
                case token::kXor:
                    return Value(std::get<uint32_t>(x.value_) ^ std::get<uint32_t>(y.value_));
                case token::kAndNot:
                    return Value(std::get<uint32_t>(x.value_) & ~std::get<uint32_t>(y.value_));
                default:
                    throw "unexpected binary op";
            }
        case 7:
            switch (op) {
                case token::kAdd:
                    return Value(std::get<int64_t>(x.value_) + std::get<int64_t>(y.value_));
                case token::kSub:
                    return Value(std::get<int64_t>(x.value_) - std::get<int64_t>(y.value_));
                case token::kMul:
                    return Value(std::get<int64_t>(x.value_) * std::get<int64_t>(y.value_));
                case token::kQuo:
                    return Value(std::get<int64_t>(x.value_) / std::get<int64_t>(y.value_));
                case token::kRem:
                    return Value(std::get<int64_t>(x.value_) % std::get<int64_t>(y.value_));
                case token::kAnd:
                    return Value(std::get<int64_t>(x.value_) & std::get<int64_t>(y.value_));
                case token::kOr:
                    return Value(std::get<int64_t>(x.value_) | std::get<int64_t>(y.value_));
                case token::kXor:
                    return Value(std::get<int64_t>(x.value_) ^ std::get<int64_t>(y.value_));
                case token::kAndNot:
                    return Value(std::get<int64_t>(x.value_) & ~std::get<int64_t>(y.value_));
                default:
                    throw "unexpected binary op";
            }
        case 8:
            switch (op) {
                case token::kAdd:
                    return Value(std::get<uint64_t>(x.value_) + std::get<uint64_t>(y.value_));
                case token::kSub:
                    return Value(std::get<uint64_t>(x.value_) - std::get<uint64_t>(y.value_));
                case token::kMul:
                    return Value(std::get<uint64_t>(x.value_) * std::get<uint64_t>(y.value_));
                case token::kQuo:
                    return Value(std::get<uint64_t>(x.value_) / std::get<uint64_t>(y.value_));
                case token::kRem:
                    return Value(std::get<uint64_t>(x.value_) % std::get<uint64_t>(y.value_));
                case token::kAnd:
                    return Value(std::get<uint64_t>(x.value_) & std::get<uint64_t>(y.value_));
                case token::kOr:
                    return Value(std::get<uint64_t>(x.value_) | std::get<uint64_t>(y.value_));
                case token::kXor:
                    return Value(std::get<uint64_t>(x.value_) ^ std::get<uint64_t>(y.value_));
                case token::kAndNot:
                    return Value(std::get<uint64_t>(x.value_) & ~std::get<uint64_t>(y.value_));
                default:
                    throw "unexpected binary op";
            }
        default:
            throw "unexpected value_t";
    }
}

Value ShiftOp(Value x, token::Token op, Value y) {
    uint64_t s = 0;
    switch (y.value_.index()) {
        case 2:
            s = std::get<uint8_t>(y.value_);
            break;
        case 4:
            s = std::get<uint16_t>(y.value_);
            break;
        case 6:
            s = std::get<uint32_t>(y.value_);
            break;
        case 8:
            s = std::get<uint64_t>(y.value_);
            break;
        default:
            throw "unexpected value_t of shift operand";
    }
    switch (x.value_.index()) {
        case 1:
            switch (op) {
                case token::kShl:
                    return Value(std::get<int8_t>(x.value_) << s);
                case token::kShr:
                    return Value(std::get<int8_t>(x.value_) >> s);
                default:
                    throw "unexpected shift op";
            }
        case 2:
            switch (op) {
                case token::kShl:
                    return Value(std::get<uint8_t>(x.value_) << s);
                case token::kShr:
                    return Value(std::get<uint8_t>(x.value_) >> s);
                default:
                    throw "unexpected shift op";
            }
        case 3:
            switch (op) {
                case token::kShl:
                    return Value(std::get<int16_t>(x.value_) << s);
                case token::kShr:
                    return Value(std::get<int16_t>(x.value_) >> s);
                default:
                    throw "unexpected shift op";
            }
        case 4:
            switch (op) {
                case token::kShl:
                    return Value(std::get<uint16_t>(x.value_) << s);
                case token::kShr:
                    return Value(std::get<uint16_t>(x.value_) >> s);
                default:
                    throw "unexpected shift op";
            }
        case 5:
            switch (op) {
                case token::kShl:
                    return Value(std::get<int32_t>(x.value_) << s);
                case token::kShr:
                    return Value(std::get<int32_t>(x.value_) >> s);
                default:
                    throw "unexpected shift op";
            }
        case 6:
            switch (op) {
                case token::kShl:
                    return Value(std::get<uint32_t>(x.value_) << s);
                case token::kShr:
                    return Value(std::get<uint32_t>(x.value_) >> s);
                default:
                    throw "unexpected shift op";
            }
        case 7:
            switch (op) {
                case token::kShl:
                    return Value(std::get<int64_t>(x.value_) << s);
                case token::kShr:
                    return Value(std::get<int64_t>(x.value_) >> s);
                default:
                    throw "unexpected shift op";
            }
        case 8:
            switch (op) {
                case token::kShl:
                    return Value(std::get<uint64_t>(x.value_) << s);
                case token::kShr:
                    return Value(std::get<uint64_t>(x.value_) >> s);
                default:
                    throw "unexpected shift op";
            }
        default:
            throw "unexpected value_t of shifted operand";
    }
}

Value UnaryOp(token::Token op, Value x) {
    switch (x.value_.index()) {
        case 0:
            switch (op) {
                case token::kNot:
                    return Value(!std::get<bool>(x.value_));
                default:
                    throw "unexpected unary op";
            }
        case 1:
            switch (op) {
                case token::kAdd:
                    return x;
                case token::kSub:
                    return Value(-std::get<int8_t>(x.value_));
                case token::kXor:
                    return Value(~std::get<int8_t>(x.value_));
                default:
                    throw "unexpected unary op";
            }
        case 2:
            switch (op) {
                case token::kAdd:
                    return x;
                case token::kSub:
                    return Value(-std::get<uint8_t>(x.value_));
                case token::kXor:
                    return Value(~std::get<uint8_t>(x.value_));
                default:
                    throw "unexpected unary op";
            }
        case 3:
            switch (op) {
                case token::kAdd:
                    return x;
                case token::kSub:
                    return Value(-std::get<int16_t>(x.value_));
                case token::kXor:
                    return Value(~std::get<int16_t>(x.value_));
                default:
                    throw "unexpected unary op";
            }
        case 4:
            switch (op) {
                case token::kAdd:
                    return x;
                case token::kSub:
                    return Value(-std::get<uint16_t>(x.value_));
                case token::kXor:
                    return Value(~std::get<uint16_t>(x.value_));
                default:
                    throw "unexpected unary op";
            }
        case 5:
            switch (op) {
                case token::kAdd:
                    return x;
                case token::kSub:
                    return Value(-std::get<int32_t>(x.value_));
                case token::kXor:
                    return Value(~std::get<int32_t>(x.value_));
                default:
                    throw "unexpected unary op";
            }
        case 6:
            switch (op) {
                case token::kAdd:
                    return x;
                case token::kSub:
                    return Value(-std::get<uint32_t>(x.value_));
                case token::kXor:
                    return Value(~std::get<uint32_t>(x.value_));
                default:
                    throw "unexpected unary op";
            }
        case 7:
            switch (op) {
                case token::kAdd:
                    return x;
                case token::kSub:
                    return Value(-std::get<int64_t>(x.value_));
                case token::kXor:
                    return Value(~std::get<int64_t>(x.value_));
                default:
                    throw "unexpected unary op";
            }
        case 8:
            switch (op) {
                case token::kAdd:
                    return x;
                case token::kSub:
                    return Value(-std::get<uint64_t>(x.value_));
                case token::kXor:
                    return Value(~std::get<uint64_t>(x.value_));
                default:
                    throw "unexpected unary op";
            }
        default:
            throw "unexpected value_t";
    }
}

}
}
