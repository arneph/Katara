//
//  types.h
//  Katara
//
//  Created by Arne Philipeit on 5/14/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_ext_types_h
#define lang_ir_ext_types_h

#include <string>

#include "ir/representation/types.h"

namespace lang {
namespace ir_ext {

class RefCountPointer : public ir::Type {
 public:
  RefCountPointer() {}

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangRefCountPointer; }
  std::string ToString() const override { return "rcptr"; }
};

class String : public ir::Type {
 public:
  String() {}

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangString; }
  std::string ToString() const override { return "str"; }
};

constexpr int64_t kDynamicArraySize = -1;

class Array : public ir::Type {
 public:
  Array(ir::Type* element, int64_t size = kDynamicArraySize) : element_(element), size_(size) {}

  ir::Type* element() const { return element_; }
  bool is_dynaimc() const { return size_ == kDynamicArraySize; }
  int64_t size() const { return size_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangArray; }
  std::string ToString() const override { return "array"; }

 private:
  ir::Type* element_;
  int64_t size_;
};

class Struct : public ir::Type {
 public:
  Struct(std::vector<std::pair<std::string, ir::Type*>> fields) : fields_(fields) {}

  const std::vector<std::pair<std::string, ir::Type*>>& fields() const { return fields_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangStruct; }
  std::string ToString() const override { return "struct"; }

 private:
  std::vector<std::pair<std::string, ir::Type*>> fields_;
};

class Interface : public ir::Type {
 public:
  Interface(std::vector<std::string> methods) : methods_(methods) {}

  const std::vector<std::string>& methods() const { return methods_; }

  ir::TypeKind type_kind() const override { return ir::TypeKind::kLangInterface; }
  std::string ToString() const override { return "interface"; }

 private:
  std::vector<std::string> methods_;
};

}  // namespace ir_ext
}  // namespace lang

#endif /* lang_ir_ext_types_h */
