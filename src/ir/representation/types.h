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

#include "src/common/atomics.h"

namespace ir {

enum class TypeKind {
  kBool,
  kInt,
  kPointer,
  kFunc,

  kLangPointer,
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

constexpr BoolType kBool;

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

static constexpr IntType kI8{common::IntType::kI8};
static constexpr IntType kI16{common::IntType::kI16};
static constexpr IntType kI32{common::IntType::kI32};
static constexpr IntType kI64{common::IntType::kI64};
static constexpr IntType kU8{common::IntType::kU8};
static constexpr IntType kU16{common::IntType::kU16};
static constexpr IntType kU32{common::IntType::kU32};
static constexpr IntType kU64{common::IntType::kU64};

constexpr const IntType* IntTypeFor(common::IntType type) {
  switch (type) {
    case common::IntType::kI8:
      return &kI8;
    case common::IntType::kI16:
      return &kI16;
    case common::IntType::kI32:
      return &kI32;
    case common::IntType::kI64:
      return &kI64;
    case common::IntType::kU8:
      return &kU8;
    case common::IntType::kU16:
      return &kU16;
    case common::IntType::kU32:
      return &kU32;
    case common::IntType::kU64:
      return &kU64;
  }
}

class PointerType : public AtomicType {
 public:
  constexpr int8_t bit_size() const override { return 64; }
  constexpr TypeKind type_kind() const override { return TypeKind::kPointer; }
  std::string ToString() const override { return "ptr"; }
};

constexpr PointerType kPointer;

class FuncType : public AtomicType {
 public:
  constexpr int8_t bit_size() const override { return 64; }
  constexpr TypeKind type_kind() const override { return TypeKind::kFunc; }
  std::string ToString() const override { return "func"; }
};

constexpr FuncType kFunc;

class TypeTable {
 public:
  Type* AddType(std::unique_ptr<Type> type);

 private:
  std::vector<std::unique_ptr<Type>> types_;
};

}  // namespace ir

#endif /* ir_types_h */
