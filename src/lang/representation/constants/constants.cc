//
//  constants.cc
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "constants.h"

namespace lang {
namespace constants {

bool Value::CanConvertToArraySize() const {
  switch (value_.index()) {
    case 0:
      return false;
    case 1:
      return std::get<int8_t>(value_) >= 0;
    case 2:
      return true;
    case 3:
      return std::get<int16_t>(value_) >= 0;
    case 4:
      return true;
    case 5:
      return std::get<int32_t>(value_) >= 0;
    case 6:
      return true;
    case 7:
      return std::get<int64_t>(value_) >= 0;
    case 8:
      return true;
    case 9:
      return false;
    default:
      throw "unexpected value_t";
  }
}

uint64_t Value::ConvertToArraySize() const {
  switch (value_.index()) {
    case 1:
      return std::get<int8_t>(value_);
    case 2:
      return std::get<uint8_t>(value_);
    case 3:
      return std::get<int16_t>(value_);
    case 4:
      return std::get<uint16_t>(value_);
    case 5:
      return std::get<int32_t>(value_);
    case 6:
      return std::get<uint32_t>(value_);
    case 7:
      return std::get<int64_t>(value_);
    case 8:
      return std::get<uint64_t>(value_);
    default:
      throw "unexpected value_t";
  }
}

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
    case 9:
      return std::get<std::string>(value_);
    default:
      throw "unexpected value_t";
  }
}

