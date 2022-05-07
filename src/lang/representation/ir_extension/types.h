//
//  types.h
//  Katara
//
//  Created by Arne Philipeit on 5/14/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_ext_types_h
#define lang_ir_ext_types_h

#include <memory>
#include <ostream>
#include <string>

#include "src/ir/representation/types.h"

namespace lang {
namespace ir_ext {

class SharedPointer : public ir::Type {
 public:
  SharedPointer(bool is_strong, const ir::Type* element)
      : is_strong_(is_strong), element_(element) {}

  bool is_strong() const { return is_strong_; }
  const ir::Type* element() const { return element_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangSharedPointer; }
  void WriteRefString(std::ostream& os) const override { os << "lptr"; }

 private:
  bool is_strong_;
  const ir::Type* element_;
};

class StringType : public ir::Type {
 public:
  constexpr ir::TypeKind type_kind() const override { return ir::TypeKind::kLangString; }
  void WriteRefString(std::ostream& os) const override { os << "lstr"; }
};

extern const StringType kString;

constexpr int64_t kDynamicArraySize = -1;

class Array : public ir::Type {
 public:
  const ir::Type* element() const { return element_; }
  bool is_dynamic() const { return size_ == kDynamicArraySize; }
  int64_t size() const { return size_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangArray; }
  void WriteRefString(std::ostream& os) const override { os << "larray"; }

 private:
  Array() : element_(nullptr), size_(kDynamicArraySize) {}

  const ir::Type* element_;
  int64_t size_;

  friend class ArrayBuilder;
};

class ArrayBuilder {
 public:
  ArrayBuilder();

  void SetElement(const ir::Type* element) { array_->element_ = element; }
  void SetFixedSize(int64_t size) { array_->size_ = size; }

  Array* Get() { return array_.get(); }
  std::unique_ptr<Array> Build() { return std::move(array_); }

 private:
  std::unique_ptr<Array> array_;
};

class Struct : public ir::Type {
 public:
  const std::vector<std::pair<std::string, const ir::Type*>>& fields() const { return fields_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangStruct; }
  void WriteRefString(std::ostream& os) const override { os << "lstruct"; }

 private:
  Struct() {}

  std::vector<std::pair<std::string, const ir::Type*>> fields_;

  friend class StructBuilder;
};

class StructBuilder {
 public:
  StructBuilder();

  void AddField(std::string name, const ir::Type* field_type);

  Struct* Get() { return struct_.get(); }
  std::unique_ptr<Struct> Build() { return std::move(struct_); }

 private:
  std::unique_ptr<Struct> struct_;
};

class Interface : public ir::Type {
 public:
  Interface(std::vector<std::string> methods) : methods_(methods) {}

  const std::vector<std::string>& methods() const { return methods_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangInterface; }
  void WriteRefString(std::ostream& os) const override { os << "linterface"; }

 private:
  std::vector<std::string> methods_;
};

class TypeID : public ir::Type {
 public:
  TypeID() {}

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangTypeID; }
  void WriteRefString(std::ostream& os) const override { os << "ltypeid"; }
};

}  // namespace ir_ext
}  // namespace lang

#endif /* lang_ir_ext_types_h */
