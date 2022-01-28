//
//  types.h
//  Katara
//
//  Created by Arne Philipeit on 5/1/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_types_h
#define ir_types_h

#include <memory>
#include <string>
#include <vector>

#include "src/common/atomics/atomics.h"

namespace ir {

enum class TypeKind {
  kBool,
  kInt,
  kPointer,
  kFunc,

  kLangSharedPointer,
  kLangString,
  kLangArray,
  kLangStruct,
  kLangInterface,
  kLangTypeID,
};

bool IsAtomicType(TypeKind type_kind);

class Type {
 public:
  constexpr virtual ~Type() {}

  constexpr virtual TypeKind type_kind() const = 0;
  virtual std::string ToString() const = 0;
};

class AtomicType : public Type {
 public:
  constexpr virtual ~AtomicType() {}

  constexpr virtual int8_t bit_size() const = 0;
};

class BoolType : public AtomicType {
 public:
  constexpr int8_t bit_size() const override { return 8; }
  constexpr TypeKind type_kind() const override { return TypeKind::kBool; }
  std::string ToString() const override { return "b"; }
};

const BoolType* bool_type();

class IntType : public AtomicType {
 public:
  constexpr IntType(common::IntType type) : type_(type) {}

  constexpr int8_t bit_size() const override { return common::BitSizeOf(type_); }
  constexpr common::IntType int_type() const { return type_; }
  constexpr TypeKind type_kind() const override { return TypeKind::kInt; }
  std::string ToString() const override { return common::ToString(type_); }

 private:
  const common::IntType type_;
};

const IntType* i8();
const IntType* i16();
const IntType* i32();
const IntType* i64();
const IntType* u8();
const IntType* u16();
const IntType* u32();
const IntType* u64();
const IntType* IntTypeFor(common::IntType type);

class PointerType : public AtomicType {
 public:
  constexpr int8_t bit_size() const override { return 64; }
  constexpr TypeKind type_kind() const override { return TypeKind::kPointer; }
  std::string ToString() const override { return "ptr"; }
};

const PointerType* pointer_type();

class FuncType : public AtomicType {
 public:
  constexpr int8_t bit_size() const override { return 64; }
  constexpr TypeKind type_kind() const override { return TypeKind::kFunc; }
  std::string ToString() const override { return "func"; }
};

const FuncType* func_type();

class TypeTable {
 public:
  Type* AddType(std::unique_ptr<Type> type);

 private:
  std::vector<std::unique_ptr<Type>> types_;
};

}  // namespace ir

#endif /* ir_types_h */
