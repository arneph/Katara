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
namespace {

const StringType kString;
const Struct kEmptyStruct = Struct::EmptyStruct();
const Interface kEmptyInterface = Interface::EmptyInterface();
const TypeID kTypeId;

}  // namespace

void SharedPointer::WriteRefString(std::ostream& os) const {
  os << "lshared_ptr<";
  element()->WriteRefString(os);
  os << ", " << (is_strong_ ? "s" : "w") << ">";
}

bool SharedPointer::operator==(const ir::Type& that_type) const {
  if (that_type.type_kind() != ir::TypeKind::kLangSharedPointer) return false;
  auto that = static_cast<const SharedPointer&>(that_type);
  if (is_strong() != that.is_strong()) return false;
  return ir::IsEqual(element(), that.element());
}

void UniquePointer::WriteRefString(std::ostream& os) const {
  os << "lunique_ptr<";
  element()->WriteRefString(os);
  os << ">";
}

bool UniquePointer::operator==(const ir::Type& that_type) const {
  if (that_type.type_kind() != ir::TypeKind::kLangUniquePointer) return false;
  auto that = static_cast<const UniquePointer&>(that_type);
  return ir::IsEqual(element(), that.element());
}

const StringType* string() { return &kString; }

void Array::WriteRefString(std::ostream& os) const {
  os << "larray<";
  element()->WriteRefString(os);
  os << ", " << count_ << ">";
}

bool Array::operator==(const ir::Type& that_type) const {
  if (that_type.type_kind() != ir::TypeKind::kLangArray) return false;
  auto that = static_cast<const Array&>(that_type);
  if (size() != that.size()) return false;
  return ir::IsEqual(element(), that.element());
}

ArrayBuilder::ArrayBuilder() { array_ = std::unique_ptr<Array>(new Array()); }

void Struct::WriteRefString(std::ostream& os) const {
  if (fields_.empty()) {
    os << "lstruct";
    return;
  }
  os << "lstruct<";
  for (std::size_t i = 0; i < fields_.size(); i++) {
    if (i > 0) os << ", ";
    os << fields_.at(i).name << ": ";
    fields_.at(i).type->WriteRefString(os);
  }
  os << ">";
}

int64_t Struct::size() const {
  int64_t size = 0;
  for (const Field& field : fields_) {
    size += field.type->size();
  }
  return size;
}

ir::Alignment Struct::alignment() const {
  ir::Alignment alignment = ir::Alignment::kNoAlignment;
  for (const Field& field : fields_) {
    alignment = std::max(alignment, field.type->alignment());
  }
  return alignment;
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

StructBuilder::StructBuilder() { struct_ = std::unique_ptr<Struct>(new Struct()); }

void StructBuilder::AddField(std::string name, const ir::Type* field_type) {
  struct_->fields_.push_back(Struct::Field{.name = name, .type = field_type});
}

const Struct* empty_struct() { return &kEmptyStruct; }

void Interface::WriteRefString(std::ostream& os) const {
  if (methods_.empty()) {
    os << "linterface";
    return;
  }
  os << "linterface<";
  for (std::size_t i = 0; i < methods_.size(); i++) {
    if (i > 0) os << ", ";
    os << methods_.at(i).name << ": (";
    for (std::size_t j = 0; methods_.at(i).parameters.size(); j++) {
      if (j > 0) os << ", ";
      methods_.at(i).parameters.at(j)->WriteRefString(os);
    }
    os << ") => (";
    for (std::size_t j = 0; methods_.at(i).results.size(); j++) {
      if (j > 0) os << ", ";
      methods_.at(i).results.at(j)->WriteRefString(os);
    }
    os << ")";
  }
  os << ">";
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

InterfaceBuilder::InterfaceBuilder() { interface_ = std::unique_ptr<Interface>(new Interface()); }

void InterfaceBuilder::AddMethod(std::string name, std::vector<const ir::Type*> parameters,
                                 std::vector<const ir::Type*> results) {
  interface_->methods_.push_back(
      Interface::Method{.name = name, .parameters = parameters, .results = results});
}

const Interface* empty_interface() { return &kEmptyInterface; }
const TypeID* type_id() { return &kTypeId; }

}  // namespace ir_ext
}  // namespace lang
