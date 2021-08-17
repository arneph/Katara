//
//  instr_translator_test_setup.h
//  Katara-tests
//
//  Created by Arne Philipeit on 8/14/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_testing_instr_translator_test_setup_h
#define ir_to_x86_64_translator_testing_instr_translator_test_setup_h

#include <optional>

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
#include "src/x86_64/block.h"
#include "src/x86_64/func.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/ir_translator/context.h"
#include "src/x86_64/ops.h"
#include "src/x86_64/program.h"

namespace ir_to_x86_64_translator {

class InstrTranslatorTest : public ::testing::Test {
 protected:
  InstrTranslatorTest()
      : ir_func_builder_(ir_builder::FuncBuilder::ForNewFuncInProgram(&ir_program_)),
        ir_block_builder_(ir_func_builder_.AddEntryBlock()),
        x86_64_func_(x86_64_program_.DefineFunc("test_func")),
        x86_64_block_(x86_64_func_->AddBlock()) {}

  ir::Program* ir_program() { return &ir_program_; }
  ir::Func* ir_func() { return ir_func_builder_.func(); }
  ir::Block* ir_block() { return ir_block_builder_.block(); }

  ir_builder::FuncBuilder& ir_func_builder() { return ir_func_builder_; }
  ir_builder::BlockBuilder& ir_block_builder() { return ir_block_builder_; }

  void GenerateIRInfo() {
    func_live_ranges_.emplace(ir_analyzers::FindLiveRangesForFunc(ir_func()));
    interference_graph_.emplace(
        ir_analyzers::BuildInterferenceGraphForFunc(ir_func(), func_live_ranges_.value()));
  }

  const ir_info::FuncLiveRanges& func_live_ranges() { return func_live_ranges_.value(); }
  const ir_info::InterferenceGraph& interference_graph() { return interference_graph_.value(); }
  ir_info::InterferenceGraphColors& interference_graph_colors() {
    return interference_graph_colors_;
  }

  x86_64::Program* x86_64_program() { return &x86_64_program_; }
  x86_64::Func* x86_64_func() { return x86_64_func_; }
  x86_64::Block* x86_64_block() { return x86_64_block_; }

  void GenerateTranslationContexts() {
    program_ctx_.emplace(ProgramContext(&ir_program_, &x86_64_program_, /*malloc_func_num=*/0,
                                        /*free_func_num=*/0));
    func_ctx_.emplace(FuncContext(program_ctx_.value(), ir_func(), x86_64_func(),
                                  func_live_ranges(), interference_graph(),
                                  interference_graph_colors()));
    block_ctx_.emplace(BlockContext(func_ctx_.value(), ir_block(), x86_64_block()));
  }

  ProgramContext& program_ctx() { return program_ctx_.value(); }
  FuncContext& func_ctx() { return func_ctx_.value(); }
  BlockContext& block_ctx() { return block_ctx_.value(); }

 private:
  ir::Program ir_program_;
  ir_builder::FuncBuilder ir_func_builder_;
  ir_builder::BlockBuilder ir_block_builder_;

  std::optional<const ir_info::FuncLiveRanges> func_live_ranges_;
  std::optional<const ir_info::InterferenceGraph> interference_graph_;
  ir_info::InterferenceGraphColors interference_graph_colors_;

  x86_64::Program x86_64_program_;
  x86_64::Func* x86_64_func_;
  x86_64::Block* x86_64_block_;

  std::optional<ProgramContext> program_ctx_;
  std::optional<FuncContext> func_ctx_;
  std::optional<BlockContext> block_ctx_;
};

}  // namespace ir_to_x86_64_translator

#endif /* ir_to_x86_64_translator_testing_instr_translator_test_setup_h */
