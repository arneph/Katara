//
//  value_translator.cc
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "value_translator.h"

#include <cstdint>

#include "src/common/logging.h"
#include "src/ir/info/interference_graph.h"
#include "src/ir/representation/types.h"
#include "src/x86_64/ir_translator/register_allocator.h"

namespace ir_to_x86_64_translator {

x86_64::Operand TranslateValue(ir::Value* value, FuncContext& ctx) {
  switch (value->kind()) {
    case ir::Value::Kind::kConstant:
      switch (static_cast<const ir::AtomicType*>(value->type())->type_kind()) {
        case ir::TypeKind::kBool:
          return TranslateBoolConstant(static_cast<ir::BoolConstant*>(value));
        case ir::TypeKind::kInt:
          return TranslateIntConstant(static_cast<ir::IntConstant*>(value));
        case ir::TypeKind::kPointer:
          return TranslatePointerConstant(static_cast<ir::PointerConstant*>(value));
        case ir::TypeKind::kFunc:
          return TranslateFuncConstant(static_cast<ir::FuncConstant*>(value), ctx.program_ctx());
        default:
          common::fail("unsupported constant kind");
      }
    case ir::Value::Kind::kComputed:
      return TranslateComputed(static_cast<ir::Computed*>(value), ctx);
    default:
      common::fail("unsupported value kind");
  }
}

x86_64::Imm TranslateBoolConstant(ir::BoolConstant* constant) {
  return constant->value() ? x86_64::Imm(int8_t{1}) : x86_64::Imm(int8_t{0});
}

x86_64::Imm TranslateIntConstant(ir::IntConstant* constant) {
  switch (constant->value().type()) {
    case common::IntType::kI8:
    case common::IntType::kU8:
      return x86_64::Imm(int8_t(constant->value().AsInt64()));
    case common::IntType::kI16:
    case common::IntType::kU16:
      return x86_64::Imm(int16_t(constant->value().AsInt64()));
    case common::IntType::kI32:
    case common::IntType::kU32:
      return x86_64::Imm(int32_t(constant->value().AsInt64()));
    case common::IntType::kI64: {
      common::Int value = constant->value();
      if (value.CanConvertTo(common::IntType::kI32)) {
        return x86_64::Imm(int32_t(value.AsInt64()));
      } else {
        return x86_64::Imm(int64_t(value.AsInt64()));
      }
    }
    case common::IntType::kU64: {
      common::Int value = constant->value();
      if (value.CanConvertTo(common::IntType::kU32)) {
        return x86_64::Imm(int32_t(value.AsInt64()));
      } else {
        return x86_64::Imm(int64_t(value.AsInt64()));
      }
    }
  }
}

x86_64::Imm TranslatePointerConstant(ir::PointerConstant* constant) {
  if (constant == ir::NilPointer().get()) {
    return x86_64::Imm(int32_t{0});
  }
  return x86_64::Imm(constant->value());
}

x86_64::Operand TranslateFuncConstant(ir::FuncConstant* constant, ProgramContext& ctx) {
  if (constant == ir::NilFunc().get()) {
    return x86_64::Imm(int32_t{0});
  }
  return x86_64::FuncRef(ctx.x86_64_func_num_for_ir_func_num(constant->value()));
}

x86_64::RM TranslateComputed(ir::Computed* computed, FuncContext& ctx) {
  ir_info::color_t color = ctx.interference_graph_colors().GetColor(computed->number());
  int8_t ir_size = static_cast<const ir::AtomicType*>(computed->type())->bit_size();
  x86_64::Size x86_64_size = x86_64::Size(ir_size);
  return ColorAndSizeToOperand(color, x86_64_size);
}

x86_64::BlockRef TranslateBlockValue(ir::block_num_t block_value, FuncContext& ctx) {
  return x86_64::BlockRef(ctx.x86_64_block_num_for_ir_block_num(block_value));
}

}  // namespace ir_to_x86_64_translator