bool Compare(Value x, tokens::Token op, Value y) {
  if (x.value_.index() != y.value_.index()) {
    throw "can not compare constants of different types";
  }
  switch (x.value_.index()) {
    case 0:
      switch (op) {
        case tokens::kEql:
          return std::get<bool>(x.value_) == std::get<bool>(y.value_);
        case tokens::kNeq:
          return std::get<bool>(x.value_) != std::get<bool>(y.value_);
        default:
          throw "unexpected compare op";
      }
    case 1:
      switch (op) {
        case tokens::kEql:
          return std::get<int8_t>(x.value_) == std::get<int8_t>(y.value_);
        case tokens::kNeq:
          return std::get<int8_t>(x.value_) != std::get<int8_t>(y.value_);
        case tokens::kLss:
          return std::get<int8_t>(x.value_) < std::get<int8_t>(y.value_);
        case tokens::kLeq:
          return std::get<int8_t>(x.value_) <= std::get<int8_t>(y.value_);
        case tokens::kGeq:
          return std::get<int8_t>(x.value_) >= std::get<int8_t>(y.value_);
        case tokens::kGtr:
          return std::get<int8_t>(x.value_) > std::get<int8_t>(y.value_);
        default:
          throw "unexpected compare op";
      }
    case 2:
      switch (op) {
        case tokens::kEql:
          return std::get<uint8_t>(x.value_) == std::get<uint8_t>(y.value_);
        case tokens::kNeq:
          return std::get<uint8_t>(x.value_) != std::get<uint8_t>(y.value_);
        case tokens::kLss:
          return std::get<uint8_t>(x.value_) < std::get<uint8_t>(y.value_);
        case tokens::kLeq:
          return std::get<uint8_t>(x.value_) <= std::get<uint8_t>(y.value_);
        case tokens::kGeq:
          return std::get<uint8_t>(x.value_) >= std::get<uint8_t>(y.value_);
        case tokens::kGtr:
          return std::get<uint8_t>(x.value_) > std::get<uint8_t>(y.value_);
        default:
          throw "unexpected compare op";
      }
    case 3:
      switch (op) {
        case tokens::kEql:
          return std::get<int16_t>(x.value_) == std::get<int16_t>(y.value_);
        case tokens::kNeq:
          return std::get<int16_t>(x.value_) != std::get<int16_t>(y.value_);
        case tokens::kLss:
          return std::get<int16_t>(x.value_) < std::get<int16_t>(y.value_);
        case tokens::kLeq:
          return std::get<int16_t>(x.value_) <= std::get<int16_t>(y.value_);
        case tokens::kGeq:
          return std::get<int16_t>(x.value_) >= std::get<int16_t>(y.value_);
        case tokens::kGtr:
          return std::get<int16_t>(x.value_) > std::get<int16_t>(y.value_);
        default:
          throw "unexpected compare op";
      }
    case 4:
      switch (op) {
        case tokens::kEql:
          return std::get<uint16_t>(x.value_) == std::get<uint16_t>(y.value_);
        case tokens::kNeq:
          return std::get<uint16_t>(x.value_) != std::get<uint16_t>(y.value_);
        case tokens::kLss:
          return std::get<uint16_t>(x.value_) < std::get<uint16_t>(y.value_);
        case tokens::kLeq:
          return std::get<uint16_t>(x.value_) <= std::get<uint16_t>(y.value_);
        case tokens::kGeq:
          return std::get<uint16_t>(x.value_) >= std::get<uint16_t>(y.value_);
        case tokens::kGtr:
          return std::get<uint16_t>(x.value_) > std::get<uint16_t>(y.value_);
        default:
          throw "unexpected compare op";
      }
    case 5:
      switch (op) {
        case tokens::kEql:
          return std::get<int32_t>(x.value_) == std::get<int32_t>(y.value_);
        case tokens::kNeq:
          return std::get<int32_t>(x.value_) != std::get<int32_t>(y.value_);
        case tokens::kLss:
          return std::get<int32_t>(x.value_) < std::get<int32_t>(y.value_);
        case tokens::kLeq:
          return std::get<int32_t>(x.value_) <= std::get<int32_t>(y.value_);
        case tokens::kGeq:
          return std::get<int32_t>(x.value_) >= std::get<int32_t>(y.value_);
        case tokens::kGtr:
          return std::get<int32_t>(x.value_) > std::get<int32_t>(y.value_);
        default:
          throw "unexpected compare op";
      }
    case 6:
      switch (op) {
        case tokens::kEql:
          return std::get<uint32_t>(x.value_) == std::get<uint32_t>(y.value_);
        case tokens::kNeq:
          return std::get<uint32_t>(x.value_) != std::get<uint32_t>(y.value_);
        case tokens::kLss:
          return std::get<uint32_t>(x.value_) < std::get<uint32_t>(y.value_);
        case tokens::kLeq:
          return std::get<uint32_t>(x.value_) <= std::get<uint32_t>(y.value_);
        case tokens::kGeq:
          return std::get<uint32_t>(x.value_) >= std::get<uint32_t>(y.value_);
        case tokens::kGtr:
          return std::get<uint32_t>(x.value_) > std::get<uint32_t>(y.value_);
        default:
          throw "unexpected compare op";
      }
    case 7:
      switch (op) {
        case tokens::kEql:
          return std::get<int64_t>(x.value_) == std::get<int64_t>(y.value_);
        case tokens::kNeq:
          return std::get<int64_t>(x.value_) != std::get<int64_t>(y.value_);
        case tokens::kLss:
          return std::get<int64_t>(x.value_) < std::get<int64_t>(y.value_);
        case tokens::kLeq:
          return std::get<int64_t>(x.value_) <= std::get<int64_t>(y.value_);
        case tokens::kGeq:
          return std::get<int64_t>(x.value_) >= std::get<int64_t>(y.value_);
        case tokens::kGtr:
          return std::get<int64_t>(x.value_) > std::get<int64_t>(y.value_);
        default:
          throw "unexpected compare op";
      }
    case 8:
      switch (op) {
        case tokens::kEql:
          return std::get<uint64_t>(x.value_) == std::get<uint64_t>(y.value_);
        case tokens::kNeq:
          return std::get<uint64_t>(x.value_) != std::get<uint64_t>(y.value_);
        case tokens::kLss:
          return std::get<uint64_t>(x.value_) < std::get<uint64_t>(y.value_);
        case tokens::kLeq:
          return std::get<uint64_t>(x.value_) <= std::get<uint64_t>(y.value_);
        case tokens::kGeq:
          return std::get<uint64_t>(x.value_) >= std::get<uint64_t>(y.value_);
        case tokens::kGtr:
          return std::get<uint64_t>(x.value_) > std::get<uint64_t>(y.value_);
        default:
          throw "unexpected compare op";
      }
    case 9:
      switch (op) {
        case tokens::kEql:
          return std::get<std::string>(x.value_) == std::get<std::string>(y.value_);
        case tokens::kNeq:
          return std::get<std::string>(x.value_) != std::get<std::string>(y.value_);
        case tokens::kLss:
          return std::get<std::string>(x.value_) < std::get<std::string>(y.value_);
        case tokens::kLeq:
          return std::get<std::string>(x.value_) <= std::get<std::string>(y.value_);
        case tokens::kGeq:
          return std::get<std::string>(x.value_) >= std::get<std::string>(y.value_);
        case tokens::kGtr:
          return std::get<std::string>(x.value_) > std::get<std::string>(y.value_);
        default:
          throw "unexpected compare op";
      }
    default:
      throw "unexpected value_t";
  }
}

