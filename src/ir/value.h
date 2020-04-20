//
//  value.h
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_value_h
#define ir_value_h

#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>

namespace ir {

enum class Type : int8_t {
    kUnknown,
    kBool,
    kI8,
    kI16,
    kI32,
    kI64,
    kU8,
    kU16,
    kU32,
    kU64,
    kBlock,
    kFunc,
};

extern bool is_integral(Type type);
extern bool is_unsigned(Type type);
extern int8_t size(Type type);
extern Type to_type(std::string type_str);
extern std::string to_string(Type type);

class Constant {
public:
    Constant(bool value);
    Constant(int8_t value);
    Constant(int16_t value);
    Constant(int32_t value);
    Constant(int64_t value);
    Constant(uint8_t value);
    Constant(uint16_t value);
    Constant(uint32_t value);
    Constant(uint64_t value);
    Constant(Type type, int64_t value);
    
    Type type() const;
    int64_t value() const;
    
    std::string ToString() const;
    std::string ToStringWithType() const;
    
private:
    Type type_;
    int64_t value_;
};

bool operator==(const Constant& lhs,
                const Constant& rhs);
bool operator!=(const Constant& lhs,
                const Constant& rhs);

class Computed {
public:
    Computed(Type type, int64_t number_);
    
    Type type() const;
    int64_t number() const;
    
    std::string ToString() const;
    std::string ToStringWithType() const;
    
private:
    Type type_;
    int64_t number_;
};

bool operator==(const Computed& lhs,
                const Computed& rhs);
bool operator!=(const Computed& lhs,
                const Computed& rhs);
bool operator<(const Computed& lhs,
               const Computed& rhs);
bool operator>(const Computed& lhs,
               const Computed& rhs);
bool operator<=(const Computed& lhs,
                const Computed& rhs);
bool operator>=(const Computed& lhs,
                const Computed& rhs);

extern std::vector<ir::Computed>
set_to_ordered_vec(const std::unordered_set<ir::Computed>& set);
extern void
set_to_stream(const std::unordered_set<ir::Computed>& set,
              std::stringstream& ss);

class BlockValue {
public:
    BlockValue(int64_t block);
    
    int64_t block() const;
    
    std::string ToString() const;
    
private:
    int64_t block_;
};

bool operator==(const BlockValue& lhs,
                const BlockValue& rhs);
bool operator!=(const BlockValue& lhs,
                const BlockValue& rhs);

class Value {
public:
    enum class Kind : int8_t {
        kConstant,
        kComputed,
        kBlockValue,
    };
    
    Value(Constant c);
    Value(Computed c);
    Value(BlockValue b);
    
    Kind kind() const;
    Type type() const;
    
    bool is_constant() const;
    Constant constant() const;
    
    bool is_computed() const;
    Computed computed() const;
    
    bool is_block_value() const;
    BlockValue block_value() const;
    
    std::string ToString() const;
    std::string ToStringWithType() const;
    
private:
    union Data {
        Constant constant;
        Computed computed;
        BlockValue block_value;
        
        Data(Constant c) : constant(c) {}
        Data(Computed c) : computed(c) {}
        Data(BlockValue b) : block_value(b) {}
    };
    
    Kind kind_;
    Data data_;
};

bool operator==(const Value& lhs,
                const Value& rhs);
bool operator!=(const Value& lhs,
                const Value& rhs);

class InheritedValue {
public:
    InheritedValue(Value value, BlockValue origin);
    
    Value::Kind kind() const;
    Type type() const;
    Value value() const;
    BlockValue origin() const;
    
    std::string ToString() const;
    std::string ToStringWithType() const;
    
private:
    Value value_;
    BlockValue origin_;
};

}

namespace std {

template<>
struct hash<ir::Constant> {
    std::size_t operator()(const ir::Constant c) const {
        return c.value() ^ (int64_t(c.type()) << 48);
    }
};

template<>
struct hash<ir::Computed> {
    std::size_t operator()(const ir::Computed& c) const {
        return c.number() ^ (int64_t(c.type()) << 48);
    }
};

template<>
struct hash<ir::BlockValue> {
    std::size_t operator()(const ir::BlockValue& b) const {
        return b.block();
    }
};

template<>
struct hash<ir::Value> {
    std::size_t operator()(const ir::Value& v) const {
        int64_t h = int64_t(v.kind()) << 54;
        switch (v.kind()) {
            case ir::Value::Kind::kConstant:
                return h ^ hash<ir::Constant>{}(v.constant());
            case ir::Value::Kind::kComputed:
                return h ^ hash<ir::Computed>{}(v.computed());
            case ir::Value::Kind::kBlockValue:
                return h ^ hash<ir::BlockValue>{}(v.block_value());
        }
    }
};

}

#endif /* ir_value_h */
