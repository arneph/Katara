//
//  type_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 5/23/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "type_builder.h"

#include "src/common/logging/logging.h"

namespace lang {
namespace ir_builder {

TypeBuilder::TypeBuilder(types::Info* type_info, std::unique_ptr<ir::Program>& program)
    : type_info_(type_info), program_(program) {}

const ir::Type* TypeBuilder::BuildType(types::Type* types_type) {
  switch (types_type->type_kind()) {
    case types::TypeKind::kBasic:
      return BuildTypeForBasic(static_cast<types::Basic*>(types_type));
    case types::TypeKind::kPointer:
      return BuildTypeForPointer(static_cast<types::Pointer*>(types_type));
    case types::TypeKind::kArray:
    case types::TypeKind::kSlice:
      return BuildTypeForContainer(static_cast<types::Container*>(types_type));
    case types::TypeKind::kTypeParameter:
      return BuildType(static_cast<types::TypeParameter*>(types_type)->interface());
    case types::TypeKind::kNamedType:
      return BuildType(static_cast<types::NamedType*>(types_type)->underlying());
    case types::TypeKind::kTypeInstance: {
      types::InfoBuilder type_info_builder = type_info_->builder();
      types::Type* underlying =
          types::UnderlyingOf(static_cast<types::TypeInstance*>(types_type), type_info_builder);
      return BuildType(underlying);
    }
    case types::TypeKind::kTuple:
      common::fail("attempted to convert types tuple to IR type");
    case types::TypeKind::kSignature:
      return ir::func_type();
    case types::TypeKind::kStruct:
      return BuildTypeForStruct(static_cast<types::Struct*>(types_type));
    case types::TypeKind::kInterface:
      return BuildTypeForInterface(static_cast<types::Interface*>(types_type));
  }
}

const ir::Type* TypeBuilder::BuildTypeForBasic(types::Basic* types_basic) {
  switch (types_basic->kind()) {
    case types::Basic::kBool:
    case types::Basic::kUntypedBool:
      return ir::bool_type();
    case types::Basic::kInt8:
      return ir::i8();
    case types::Basic::kInt16:
      return ir::i16();
    case types::Basic::kInt32:
    case types::Basic::kUntypedRune:
      return ir::i32();
    case types::Basic::kInt:
    case types::Basic::kInt64:
    case types::Basic::kUntypedInt:
      return ir::i64();
    case types::Basic::kUint8:
      return ir::u8();
    case types::Basic::kUint16:
      return ir::u16();
    case types::Basic::kUint32:
      return ir::u32();
    case types::Basic::kUint:
    case types::Basic::kUint64:
      return ir::u64();
    case types::Basic::kString:
    case types::Basic::kUntypedString:
      return ir_ext::string();
    case types::Basic::kUntypedNil:
      return ir::pointer_type();
    default:
      common::fail("unexpected basic type");
  }
}

const ir_ext::SharedPointer* TypeBuilder::BuildTypeForPointer(types::Pointer* types_pointer) {
  if (auto it = types_pointer_to_ir_pointer_lookup_.find(types_pointer);
      it != types_pointer_to_ir_pointer_lookup_.end()) {
    return it->second;
  }
  const ir_ext::SharedPointer* ir_pointer;
  switch (types_pointer->kind()) {
    case types::Pointer::Kind::kStrong:
      ir_pointer = BuildStrongPointerToType(types_pointer->element_type());
      break;
    case types::Pointer::Kind::kWeak:
      ir_pointer = BuildWeakPointerToType(types_pointer->element_type());
      break;
  }
  types_pointer_to_ir_pointer_lookup_.insert({types_pointer, ir_pointer});
  return ir_pointer;
}

const ir_ext::SharedPointer* TypeBuilder::BuildStrongPointerToType(
    types::Type* types_element_type) {
  const ir::Type* ir_element_type = BuildType(types_element_type);
  if (auto it = ir_element_type_to_ir_strong_pointer_lookup_.find(ir_element_type);
      it != ir_element_type_to_ir_strong_pointer_lookup_.end()) {
    return it->second;
  }
  ir_ext::SharedPointer* ir_pointer =
      static_cast<ir_ext::SharedPointer*>(program_->type_table().AddType(
          std::make_unique<ir_ext::SharedPointer>(/*is_strong=*/true, ir_element_type)));
  ir_element_type_to_ir_strong_pointer_lookup_.insert({ir_element_type, ir_pointer});
  return ir_pointer;
}

const ir_ext::SharedPointer* TypeBuilder::BuildWeakPointerToType(types::Type* types_element_type) {
  const ir::Type* ir_element_type = BuildType(types_element_type);
  if (auto it = ir_element_type_to_ir_weak_pointer_lookup_.find(ir_element_type);
      it != ir_element_type_to_ir_weak_pointer_lookup_.end()) {
    return it->second;
  }
  ir_ext::SharedPointer* ir_pointer =
      static_cast<ir_ext::SharedPointer*>(program_->type_table().AddType(
          std::make_unique<ir_ext::SharedPointer>(/*is_strong=*/false, ir_element_type)));
  ir_element_type_to_ir_weak_pointer_lookup_.insert({ir_element_type, ir_pointer});
  return ir_pointer;
}

const ir_ext::Array* TypeBuilder::BuildTypeForContainer(types::Container* types_container) {
  if (auto it = types_container_to_ir_array_lookup_.find(types_container);
      it != types_container_to_ir_array_lookup_.end()) {
    return it->second;
  }

  ir_ext::ArrayBuilder ir_array_builder;
  ir_ext::Array* ir_array = ir_array_builder.Get();
  types_container_to_ir_array_lookup_.insert({types_container, ir_array});

  types::Type* types_element = types_container->element_type();
  const ir::Type* ir_element = BuildType(types_element);
  ir_array_builder.SetElement(ir_element);
  if (types_container->type_kind() == types::TypeKind::kArray) {
    types::Array* types_array = static_cast<types::Array*>(types_container);
    ir_array_builder.SetFixedCount(types_array->length());
  }
  program_->type_table().AddType(ir_array_builder.Build());
  return ir_array;
}

const ir_ext::Struct* TypeBuilder::BuildTypeForStruct(types::Struct* types_struct) {
  if (types_struct->is_empty()) {
    return ir_ext::empty_struct();
  }
  if (auto it = types_struct_to_ir_struct_lookup_.find(types_struct);
      it != types_struct_to_ir_struct_lookup_.end()) {
    return it->second;
  }

  ir_ext::StructBuilder ir_struct_builder;
  ir_ext::Struct* ir_struct = ir_struct_builder.Get();
  types_struct_to_ir_struct_lookup_.insert({types_struct, ir_struct});

  for (types::Variable* types_field : types_struct->fields()) {
    std::string name = types_field->name();
    const ir::Type* ir_field_type = BuildType(types_field->type());
    ir_struct_builder.AddField(name, ir_field_type);
  }
  program_->type_table().AddType(ir_struct_builder.Build());
  return ir_struct;
}

const ir_ext::Interface* TypeBuilder::BuildTypeForInterface(types::Interface* types_interface) {
  if (types_interface->is_empty()) {
    return ir_ext::empty_interface();
  }
  if (auto it = types_interface_to_ir_interface_lookup_.find(types_interface);
      it != types_interface_to_ir_interface_lookup_.end()) {
    return it->second;
  }

  ir_ext::InterfaceBuilder ir_interface_builder;
  ir_ext::Interface* ir_interface = ir_interface_builder.Get();
  types_interface_to_ir_interface_lookup_.insert({types_interface, ir_interface});

  for (types::Func* types_method : types_interface->methods()) {
    ir_interface_builder.AddMethod(types_method->name(), {}, {});
  }
  program_->type_table().AddType(ir_interface_builder.Build());
  return ir_interface;
}

}  // namespace ir_builder
}  // namespace lang
