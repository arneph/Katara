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
#include <string>

#include "src/ir/representation/types.h"

namespace lang {
namespace ir_ext {

class Pointer : public ir::Type {
 public:
  Pointer(bool is_strong, ir::Type* element) : is_strong_(is_strong), element_(element) {}

  bool is_strong() const { return is_strong_; }
  ir::Type* element() const { return element_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangPointer; }
  std::string ToString() const override { return "lptr"; }

 private:
  bool is_strong_;
  ir::Type* element_;
};

class String : public ir::Type {
 public:
  String() {}

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangString; }
  std::string ToString() const override { return "lstr"; }
};

constexpr int64_t kDynamicArraySize = -1;

class Array : public ir::Type {
 public:
  ir::Type* element() const { return element_; }
  bool is_dynamic() const { return size_ == kDynamicArraySize; }
  int64_t size() const { return size_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangArray; }
  std::string ToString() const override { return "larray"; }

 private:
  Array() : element_(nullptr), size_(kDynamicArraySize) {}

  ir::Type* element_;
  int64_t size_;

  friend class ArrayBuilder;
};

class ArrayBuilder {
 public:
  ArrayBuilder();

  void SetElement(ir::Type* element) { array_->element_ = element; }
  void SetFixedSize(int64_t size) { array_->size_ = size; }

  Array* Get() { return array_.get(); }
  std::unique_ptr<Array> Build() { return std::move(array_); }

 private:
  std::unique_ptr<Array> array_;
};

class Struct : public ir::Type {
 public:
  const std::vector<std::pair<std::string, ir::Type*>>& fields() const { return fields_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangStruct; }
  std::string ToString() const override { return "lstruct"; }

 private:
  Struct() {}

  std::vector<std::pair<std::string, ir::Type*>> fields_;

  friend class StructBuilder;
};

class StructBuilder {
 public:
  StructBuilder();

  void AddField(std::string name, ir::Type* field_type);

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
  std::string ToString() const override { return "linterface"; }

 private:
  std::vector<std::string> methods_;
};

class TypeID : public ir::Type {
 public:
  TypeID() {}

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangTypeID; }
  std::string ToString() const override { return "ltypeid"; }
};

}  // namespace ir_ext
}  // namespace lang

#endif /* lang_ir_ext_types_h */