Value BinaryOp(Value x, tokens::Token op, Value y) {
  if (x.value_.index() != y.value_.index()) {
    throw "can not apply binary op to constants of different types";
  }
  switch (x.value_.index()) {
    case 0:
      switch (op) {
        case tokens::kLAnd:
          return Value(std::get<bool>(x.value_) && std::get<bool>(y.value_));
        case tokens::kLOr:
          return Value(std::get<bool>(x.value_) || std::get<bool>(y.value_));
        default:
          throw "unexpected binary op";
      }
    case 1:
      switch (op) {
        case tokens::kAdd:
          return Value(int8_t(std::get<int8_t>(x.value_) + std::get<int8_t>(y.value_)));
        case tokens::kSub:
          return Value(int8_t(std::get<int8_t>(x.value_) - std::get<int8_t>(y.value_)));
        case tokens::kMul:
          return Value(int8_t(std::get<int8_t>(x.value_) * std::get<int8_t>(y.value_)));
        case tokens::kQuo:
          return Value(int8_t(std::get<int8_t>(x.value_) / std::get<int8_t>(y.value_)));
        case tokens::kRem:
          return Value(int8_t(std::get<int8_t>(x.value_) % std::get<int8_t>(y.value_)));
        case tokens::kAnd:
          return Value(int8_t(std::get<int8_t>(x.value_) & std::get<int8_t>(y.value_)));
        case tokens::kOr:
          return Value(int8_t(std::get<int8_t>(x.value_) | std::get<int8_t>(y.value_)));
        case tokens::kXor:
          return Value(int8_t(std::get<int8_t>(x.value_) ^ std::get<int8_t>(y.value_)));
        case tokens::kAndNot:
          return Value(int8_t(std::get<int8_t>(x.value_) & ~std::get<int8_t>(y.value_)));
        default:
          throw "unexpected binary op";
      }
    case 2:
      switch (op) {
        case tokens::kAdd:
          return Value(uint8_t(std::get<uint8_t>(x.value_) + std::get<uint8_t>(y.value_)));
        case tokens::kSub:
          return Value(uint8_t(std::get<uint8_t>(x.value_) - std::get<uint8_t>(y.value_)));
        case tokens::kMul:
          return Value(uint8_t(std::get<uint8_t>(x.value_) * std::get<uint8_t>(y.value_)));
        case tokens::kQuo:
          return Value(uint8_t(std::get<uint8_t>(x.value_) / std::get<uint8_t>(y.value_)));
        case tokens::kRem:
          return Value(uint8_t(std::get<uint8_t>(x.value_) % std::get<uint8_t>(y.value_)));
        case tokens::kAnd:
          return Value(uint8_t(std::get<uint8_t>(x.value_) & std::get<uint8_t>(y.value_)));
        case tokens::kOr:
          return Value(uint8_t(std::get<uint8_t>(x.value_) | std::get<uint8_t>(y.value_)));
        case tokens::kXor:
          return Value(uint8_t(std::get<uint8_t>(x.value_) ^ std::get<uint8_t>(y.value_)));
        case tokens::kAndNot:
          return Value(uint8_t(std::get<uint8_t>(x.value_) & ~std::get<uint8_t>(y.value_)));
        default:
          throw "unexpected binary op";
      }
    case 3:
      switch (op) {
        case tokens::kAdd:
          return Value(int16_t(std::get<int16_t>(x.value_) + std::get<int16_t>(y.value_)));
        case tokens::kSub:
          return Value(int16_t(std::get<int16_t>(x.value_) - std::get<int16_t>(y.value_)));
        case tokens::kMul:
          return Value(int16_t(std::get<int16_t>(x.value_) * std::get<int16_t>(y.value_)));
        case tokens::kQuo:
          return Value(int16_t(std::get<int16_t>(x.value_) / std::get<int16_t>(y.value_)));
        case tokens::kRem:
          return Value(int16_t(std::get<int16_t>(x.value_) % std::get<int16_t>(y.value_)));
        case tokens::kAnd:
          return Value(int16_t(std::get<int16_t>(x.value_) & std::get<int16_t>(y.value_)));
        case tokens::kOr:
          return Value(int16_t(std::get<int16_t>(x.value_) | std::get<int16_t>(y.value_)));
        case tokens::kXor:
          return Value(int16_t(std::get<int16_t>(x.value_) ^ std::get<int16_t>(y.value_)));
        case tokens::kAndNot:
          return Value(int16_t(std::get<int16_t>(x.value_) & ~std::get<int16_t>(y.value_)));
        default:
          throw "unexpected binary op";
      }
    case 4:
      switch (op) {
        case tokens::kAdd:
          return Value(uint16_t(std::get<uint16_t>(x.value_) + std::get<uint16_t>(y.value_)));
        case tokens::kSub:
          return Value(uint16_t(std::get<uint16_t>(x.value_) - std::get<uint16_t>(y.value_)));
        case tokens::kMul:
          return Value(uint16_t(std::get<uint16_t>(x.value_) * std::get<uint16_t>(y.value_)));
        case tokens::kQuo:
          return Value(uint16_t(std::get<uint16_t>(x.value_) / std::get<uint16_t>(y.value_)));
        case tokens::kRem:
          return Value(uint16_t(std::get<uint16_t>(x.value_) % std::get<uint16_t>(y.value_)));
        case tokens::kAnd:
          return Value(uint16_t(std::get<uint16_t>(x.value_) & std::get<uint16_t>(y.value_)));
        case tokens::kOr:
          return Value(uint16_t(std::get<uint16_t>(x.value_) | std::get<uint16_t>(y.value_)));
        case tokens::kXor:
          return Value(uint16_t(std::get<uint16_t>(x.value_) ^ std::get<uint16_t>(y.value_)));
        case tokens::kAndNot:
          return Value(uint16_t(std::get<uint16_t>(x.value_) & ~std::get<uint16_t>(y.value_)));
        default:
          throw "unexpected binary op";
      }
    case 5:
      switch (op) {
        case tokens::kAdd:
          return Value(int32_t(std::get<int32_t>(x.value_) + std::get<int32_t>(y.value_)));
        case tokens::kSub:
          return Value(int32_t(std::get<int32_t>(x.value_) - std::get<int32_t>(y.value_)));
        case tokens::kMul:
          return Value(int32_t(std::get<int32_t>(x.value_) * std::get<int32_t>(y.value_)));
        case tokens::kQuo:
          return Value(int32_t(std::get<int32_t>(x.value_) / std::get<int32_t>(y.value_)));
        case tokens::kRem:
          return Value(int32_t(std::get<int32_t>(x.value_) % std::get<int32_t>(y.value_)));
        case tokens::kAnd:
          return Value(int32_t(std::get<int32_t>(x.value_) & std::get<int32_t>(y.value_)));
        case tokens::kOr:
          return Value(int32_t(std::get<int32_t>(x.value_) | std::get<int32_t>(y.value_)));
        case tokens::kXor:
          return Value(int32_t(std::get<int32_t>(x.value_) ^ std::get<int32_t>(y.value_)));
        case tokens::kAndNot:
          return Value(int32_t(std::get<int32_t>(x.value_) & ~std::get<int32_t>(y.value_)));
        default:
          throw "unexpected binary op";
      }
    case 6:
      switch (op) {
        case tokens::kAdd:
          return Value(uint32_t(std::get<uint32_t>(x.value_) + std::get<uint32_t>(y.value_)));
        case tokens::kSub:
          return Value(uint32_t(std::get<uint32_t>(x.value_) - std::get<uint32_t>(y.value_)));
        case tokens::kMul:
          return Value(uint32_t(std::get<uint32_t>(x.value_) * std::get<uint32_t>(y.value_)));
        case tokens::kQuo:
          return Value(uint32_t(std::get<uint32_t>(x.value_) / std::get<uint32_t>(y.value_)));
        case tokens::kRem:
          return Value(uint32_t(std::get<uint32_t>(x.value_) % std::get<uint32_t>(y.value_)));
        case tokens::kAnd:
          return Value(uint32_t(std::get<uint32_t>(x.value_) & std::get<uint32_t>(y.value_)));
        case tokens::kOr:
          return Value(uint32_t(std::get<uint32_t>(x.value_) | std::get<uint32_t>(y.value_)));
        case tokens::kXor:
          return Value(uint32_t(std::get<uint32_t>(x.value_) ^ std::get<uint32_t>(y.value_)));
        case tokens::kAndNot:
          return Value(uint32_t(std::get<uint32_t>(x.value_) & ~std::get<uint32_t>(y.value_)));
        default:
          throw "unexpected binary op";
      }
    case 7:
      switch (op) {
        case tokens::kAdd:
          return Value(int64_t(std::get<int64_t>(x.value_) + std::get<int64_t>(y.value_)));
        case tokens::kSub:
          return Value(int64_t(std::get<int64_t>(x.value_) - std::get<int64_t>(y.value_)));
        case tokens::kMul:
          return Value(int64_t(std::get<int64_t>(x.value_) * std::get<int64_t>(y.value_)));
        case tokens::kQuo:
          return Value(int64_t(std::get<int64_t>(x.value_) / std::get<int64_t>(y.value_)));
        case tokens::kRem:
          return Value(int64_t(std::get<int64_t>(x.value_) % std::get<int64_t>(y.value_)));
        case tokens::kAnd:
          return Value(int64_t(std::get<int64_t>(x.value_) & std::get<int64_t>(y.value_)));
        case tokens::kOr:
          return Value(int64_t(std::get<int64_t>(x.value_) | std::get<int64_t>(y.value_)));
        case tokens::kXor:
          return Value(int64_t(std::get<int64_t>(x.value_) ^ std::get<int64_t>(y.value_)));
        case tokens::kAndNot:
          return Value(int64_t(std::get<int64_t>(x.value_) & ~std::get<int64_t>(y.value_)));
        default:
          throw "unexpected binary op";
      }
    case 8:
      switch (op) {
        case tokens::kAdd:
          return Value(uint64_t(std::get<uint64_t>(x.value_) + std::get<uint64_t>(y.value_)));
        case tokens::kSub:
          return Value(uint64_t(std::get<uint64_t>(x.value_) - std::get<uint64_t>(y.value_)));
        case tokens::kMul:
          return Value(uint64_t(std::get<uint64_t>(x.value_) * std::get<uint64_t>(y.value_)));
        case tokens::kQuo:
          return Value(uint64_t(std::get<uint64_t>(x.value_) / std::get<uint64_t>(y.value_)));
        case tokens::kRem:
          return Value(uint64_t(std::get<uint64_t>(x.value_) % std::get<uint64_t>(y.value_)));
        case tokens::kAnd:
          return Value(uint64_t(std::get<uint64_t>(x.value_) & std::get<uint64_t>(y.value_)));
        case tokens::kOr:
          return Value(uint64_t(std::get<uint64_t>(x.value_) | std::get<uint64_t>(y.value_)));
        case tokens::kXor:
          return Value(uint64_t(std::get<uint64_t>(x.value_) ^ std::get<uint64_t>(y.value_)));
        case tokens::kAndNot:
          return Value(uint64_t(std::get<uint64_t>(x.value_) & ~std::get<uint64_t>(y.value_)));
        default:
          throw "unexpected binary op";
      }
    case 9:
      switch (op) {
        case tokens::kAdd:
          return Value(std::get<std::string>(x.value_) + std::get<std::string>(y.value_));
        default:
          throw "unexpected binary op";
      }
    default:
      throw "unexpected value_t";
  }
}

