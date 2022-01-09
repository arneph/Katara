//
//  func_translator.cc
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "func_translator.h"

#include <algorithm>
#include <vector>

#include "src/ir/representation/block.h"
#include "src/x86_64/block.h"
#include "src/x86_64/instrs/control_flow_instrs.h"
#include "src/x86_64/instrs/data_instrs.h"
#include "src/x86_64/ir_translator/instrs_translator.h"
#include "src/x86_64/ir_translator/register_allocator.h"

namespace ir_to_x86_64_translator {
namespace {

std::vector<const ir::Block*> GetSortedBlocksInFunc(const ir::Func* ir_func) {
  std::vector<const ir::Block*> ir_blocks;
  ir_blocks.reserve(ir_func->blocks().size());
  for (auto& ir_block : ir_func->blocks()) {
    ir_blocks.push_back(ir_block.get());
  }
  std::sort(ir_blocks.begin(), ir_blocks.end(),
            [ir_func](const ir::Block* lhs, const ir::Block* rhs) -> bool {
              // entry block always first:
              if (lhs == ir_func->entry_block()) {
                return true;
              } else if (rhs == ir_func->entry_block()) {
                return false;
              }
              // otherwise sort by block number:
              return lhs->number() < rhs->number();
            });
  return ir_blocks;
}

std::vector<x86_64::Block*> PrepareBlocks(std::vector<const ir::Block*>& ir_blocks,
                                          FuncContext& func_ctx) {
  std::vector<x86_64::Block*> x86_64_blocks;
  x86_64_blocks.reserve(ir_blocks.size());
  for (const ir::Block* ir_block : ir_blocks) {
    x86_64::Block* x86_64_block = func_ctx.x86_64_func()->AddBlock();

    x86_64_blocks.push_back(x86_64_block);
    func_ctx.set_x86_64_block_num_for_ir_block_num(ir_block->number(), x86_64_block->block_num());
  }
  return x86_64_blocks;
}

void TranslateBlock(BlockContext& ctx) {
  for (auto& ir_instr : ctx.ir_block()->instrs()) {
    TranslateInstr(ir_instr.get(), ctx);
  }
}

void ForEachUsedCalleeSavedRegister(std::function<void(x86_64::Reg)> func, FuncContext& ctx) {
  for (ir_info::color_t color : ctx.used_colors()) {
    x86_64::RM rm = ColorAndSizeToOperand(color, x86_64::k64);
    if (!rm.is_reg()) {
      continue;
    }
    x86_64::Reg reg = rm.reg();
    if (SavingBehaviourForReg(reg) != RegSavingBehaviour::kByCallee) {
      continue;
    }
    func(reg);
  }
}

void GenerateFuncPrologue(BlockContext& ctx) {
  std::vector<std::unique_ptr<x86_64::Instr>>::const_iterator it =
      ctx.x86_64_block()->instrs().begin();
  it = ctx.x86_64_block()->InsertInstr<x86_64::Push>(it, x86_64::rbp);
  ++it;
  it = ctx.x86_64_block()->InsertInstr<x86_64::Mov>(it, x86_64::rbp, x86_64::rsp);
  ++it;
  ForEachUsedCalleeSavedRegister(
      [&it, &ctx](x86_64::Reg reg) {
        it = ctx.x86_64_block()->InsertInstr<x86_64::Push>(it, reg);
        ++it;
      },
      ctx.func_ctx());
  // TODO: reserve stack space
}

void GenerateFuncEpilogue(BlockContext& ctx) {
  // TODO: revert stack pointer
  ForEachUsedCalleeSavedRegister(
      [&ctx](x86_64::Reg reg) { ctx.x86_64_block()->AddInstr<x86_64::Pop>(reg); }, ctx.func_ctx());
  ctx.x86_64_block()->AddInstr<x86_64::Pop>(x86_64::rbp);
  ctx.x86_64_block()->AddInstr<x86_64::Ret>();
}

}  // namespace

void TranslateFunc(FuncContext& func_ctx) {
  std::vector<const ir::Block*> ir_blocks = GetSortedBlocksInFunc(func_ctx.ir_func());
  std::vector<x86_64::Block*> x86_64_blocks = PrepareBlocks(ir_blocks, func_ctx);

  for (std::size_t i = 0; i < ir_blocks.size(); i++) {
    const ir::Block* ir_block = ir_blocks.at(i);
    x86_64::Block* x86_64_block = x86_64_blocks.at(i);

    BlockContext block_ctx(func_ctx, ir_block, x86_64_block);
    TranslateBlock(block_ctx);
  }

  for (std::size_t i = 0; i < ir_blocks.size(); i++) {
    const ir::Block* ir_block = ir_blocks.at(i);
    x86_64::Block* x86_64_block = x86_64_blocks.at(i);

    BlockContext block_ctx(func_ctx, ir_block, x86_64_block);

    if (ir_block->number() == func_ctx.ir_func()->entry_block_num()) {
      GenerateFuncPrologue(block_ctx);
    }
    if (ir_block->instrs().back()->instr_kind() == ir::InstrKind::kReturn) {
      GenerateFuncEpilogue(block_ctx);
    }
  }
}

}  // namespace ir_to_x86_64_translator
