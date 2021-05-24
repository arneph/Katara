//
//  types_builder.h
//  Katara
//
//  Created by Arne Philipeit on 5/23/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_builder_types_builder_h
#define lang_ir_builder_types_builder_h

#include <memory>

#include "ir/representation/program.h"
#include "ir/representation/types.h"
#include "lang/representation/ir_extension/types.h"
#include "lang/representation/types/expr_info.h"
#include "lang/representation/types/info.h"
#include "lang/representation/types/initializer.h"
#include "lang/representation/types/objects.h"
#include "lang/representation/types/package.h"
#include "lang/representation/types/scope.h"
#include "lang/representation/types/selection.h"
#include "lang/representation/types/types.h"
#include "lang/representation/types/types_util.h"

namespace lang {
namespace ir_builder {

class TypesBuilder {
 public:
  TypesBuilder(types::Info* type_info, std::unique_ptr<ir::Program>& program);

  ir_ext::String* ir_string_type() const { return ir_string_type_; }
  ir_ext::Struct* ir_empty_struct() const { return ir_empty_struct_; }
  ir_ext::Interface* ir_empty_interface() const { return ir_empty_interface_; }
  ir_ext::TypeID* ir_type_id() const { return ir_type_id_; }

  ir::Type* BuildType(types::Type* types_type);
  ir::Type* BuildTypeForBasic(types::Basic* types_basic);
  ir_ext::Pointer* BuildTypeForPointer(types::Pointer* types_pointer);
  ir_ext::Pointer* BuildStrongPointerToType(types::Type* types_element_type);
  ir_ext::Pointer* BuildWeakPointerToType(types::Type* types_element_type);
  ir_ext::Array* BuildTypeForContainer(types::Container* types_container);
  ir_ext::Struct* BuildTypeForStruct(types::Struct* types_struct);
  ir_ext::Interface* BuildTypeForInterface(types::Interface* types_interface);

 private:
  types::Info* type_info_;
  std::unique_ptr<ir::Program>& program_;

  ir_ext::String* ir_string_type_;
  ir_ext::Struct* ir_empty_struct_;
  ir_ext::Interface* ir_empty_interface_;
  ir_ext::TypeID* ir_type_id_;

  std::unordered_map<ir::Type*, ir_ext::Pointer*> ir_element_type_to_ir_strong_pointer_lookup_;
  std::unordered_map<ir::Type*, ir_ext::Pointer*> ir_element_type_to_ir_weak_pointer_lookup_;
  std::unordered_map<types::Pointer*, ir_ext::Pointer*> types_pointer_to_ir_pointer_lookup_;
  std::unordered_map<types::Container*, ir_ext::Array*> types_container_to_ir_array_lookup_;
  std::unordered_map<types::Struct*, ir_ext::Struct*> types_struct_to_ir_struct_lookup_;
  std::unordered_map<types::Interface*, ir_ext::Interface*> types_interface_to_ir_interface_lookup_;
};

}  // namespace ir_builder
}  // namespace lang

#endif /* lang_ir_builder_types_builder_h */
