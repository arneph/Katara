//
//  data_instrs_translator.cc
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "data_instrs_translator.h"

#include <cstdint>

#include "src/common/logging/logging.h"
#include "src/ir/representation/values.h"
#include "src/x86_64/instrs/control_flow_instrs.h"
#include "src/x86_64/instrs/data_instrs.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/ir_translator/call_generator.h"
#include "src/x86_64/ir_translator/context.h"
#include "src/x86_64/ir_translator/mov_generator.h"
#include "src/x86_64/ir_translator/size_translator.h"
#include "src/x86_64/ir_translator/temporary_reg.h"
#include "src/x86_64/ir_translator/value_translator.h"
#include "src/x86_64/ops.h"

namespace ir_to_x86_64_translator {

void TranslateMovInstr(ir::MovInstr* ir_mov_instr, BlockContext& ctx) {
  x86_64::RM x86_64_result = TranslateComputed(ir_mov_instr->result().get(), ctx.func_ctx());
  x86_64::Operand x86_64_origin = TranslateValue(
      ir_mov_instr->origin().get(), IntNarrowing::k64To32BitIfPossible, ctx.func_ctx());

  GenerateMov(x86_64_result, x86_64_origin, ir_mov_instr, ctx);
}

void TranslateMallocInstr(ir::MallocInstr* ir_malloc_instr, BlockContext& ctx) {
  x86_64::FuncRef malloc_ref(ctx.x86_64_program()->declared_funcs().at("malloc"));
  GenerateCall(ir_malloc_instr, malloc_ref, /*ir_results=*/{ir_malloc_instr->result().get()},
               /*ir_args=*/{ir_malloc_instr->size().get()}, ctx);
}

void TranslateLoadInstr(ir::LoadInstr* ir_load_instr, BlockContext& ctx) {
  ir::Value* ir_address = ir_load_instr->address().get();
  ir::Computed* ir_result = ir_load_instr->result().get();

  x86_64::Operand x86_64_address =
      TranslateValue(ir_address, IntNarrowing::k64To32BitIfPossible, ctx.func_ctx());
  x86_64::RM x86_64_result = TranslateComputed(ir_result, ctx.func_ctx());
  x86_64::Size x86_64_size = TranslateSizeOfType(ir_result->type());

  x86_64::Mem mem(x86_64_size, /*disp=*/int32_t{0});
  std::optional<TemporaryReg> tmp;
  if (x86_64_address.is_imm()) {
    mem = x86_64::Mem(x86_64_size, /*disp=*/int32_t(x86_64_address.imm().value()));

  } else {
    x86_64::Reg x86_64_address_reg = x86_64::rax;
    if (x86_64_address.is_reg()) {
      x86_64_address_reg = x86_64_address.reg();
    } else {
      tmp =
          TemporaryReg::ForOperand(x86_64_address, /*can_use_result_reg=*/true, ir_load_instr, ctx);
      x86_64_address_reg = tmp->reg();
    }
    mem = x86_64::Mem(x86_64_size, /*base_reg=*/uint8_t(x86_64_address_reg.reg()));
  }

  if (x86_64_result.is_reg()) {
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(x86_64_result, mem);
  } else if (x86_64_result.is_mem()) {
    if (!tmp.has_value()) {
      tmp = TemporaryReg::Prepare(x86_64_size, /*can_use_result_reg=*/false, ir_load_instr, ctx);
    }
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(tmp->reg(), mem);
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(x86_64_result, tmp->reg());
  } else {
    common::fail("unexpected load result operand");
  }

  if (tmp.has_value()) {
    tmp->Restore(ctx);
  }
}

void TranslateStoreInstr(ir::StoreInstr* ir_store_instr, BlockContext& ctx) {
  ir::Value* ir_address = ir_store_instr->address().get();
  ir::Value* ir_value = ir_store_instr->value().get();

  x86_64::Operand x86_64_address =
      TranslateValue(ir_address, IntNarrowing::k64To32BitIfPossible, ctx.func_ctx());
  x86_64::Operand x86_64_value = TranslateValue(ir_value, IntNarrowing::kNone, ctx.func_ctx());
  x86_64::Size x86_64_size = TranslateSizeOfType(ir_value->type());

  std::optional<TemporaryReg> value_tmp;
  if ((x86_64_value.is_imm() && x86_64_value.size() == 64) || x86_64_value.is_mem()) {
    value_tmp =
        TemporaryReg::ForOperand(x86_64_value, /*can_use_result_reg=*/false, ir_store_instr, ctx);
    x86_64_value = value_tmp->reg();
  }

  std::optional<TemporaryReg> address_tmp;
  x86_64::Mem mem(x86_64_size, /*disp=*/int32_t{0});
  if (x86_64_address.is_imm()) {
    mem = x86_64::Mem(x86_64_size, /*disp=*/int32_t(x86_64_address.imm().value()));

  } else {
    x86_64::Reg x86_64_address_reg = x86_64::rax;
    if (x86_64_address.is_reg()) {
      x86_64_address_reg = x86_64_address.reg();
    } else {
      address_tmp = TemporaryReg::ForOperand(x86_64_address, /*can_use_result_reg=*/false,
                                             ir_store_instr, ctx);
      x86_64_address_reg = address_tmp->reg();
    }
    mem = x86_64::Mem(x86_64_size, /*base_reg=*/uint8_t(x86_64_address_reg.reg()));
  }

  ctx.x86_64_block()->AddInstr<x86_64::Mov>(mem, x86_64_value);

  if (address_tmp.has_value()) {
    address_tmp->Restore(ctx);
  }
  if (value_tmp.has_value()) {
    value_tmp->Restore(ctx);
  }
}

void TranslateFreeInstr(ir::FreeInstr* ir_free_instr, BlockContext& ctx) {
  x86_64::FuncRef free_ref(ctx.x86_64_program()->declared_funcs().at("free"));
  GenerateCall(ir_free_instr, free_ref, /*ir_results=*/{},
               /*ir_args=*/{ir_free_instr->address().get()}, ctx);
}

}  // namespace ir_to_x86_64_translator
