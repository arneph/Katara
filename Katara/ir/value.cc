//
//  value.cc
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "value.h"

#include <any>

#include "ir/block.h"

namespace ir {

bool is_integral(Type type) {
    switch (type) {
        case Type::kBool:
        case Type::kI8:
        case Type::kI16:
        case Type::kI32:
        case Type::kI64:
        case Type::kU8:
        case Type::kU16:
        case Type::kU32:
        case Type::kU64:
            return true;
        default:
            return false;
    }
}

bool is_unsigned(Type type) {
    switch (type) {
        case Type::kBool:
        case Type::kU8:
        case Type::kU16:
        case Type::kU32:
        case Type::kU64:
            return true;
        case Type::kI8:
        case Type::kI16:
        case Type::kI32:
        case Type::kI64:
            return false;
        default:
            throw "type is non-integral";
    }
}

extern int8_t size(Type type) {
    switch (type) {
        case Type::kBool:
        case Type::kI8:
        case Type::kU8:
            return 8;
        case Type::kI16:
        case Type::kU16:
            return 16;
        case Type::kI32:
        case Type::kU32:
            return 32;
        case Type::kI64:
        case Type::kU64:
        case Type::kFunc:
            return 64;
        default:
            throw "type has no associated size";
    }
}

Type to_type(std::string type_str) {
    if (type_str == "b")
        return Type::kBool;
    if (type_str == "i8")
        return Type::kI8;
    if (type_str == "i16")
        return Type::kI16;
    if (type_str == "i32")
        return Type::kI32;
    if (type_str == "i64")
        return Type::kI64;
    if (type_str == "u8")
        return Type::kU8;
    if (type_str == "u16")
        return Type::kU16;
    if (type_str == "u32")
        return Type::kU32;
    if (type_str == "u64")
        return Type::kU64;
    if (type_str == "block")
        return Type::kBlock;
    if (type_str == "func")
        return Type::kFunc;
    throw "unknown type string";
}

std::string to_string(Type type) {
    switch (type) {
        case Type::kUnknown:
            throw "can not convert unknown type to string";
        case Type::kBool:  return "b";
        case Type::kI8:    return "i8";
        case Type::kI16:   return "i16";
        case Type::kI32:   return "i32";
        case Type::kI64:   return "i64";
        case Type::kU8:    return "u8";
        case Type::kU16:   return "u16";
        case Type::kU32:   return "u32";
        case Type::kU64:   return "u64";
        case Type::kBlock: return "block";
        case Type::kFunc:  return "func";
    }
}

Constant::Constant(Type type, Data value)
    : type_(type), value_(value) {
    if (!is_integral(type) && type != Type::kFunc)
        throw "attempted to create const of non-integral type "
              "or function value";
}

Type Constant::type() const {
    return type_;
}

Constant::Data Constant::value() const {
    return value_;
}

bool operator==(const Constant& lhs,
                const Constant& rhs) {
    return lhs.type() == rhs.type()
        && lhs.value().i64 == rhs.value().i64;
}

bool operator!=(const Constant& lhs,
                const Constant& rhs) {
    return !(lhs == rhs);
}

std::string Constant::ToString() const {
    switch (type_) {
        case Type::kBool:
            return (value_.b) ? "#t" : "#f";
        case Type::kI8:
            return "#" + std::to_string(value_.i8);
        case Type::kI16:
            return "#" + std::to_string(value_.i16);
        case Type::kI32:
            return "#" + std::to_string(value_.i32);
        case Type::kI64:
            return "#" + std::to_string(value_.i64);
        case Type::kU8:
            return "#" + std::to_string(value_.u8);
        case Type::kU16:
            return "#" + std::to_string(value_.u16);
        case Type::kU32:
            return "#" + std::to_string(value_.u32);
        case Type::kU64:
            return "#" + std::to_string(value_.u64);
        case Type::kFunc:
            return "@" + std::to_string(value_.func);
        default:
            throw "unexpected const type";
    }
}

std::string Constant::ToStringWithType() const {
    if (type_ == Type::kBool ||
        type_ == Type::kFunc) {
        return ToString();
    }
    
    return ToString() + ":" + to_string(type_);
}

Computed::Computed(Type type, int64_t number)
    : type_(type), number_(number) {}

Type Computed::type() const {
    return type_;
}

int64_t Computed::number() const {
    return number_;
}

std::string Computed::ToString() const {
    return "%" + std::to_string(number_);
}

std::string Computed::ToStringWithType() const {
    return ToString() + ":" + to_string(type_);
}

bool operator==(const Computed& lhs,
                const Computed& rhs) {
    return lhs.type() == rhs.type()
        && lhs.number() == rhs.number();
}

bool operator!=(const Computed &lhs,
                const Computed &rhs) {
    return !(lhs == rhs);
}

bool operator<(const Computed& lhs,
               const Computed& rhs) {
    return lhs.number() < rhs.number();
}

bool operator>(const Computed& lhs,
               const Computed& rhs) {
    return lhs.number() > rhs.number();
}

bool operator<=(const Computed& lhs,
               const Computed& rhs) {
    return lhs.number() <= rhs.number();
}

bool operator>=(const Computed& lhs,
               const Computed& rhs) {
    return lhs.number() >= rhs.number();
}

std::vector<ir::Computed> set_to_ordered_vec(
    const std::unordered_set<ir::Computed>& set) {
    std::vector<ir::Computed> vec(set.begin(), set.end());
    std::sort(vec.begin(), vec.end());
    
    return vec;
}

void set_to_stream(const std::unordered_set<ir::Computed>& set,
                   std::stringstream& ss) {
    bool first = true;
    for (ir::Computed value : set_to_ordered_vec(set)) {
        if (first) {
            first = false;
        } else {
            ss << ", ";
        }
        ss << value.ToString();
    }
}

BlockValue::BlockValue(int64_t block)
    : block_(block) {}

int64_t BlockValue::block() const {
    return block_;
}

std::string BlockValue::ToString() const {
    return "{" + std::to_string(block_) + "}";
}

bool operator==(const BlockValue& lhs,
                const BlockValue& rhs) {
    return lhs.block() == rhs.block();
}

bool operator!=(const BlockValue& lhs,
                const BlockValue& rhs) {
    return !(lhs == rhs);
}

Value::Value(Constant c)   : kind_(Kind::kConstant),   data_(c) {}
Value::Value(Computed c)   : kind_(Kind::kComputed),   data_(c) {}
Value::Value(BlockValue b) : kind_(Kind::kBlockValue), data_(b) {}

Value::Kind Value::kind() const {
    return kind_;
}

Type Value::type() const {
    switch (kind_) {
        case Kind::kConstant:
            return data_.constant.type();
        case Kind::kComputed:
            return data_.computed.type();
        case Kind::kBlockValue:
            return Type::kBlock;
    }
}

bool Value::is_constant() const {
    return kind_ == Kind::kConstant;
}

Constant Value::constant() const {
    if (kind_ != Kind::kConstant)
        throw "attempted to obtain constant from non-constant value";
    return data_.constant;
}

bool Value::is_computed() const {
    return kind_ == Kind::kComputed;
}

Computed Value::computed() const {
    if (kind_ != Kind::kComputed)
        throw "attempted to obtain computed from non-computed value";
    return data_.computed;
}

bool Value::is_block_value() const {
    return kind_ == Kind::kBlockValue;
}

BlockValue Value::block_value() const {
    if (kind_ != Kind::kBlockValue)
        throw "attempted to obtain block value from "
              "non-block-value value";
    return data_.block_value;
}

std::string Value::ToString() const {
    switch (kind_) {
        case Kind::kConstant:
            return data_.constant.ToString();
        case Kind::kComputed:
            return data_.computed.ToString();
        case Kind::kBlockValue:
            return data_.block_value.ToString();
    }
}

std::string Value::ToStringWithType() const {
    switch (kind_) {
        case Kind::kConstant:
            return data_.constant.ToStringWithType();
        case Kind::kComputed:
            return data_.computed.ToStringWithType();
        case Kind::kBlockValue:
            return data_.block_value.ToString();
    }
}

bool operator==(const Value& lhs,
                const Value& rhs) {
    if (lhs.kind() != rhs.kind()) {
        return false;
    }
    switch (lhs.kind()) {
        case ir::Value::Kind::kConstant:
            return lhs.constant() == rhs.constant();
        case ir::Value::Kind::kComputed:
            return lhs.computed() == rhs.computed();
        case ir::Value::Kind::kBlockValue:
            return lhs.block_value() == rhs.block_value();
    }
}

bool operator!=(const Value& lhs,
                const Value& rhs) {
    return !(lhs == rhs);
}

InheritedValue::InheritedValue(Value value, BlockValue origin)
    : value_(value), origin_(origin) {
    if (value.is_block_value())
        throw "can not inherit block value";
}

Value::Kind InheritedValue::kind() const {
    return value_.kind();
}

Type InheritedValue::type() const {
    return value_.type();
}

Value InheritedValue::value() const {
    return value_;
}

BlockValue InheritedValue::origin() const {
    return origin_;
}

std::string InheritedValue::ToString() const {
    return value_.ToString() + ":" + origin_.ToString();
}

std::string InheritedValue::ToStringWithType() const {
    return value_.ToStringWithType() + origin_.ToString();
}

}
