//
//  arithmetic_logic_instrs_translator_test.cc
//  Katara
//
//  Created by Arne Philipeit on 8/12/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "src/x86_64/ir_translator/instr_translators/arithmetic_logic_instrs_translator.h"

#include <memory>

#include "gtest/gtest.h"
#include "src/ir/analyzers/interference_graph_builder.h"
#include "src/ir/analyzers/live_range_analyzer.h"
#include "src/ir/builder/block_builder.h"
#include "src/ir/builder/func_builder.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"

namespace ir_to_x86_64_translator {

TEST(TranslateBoolNotInstrTest, HandlesRegOperandAndResult) {
  ir::Program ir_program;
  ir_builder::FuncBuilder fb = ir_builder::FuncBuilder::ForNewFuncInProgram(&ir_program);
  std::shared_ptr<ir::Computed> ir_operand = fb.AddArg(ir::bool_type());
  fb.AddResultType(ir::bool_type());

  ir_builder::BlockBuilder bb = fb.AddEntryBlock();
  std::shared_ptr<ir::Computed> ir_result =
      std::static_pointer_cast<ir::Computed>(bb.BoolNot(ir_operand));
  bb.Return({ir_result});

  ir::Func* ir_func = fb.func();
  ir::Block* ir_block = ir_func->entry_block();
  ir::BoolNotInstr* ir_bool_not_instr =
      static_cast<ir::BoolNotInstr*>(bb.block()->instrs().front().get());

  const ir_info::FuncLiveRanges func_live_ranges = ir_analyzers::FindLiveRangesForFunc(ir_func);
  const ir_info::InterferenceGraph interference_graph =
      ir_analyzers::BuildInterferenceGraphForFunc(ir_func, func_live_ranges);
  ir_info::InterferenceGraphColors interference_graph_colors;
  interference_graph_colors.SetColor(ir_operand->number(), 0);
  interference_graph_colors.SetColor(ir_result->number(), 1);

  x86_64::Program x86_64_program;
  x86_64::Func* x86_64_func = x86_64_program.DefineFunc("test_func");
  x86_64::Block* x86_64_block = x86_64_func->AddBlock();

  ProgramContext program_ctx(&ir_program, &x86_64_program, /*malloc_func_num=*/0,
                             /*free_func_num=*/0);
  FuncContext func_ctx(program_ctx, ir_func, x86_64_func, func_live_ranges, interference_graph,
                       interference_graph_colors);
  BlockContext block_ctx(func_ctx, ir_block, x86_64_block);

  TranslateBoolNotInstr(ir_bool_not_instr, block_ctx);

  EXPECT_EQ(x86_64_block->instrs().size(), 2);
  EXPECT_EQ(x86_64_block->instrs().at(0)->ToString(), "mov cl,al");
  EXPECT_EQ(x86_64_block->instrs().at(1)->ToString(), "not cl");
}

}  // namespace ir_to_x86_64_translator
