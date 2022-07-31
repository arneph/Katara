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
#include <ostream>
#include <string>
#include <vector>

#include "src/common/atomics/atomics.h"
#include "src/ir/representation/object.h"

namespace ir {

enum class TypeKind {
  kBool,
  kInt,
  kPointer,
  kFunc,

  kLangSharedPointer,
  kLangUniquePointer,
  kLangString,
  kLangArray,
  kLangStruct,
  kLangInterface,
  kLangTypeID,
};

enum Alignment : int8_t {
  kNoAlignment = 0,
  k1Byte = 1 << 0,
  k2Byte = 1 << 1,
  k4Byte = 1 << 2,
  k8Byte = 1 << 3,
  k16Byte = 1 << 4,
};

bool IsAtomicType(TypeKind type_kind);

class Type : public Object {
 public:
  constexpr virtual ~Type() {}

  constexpr Object::Kind object_kind() const final { return Object::Kind::kType; }
  constexpr virtual TypeKind type_kind() const = 0;
  constexpr virtual int64_t size() const = 0;
  constexpr virtual Alignment alignment() const = 0;

  constexpr virtual bool operator==(const Type& that) const = 0;
};

constexpr bool IsEqual(const Type* type_a, const Type* type_b) {
  if (type_a == type_b) return true;
  if (type_a == nullptr || type_b == nullptr) return false;
  return *type_a == *type_b;
}

class AtomicType : public Type {
 public:
  constexpr virtual ~AtomicType() {}

  constexpr virtual int8_t bit_size() const = 0;
  constexpr bool operator==(const Type& that) const override {
    return type_kind() == that.type_kind();
  }
};

class BoolType : public AtomicType {
 public:
  constexpr int8_t bit_size() const override { return 8; }
  constexpr TypeKind type_kind() const override { return TypeKind::kBool; }
  constexpr int64_t size() const override { return 1; }
  constexpr Alignment alignment() const override { return Alignment::k1Byte; }
  void WriteRefString(std::ostream& os) const override { os << "b"; }
};

const BoolType* bool_type();

class IntType : public AtomicType {
 public:
  constexpr IntType(common::IntType type) : type_(type) {}

  constexpr int8_t bit_size() const override { return common::BitSizeOf(type_); }
  constexpr common::IntType int_type() const { return type_; }
  constexpr TypeKind type_kind() const override { return TypeKind::kInt; }
  constexpr int64_t size() const override { return common::BitSizeOf(type_) / 8; }
  constexpr Alignment alignment() const override { return Alignment(size()); }
  bool operator==(const Type& that) const override;
  void WriteRefString(std::ostream& os) const override { os << common::ToString(type_); }

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
  constexpr int64_t size() const override { return 8; }
  constexpr Alignment alignment() const override { return Alignment::k8Byte; }
  void WriteRefString(std::ostream& os) const override { os << "ptr"; }
};

const PointerType* pointer_type();

class FuncType : public AtomicType {
 public:
  constexpr int8_t bit_size() const override { return 64; }
  constexpr TypeKind type_kind() const override { return TypeKind::kFunc; }
  constexpr int64_t size() const override { return 8; }
  constexpr Alignment alignment() const override { return Alignment::k8Byte; }
  void WriteRefString(std::ostream& os) const override { os << "func"; }
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
