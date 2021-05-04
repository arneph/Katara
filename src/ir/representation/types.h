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

  kLangBasic,

  kLangPointer,
  kLangArray,
  kLangSlice,
  kLangWrapperStart = kLangPointer,
  kLangWrapperEnd = kLangSlice,
  kLangContainerStart = kLangArray,
  kLangContainerEnd = kLangSlice,

  kLangTypeParameter,
  kLangNamedType,
  kLangTypeInstance,
  kLangTuple,
  kLangSignature,
  kLangStruct,
  kLangInterface,

  kLangStart = kLangBasic,
  kLangEnd = kLangInterface,
};

class Type {
 public:
  virtual ~Type() {}

  bool is_atomic() const { return type_kind() == TypeKind::kAtomic; }
  bool is_lang_type() const;

  virtual TypeKind type_kind() const = 0;
  virtual std::string ToString() const = 0;
};

enum class AtomicTypeKind : int8_t {
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

extern bool IsIntegral(AtomicTypeKind type);
extern bool IsUnsigned(AtomicTypeKind type);
extern int8_t SizeOf(AtomicTypeKind type);
extern AtomicTypeKind ToAtomicTypeKind(std::string type_str);
extern std::string ToString(AtomicTypeKind type);

class AtomicType : public Type {
 public:
  AtomicType(AtomicTypeKind kind) : kind_(kind) {}

  AtomicTypeKind kind() const { return kind_; }

  TypeKind type_kind() const override { return TypeKind::kAtomic; }
  std::string ToString() const override { return ir::ToString(kind_); }

 private:
  AtomicTypeKind kind_;
};

class AtomicTypeTable {
 public:
  AtomicTypeTable();

  AtomicType* AtomicTypeForKind(AtomicTypeKind kind) const {
    return types_.at(static_cast<int8_t>(kind)).get();
  }

 private:
  std::vector<std::unique_ptr<AtomicType>> types_;
};

}  // namespace ir

#endif /* ir_types_h */
