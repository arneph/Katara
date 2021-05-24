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

namespace ir {

enum class TypeKind {
  kAtomic,

  kLangPointer,
  kLangString,
  kLangArray,
  kLangStruct,
  kLangInterface,
  kLangTypeID,
};

class Type {
 public:
  virtual ~Type() {}

  virtual TypeKind type_kind() const = 0;
  virtual std::string ToString() const = 0;
};

enum class AtomicKind : int8_t {
  kBool,
  kI8,
  kI16,
  kI32,
  kI64,
  kU8,
  kU16,
  kU32,
  kU64,
  kPtr,
  kFunc,
};

extern bool IsIntegral(AtomicKind type);
extern bool IsUnsigned(AtomicKind type);
extern int8_t SizeOf(AtomicKind type);
extern AtomicKind ToAtomicTypeKind(std::string type_str);
extern std::string ToString(AtomicKind type);

class Atomic : public Type {
 public:
  Atomic(AtomicKind kind) : kind_(kind) {}

  AtomicKind kind() const { return kind_; }

  TypeKind type_kind() const override { return TypeKind::kAtomic; }
  std::string ToString() const override { return ir::ToString(kind_); }

 private:
  AtomicKind kind_;
};

class TypeTable {
 public:
  TypeTable();

  Atomic* AtomicOfKind(AtomicKind kind) const {
    return atomic_types_.at(static_cast<int8_t>(kind)).get();
  }

  Type* AddType(std::unique_ptr<Type> type);

 private:
  std::vector<std::unique_ptr<Atomic>> atomic_types_;
  std::vector<std::unique_ptr<Type>> types_;
};

}  // namespace ir

#endif /* ir_types_h */
