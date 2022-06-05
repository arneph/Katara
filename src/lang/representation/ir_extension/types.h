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

class SmartPointer : public ir::Type {
 public:
  SmartPointer(const ir::Type* element) : element_(element) {}

  const ir::Type* element() const { return element_; }

 private:
  const ir::Type* element_;
};

class SharedPointer : public SmartPointer {
 public:
  SharedPointer(bool is_strong, const ir::Type* element)
      : SmartPointer(element), is_strong_(is_strong) {}

  bool is_strong() const { return is_strong_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangSharedPointer; }
  void WriteRefString(std::ostream& os) const override;

  bool operator==(const ir::Type& that) const override;

 private:
  bool is_strong_;
};

class UniquePointer : public SmartPointer {
 public:
  explicit UniquePointer(const ir::Type* element) : SmartPointer(element) {}

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangUniquePointer; }
  void WriteRefString(std::ostream& os) const override;

  bool operator==(const ir::Type& that) const override;
};

class StringType : public ir::Type {
 public:
  constexpr ir::TypeKind type_kind() const override { return ir::TypeKind::kLangString; }
  void WriteRefString(std::ostream& os) const override { os << "lstr"; }

  bool operator==(const ir::Type& that) const override { return type_kind() == that.type_kind(); }
};

const StringType* string();

constexpr int64_t kDynamicArraySize = -1;

class Array : public ir::Type {
 public:
  const ir::Type* element() const { return element_; }
  bool is_dynamic() const { return size_ == kDynamicArraySize; }
  int64_t size() const { return size_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangArray; }
  void WriteRefString(std::ostream& os) const override;

  bool operator==(const ir::Type& that) const override;

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
  struct Field {
    std::string name;
    const ir::Type* type;
  };

  static Struct EmptyStruct() { return Struct(); }

  const std::vector<Field>& fields() const { return fields_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangStruct; }
  void WriteRefString(std::ostream& os) const override;

  bool operator==(const ir::Type& that) const override;

 private:
  Struct() {}

  std::vector<Field> fields_;

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

const Struct* empty_struct();

class Interface : public ir::Type {
 public:
  struct Method {
    std::string name;
    std::vector<const ir::Type*> parameters;
    std::vector<const ir::Type*> results;
  };

  static Interface EmptyInterface() { return Interface(); }

  const std::vector<Method>& methods() const { return methods_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangInterface; }
  void WriteRefString(std::ostream& os) const override;

  bool operator==(const ir::Type& that) const override;

 private:
  Interface() {}

  std::vector<Method> methods_;

  friend class InterfaceBuilder;
};

class InterfaceBuilder {
 public:
  InterfaceBuilder();

  void AddMethod(std::string name, std::vector<const ir::Type*> parameters,
                 std::vector<const ir::Type*> results);

  Interface* Get() { return interface_.get(); }
  std::unique_ptr<Interface> Build() { return std::move(interface_); }

 private:
  std::unique_ptr<Interface> interface_;
};

const Interface* empty_interface();

class TypeID : public ir::Type {
 public:
  TypeID() {}

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangTypeID; }
  void WriteRefString(std::ostream& os) const override { os << "ltypeid"; }

  bool operator==(const ir::Type& that) const override { return type_kind() == that.type_kind(); }
};

const TypeID* type_id();

}  // namespace ir_ext
}  // namespace lang

#endif /* lang_ir_ext_types_h */
