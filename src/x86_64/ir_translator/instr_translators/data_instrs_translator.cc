//
//  data_instrs_translator.cc
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "data_instrs_translator.h"

#include <cstdint>

#include "src/common/logging.h"
#include "src/ir/representation/values.h"
#include "src/x86_64/instrs/data_instrs.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/ir_translator/context.h"
#include "src/x86_64/ir_translator/mov_generator.h"
#include "src/x86_64/ir_translator/size_translator.h"
#include "src/x86_64/ir_translator/temporary_reg.h"
#include "src/x86_64/ir_translator/value_translator.h"
#include "src/x86_64/ops.h"

namespace ir_to_x86_64_translator {

void TranslateMovInstr(ir::MovInstr* ir_mov_instr, BlockContext& ctx) {
  auto ir_result = ir_mov_instr->result().get();
  auto ir_origin = ir_mov_instr->origin().get();

  GenerateMov(ir_result, ir_origin, ir_mov_instr, ctx);
}

void TranslateMallocInstr(ir::MallocInstr* ir_malloc_instr, BlockContext& ctx) {
  // TODO: implement
}

void TranslateLoadInstr(ir::LoadInstr* ir_load_instr, BlockContext& ctx) {
  ir::Value* ir_address = ir_load_instr->address().get();
  ir::Computed* ir_result = ir_load_instr->result().get();

  x86_64::Operand x86_64_address_holder = TranslateValue(ir_address, ctx.func_ctx());
  x86_64::RM x86_64_result = TranslateComputed(ir_result, ctx.func_ctx());

  bool requires_address_reg = false;
  bool address_reg_needs_preservation = !x86_64_result.is_reg();
  x86_64::Reg x86_64_address_reg = (x86_64_result.is_reg()) ? x86_64_result.reg() : x86_64::rax;
  if (x86_64_address_holder.is_mem()) {
    requires_address_reg = true;

  } else if (x86_64_address_holder.is_imm()) {
    common::Int v = common::Int(static_cast<ir::PointerConstant*>(ir_address)->value());
    requires_address_reg = !v.CanConvertTo(common::IntType::kI32);

  } else if (x86_64_address_holder.is_reg()) {
    x86_64_address_reg = x86_64_address_holder.reg();
    requires_address_reg = true;
    address_reg_needs_preservation = false;
  }

  if (requires_address_reg) {
    if (address_reg_needs_preservation) {
      ctx.x86_64_block()->AddInstr<x86_64::Push>(x86_64_address_reg);
    }
    if (x86_64_address_holder != x86_64_address_reg) {
      ctx.x86_64_block()->AddInstr<x86_64::Mov>(x86_64_address_reg, x86_64_address_holder);
    }
  }

  x86_64::Size x86_64_size = TranslateSizeOfType(ir_result->type());
  x86_64::Mem mem(x86_64_size, int32_t{0});
  if (requires_address_reg) {
    mem = x86_64::Mem(x86_64_size, /*base_reg=*/uint8_t(x86_64_address_reg.reg()));
  } else {
    if (!x86_64_address_holder.is_imm()) {
      common::fail("unexpected load address kind");
    }
    mem = x86_64::Mem(x86_64_size, /*disp=*/int32_t(x86_64_address_holder.imm().value()));
  }
  ctx.x86_64_block()->AddInstr<x86_64::Mov>(x86_64_result, mem);

  if (requires_address_reg && address_reg_needs_preservation) {
    ctx.x86_64_block()->AddInstr<x86_64::Pop>(x86_64_address_reg);
  }
}

void TranslateStoreInstr(ir::StoreInstr* ir_store_instr, BlockContext& ctx) {
  ir::Value* ir_address = ir_store_instr->address().get();
  ir::Value* ir_value = ir_store_instr->value().get();

  x86_64::Operand x86_64_address_holder = TranslateValue(ir_address, ctx.func_ctx());
  x86_64::Operand x86_64_value = TranslateValue(ir_value, ctx.func_ctx());

  bool requires_value_reg = false;
  x86_64::Reg x86_64_value_reg = x86_64::rax;
  if (x86_64_value.is_imm() && x86_64_value.imm().size() == x86_64::k64) {
    common::Int v = common::Int(x86_64_value.imm().value());
    if (v.CanConvertTo(common::IntType::kI32)) {
      v.ConvertTo(common::IntType::kI32);
      x86_64_value = x86_64::Imm(int32_t(v.AsInt64()));
    } else {
      requires_value_reg = true;
    }
  }

  bool requires_address_reg = false;
  bool address_reg_needs_preservation = true;
  x86_64::Reg x86_64_address_reg = x86_64::rax;
  if (x86_64_address_holder.is_mem()) {
    requires_address_reg = true;

  } else if (x86_64_address_holder.is_imm()) {
    common::Int v = common::Int(static_cast<ir::PointerConstant*>(ir_address)->value());
    if (v.CanConvertTo(common::IntType::kI32)) {
      v = v.ConvertTo(common::IntType::kI32);
      x86_64_address_holder = x86_64::Imm(int32_t(v.AsInt64()));
    } else {
      requires_address_reg = true;
    }

  } else if (x86_64_address_holder.is_reg()) {
    x86_64_address_reg = x86_64_address_holder.reg();
    requires_address_reg = true;
    address_reg_needs_preservation = false;
  }

  if (x86_64_value_reg == x86_64_address_reg) {
    x86_64_value_reg = x86_64::rdx;
  }

  if (requires_value_reg) {
    ctx.x86_64_block()->AddInstr<x86_64::Push>(x86_64_value_reg);
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(x86_64_value_reg, x86_64_value);
    x86_64_value = x86_64_value_reg;
  }

  if (requires_address_reg) {
    if (address_reg_needs_preservation) {
      ctx.x86_64_block()->AddInstr<x86_64::Push>(x86_64_address_reg);
    }
    if (x86_64_address_holder != x86_64_address_reg) {
      ctx.x86_64_block()->AddInstr<x86_64::Mov>(x86_64_address_reg, x86_64_address_holder);
    }
  }

  x86_64::Size x86_64_size = TranslateSizeOfType(ir_value->type());
  x86_64::Mem mem(x86_64_size, int32_t{0});
  if (requires_address_reg) {
    mem = x86_64::Mem(x86_64_size, /*base_reg=*/uint8_t(x86_64_address_reg.reg()));
  } else {
    if (!x86_64_address_holder.is_imm()) {
      common::fail("unexpected load address kind");
    }
    mem = x86_64::Mem(x86_64_size, /*disp=*/int32_t(x86_64_address_holder.imm().value()));
  }
  ctx.x86_64_block()->AddInstr<x86_64::Mov>(mem, x86_64_value);

  if (requires_address_reg && address_reg_needs_preservation) {
    ctx.x86_64_block()->AddInstr<x86_64::Pop>(x86_64_address_reg);
  }
  if (requires_value_reg) {
    ctx.x86_64_block()->AddInstr<x86_64::Pop>(x86_64_value_reg);
  }
}

void TranslateFreeInstr(ir::FreeInstr* ir_free_instr, BlockContext& ctx) {
  // TODO: implement
}

}  // namespace ir_to_x86_64_translator