Value ShiftOp(Value x, tokens::Token op, Value y) {
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
        case tokens::kShl:
          return Value(int8_t(std::get<int8_t>(x.value_) << s));
        case tokens::kShr:
          return Value(int8_t(std::get<int8_t>(x.value_) >> s));
        default:
          throw "unexpected shift op";
      }
    case 2:
      switch (op) {
        case tokens::kShl:
          return Value(uint8_t(std::get<uint8_t>(x.value_) << s));
        case tokens::kShr:
          return Value(uint8_t(std::get<uint8_t>(x.value_) >> s));
        default:
          throw "unexpected shift op";
      }
    case 3:
      switch (op) {
        case tokens::kShl:
          return Value(int16_t(std::get<int16_t>(x.value_) << s));
        case tokens::kShr:
          return Value(int16_t(std::get<int16_t>(x.value_) >> s));
        default:
          throw "unexpected shift op";
      }
    case 4:
      switch (op) {
        case tokens::kShl:
          return Value(uint16_t(std::get<uint16_t>(x.value_) << s));
        case tokens::kShr:
          return Value(uint16_t(std::get<uint16_t>(x.value_) >> s));
        default:
          throw "unexpected shift op";
      }
    case 5:
      switch (op) {
        case tokens::kShl:
          return Value(int32_t(std::get<int32_t>(x.value_) << s));
        case tokens::kShr:
          return Value(int32_t(std::get<int32_t>(x.value_) >> s));
        default:
          throw "unexpected shift op";
      }
    case 6:
      switch (op) {
        case tokens::kShl:
          return Value(uint32_t(std::get<uint32_t>(x.value_) << s));
        case tokens::kShr:
          return Value(uint32_t(std::get<uint32_t>(x.value_) >> s));
        default:
          throw "unexpected shift op";
      }
    case 7:
      switch (op) {
        case tokens::kShl:
          return Value(int64_t(std::get<int64_t>(x.value_) << s));
        case tokens::kShr:
          return Value(int64_t(std::get<int64_t>(x.value_) >> s));
        default:
          throw "unexpected shift op";
      }
    case 8:
      switch (op) {
        case tokens::kShl:
          return Value(uint64_t(std::get<uint64_t>(x.value_) << s));
        case tokens::kShr:
          return Value(uint64_t(std::get<uint64_t>(x.value_) >> s));
        default:
          throw "unexpected shift op";
      }
    default:
      throw "unexpected value_t of shifted operand";
  }
}

