//
//  types.cc
//  Katara
//
//  Created by Arne Philipeit on 5/14/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "types.h"

namespace lang {
namespace ir_ext {

const StringType kString;

bool SharedPointer::operator==(const ir::Type& that_type) const {
  if (that_type.type_kind() != ir::TypeKind::kLangSharedPointer) return false;
  auto that = static_cast<const SharedPointer&>(that_type);
  if (is_strong() != that.is_strong()) return false;
  return ir::IsEqual(element(), that.element());
}

bool UniquePointer::operator==(const ir::Type& that_type) const {
  if (that_type.type_kind() != ir::TypeKind::kLangUniquePointer) return false;
  auto that = static_cast<const UniquePointer&>(that_type);
  return ir::IsEqual(element(), that.element());
}

bool Array::operator==(const ir::Type& that_type) const {
  if (that_type.type_kind() != ir::TypeKind::kLangArray) return false;
  auto that = static_cast<const Array&>(that_type);
  if (size() != that.size()) return false;
  return ir::IsEqual(element(), that.element());
}

bool Struct::operator==(const ir::Type& that_type) const {
  if (that_type.type_kind() != ir::TypeKind::kLangStruct) return false;
  auto that = static_cast<const Struct&>(that_type);
  if (fields().size() != that.fields().size()) return false;
  for (std::size_t i = 0; i < fields().size(); i++) {
    const Field& field_a = fields().at(i);
    const Field& field_b = that.fields().at(i);
    if (field_a.name != field_b.name) return false;
    if (!ir::IsEqual(field_a.type, field_b.type)) return false;
  }
  return true;
}

bool Interface::operator==(const ir::Type& that_type) const {
  if (that_type.type_kind() != ir::TypeKind::kLangInterface) return false;
  auto that = static_cast<const Interface&>(that_type);
  if (methods().size() != that.methods().size()) return false;
  for (std::size_t i = 0; i < methods().size(); i++) {
    const Method& method_a = methods().at(i);
    const Method& method_b = that.methods().at(i);
    if (method_a.name != method_b.name) return false;
    if (method_a.parameters.size() != method_b.parameters.size()) return false;
    if (method_a.results.size() != method_b.results.size()) return false;
    for (std::size_t j = 0; j < method_a.parameters.size(); j++) {
      if (!ir::IsEqual(method_a.parameters.at(j), method_b.parameters.at(j))) return false;
    }
    for (std::size_t j = 0; j < method_a.results.size(); j++) {
      if (!ir::IsEqual(method_a.results.at(j), method_b.results.at(j))) return false;
    }
  }
  return true;
}

ArrayBuilder::ArrayBuilder() { array_ = std::unique_ptr<Array>(new Array()); }

StructBuilder::StructBuilder() { struct_ = std::unique_ptr<Struct>(new Struct()); }

void StructBuilder::AddField(std::string name, const ir::Type* field_type) {
  struct_->fields_.push_back(Struct::Field{.name = name, .type = field_type});
}

}  // namespace ir_ext
}  // namespace lang
