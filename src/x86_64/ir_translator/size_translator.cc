//
//  size_translator.cc
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "size_translator.h"

#include "src/common/logging/logging.h"

namespace ir_to_x86_64_translator {

x86_64::Size TranslateSizeOfType(const ir::Type* ir_type) {
  switch (ir_type->type_kind()) {
    case ir::TypeKind::kBool:
      return x86_64::Size::k8;
    case ir::TypeKind::kInt:
      return TranslateSizeOfIntType(static_cast<const ir::IntType*>(ir_type));
    case ir::TypeKind::kPointer:
    case ir::TypeKind::kFunc:
      return x86_64::Size::k64;
    default:
      common::fail("unexpected int binary operand type");
  }
}

x86_64::Size TranslateSizeOfIntType(const ir::IntType* ir_int_type) {
  return TranslateSizeOfIntType(ir_int_type->int_type());
}

x86_64::Size TranslateSizeOfIntType(common::atomics::IntType common_int_type) {
  return x86_64::Size(common::atomics::BitSizeOf(common_int_type));
}

}  // namespace ir_to_x86_64_translator