Value UnaryOp(tokens::Token op, Value x) {
  switch (x.value_.index()) {
    case 0:
      switch (op) {
        case tokens::kNot:
          return Value(!std::get<bool>(x.value_));
        default:
          throw "unexpected unary op";
      }
    case 1:
      switch (op) {
        case tokens::kAdd:
          return x;
        case tokens::kSub:
          return Value(int8_t(-std::get<int8_t>(x.value_)));
        case tokens::kXor:
          return Value(int8_t(~std::get<int8_t>(x.value_)));
        default:
          throw "unexpected unary op";
      }
    case 2:
      switch (op) {
        case tokens::kAdd:
          return x;
        case tokens::kSub:
          return Value(int8_t(-std::get<uint8_t>(x.value_)));
        case tokens::kXor:
          return Value(uint8_t(~std::get<uint8_t>(x.value_)));
        default:
          throw "unexpected unary op";
      }
    case 3:
      switch (op) {
        case tokens::kAdd:
          return x;
        case tokens::kSub:
          return Value(int16_t(-std::get<int16_t>(x.value_)));
        case tokens::kXor:
          return Value(int16_t(~std::get<int16_t>(x.value_)));
        default:
          throw "unexpected unary op";
      }
    case 4:
      switch (op) {
        case tokens::kAdd:
          return x;
        case tokens::kSub:
          return Value(int16_t(-std::get<uint16_t>(x.value_)));
        case tokens::kXor:
          return Value(uint16_t(~std::get<uint16_t>(x.value_)));
        default:
          throw "unexpected unary op";
      }
    case 5:
      switch (op) {
        case tokens::kAdd:
          return x;
        case tokens::kSub:
          return Value(int32_t(-std::get<int32_t>(x.value_)));
        case tokens::kXor:
          return Value(int32_t(~std::get<int32_t>(x.value_)));
        default:
          throw "unexpected unary op";
      }
    case 6:
      switch (op) {
        case tokens::kAdd:
          return x;
        case tokens::kSub:
          return Value(int32_t(-std::get<uint32_t>(x.value_)));
        case tokens::kXor:
          return Value(uint32_t(~std::get<uint32_t>(x.value_)));
        default:
          throw "unexpected unary op";
      }
    case 7:
      switch (op) {
        case tokens::kAdd:
          return x;
        case tokens::kSub:
          return Value(int64_t(-std::get<int64_t>(x.value_)));
        case tokens::kXor:
          return Value(int64_t(~std::get<int64_t>(x.value_)));
        default:
          throw "unexpected unary op";
      }
    case 8:
      switch (op) {
        case tokens::kAdd:
          return x;
        case tokens::kSub:
          return Value(int64_t(-std::get<uint64_t>(x.value_)));
        case tokens::kXor:
          return Value(uint64_t(~std::get<uint64_t>(x.value_)));
        default:
          throw "unexpected unary op";
      }
    default:
      throw "unexpected value_t";
  }
}

template <>
Value Convert<bool>(Value x) {
  switch (x.value_.index()) {
    case 0:
      return x;
    default:
      throw "unexpected value_t";
  }
}

template <>
Value Convert<std::string>(Value x) {
  switch (x.value_.index()) {
    case 9:
      return x;
    default:
      throw "unexpected value_t";
  }
}

}  // namespace constants
}  // namespace lang
