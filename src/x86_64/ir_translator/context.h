//
//  context.h
//  Katara
//
//  Created by Arne Philipeit on 8/10/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_context_h
#define ir_to_x86_64_translator_context_h

#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include "src/ir/info/block_live_ranges.h"
#include "src/ir/info/func_live_ranges.h"
#include "src/ir/info/interference_graph.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/x86_64/block.h"
#include "src/x86_64/func.h"
#include "src/x86_64/program.h"

namespace ir_to_x86_64_translator {

class ProgramContext {
 public:
  ProgramContext(const ir::Program* ir_program, x86_64::Program* x86_64_program,
                 x86_64::func_num_t malloc_func_num, x86_64::func_num_t free_func_num)
      : ir_program_(ir_program),
        x86_64_program_(x86_64_program),
        malloc_func_num_(malloc_func_num),
        free_func_num_(free_func_num) {}

  const ir::Program* ir_program() const { return ir_program_; }
  x86_64::Program* x86_64_program() const { return x86_64_program_; }

  x86_64::func_num_t malloc_func_num() const { return malloc_func_num_; }
  x86_64::func_num_t free_func_num() const { return free_func_num_; }

  x86_64::func_num_t x86_64_func_num_for_ir_func_num(ir::func_num_t ir_func_num) const;
  void set_x86_64_func_num_for_ir_func_num(ir::func_num_t ir_func_num,
                                           x86_64::func_num_t x86_64_func_num);

 private:
  const ir::Program* ir_program_;
  x86_64::Program* x86_64_program_;

  x86_64::func_num_t malloc_func_num_;
  x86_64::func_num_t free_func_num_;
  std::unordered_map<ir::func_num_t, x86_64::func_num_t> ir_to_x86_64_func_nums_;
};

class FuncContext {
 public:
  FuncContext(ProgramContext& program_ctx, const ir::Func* ir_func, x86_64::Func* x86_64_func,
              const ir_info::FuncLiveRanges& live_ranges,
              const ir_info::InterferenceGraph& interference_graph,
              const ir_info::InterferenceGraphColors& interference_graph_colors)
      : program_ctx_(program_ctx),
        ir_func_(ir_func),
        x86_64_func_(x86_64_func),
        live_ranges_(live_ranges),
        interference_graph_(interference_graph),
        interference_graph_colors_(interference_graph_colors),
        used_colors_(interference_graph_colors.GetColors(interference_graph.values())) {}

  ProgramContext& program_ctx() const { return program_ctx_; }

  const ir::Func* ir_func() const { return ir_func_; }
  x86_64::Func* x86_64_func() const { return x86_64_func_; }

  const ir_info::FuncLiveRanges& live_ranges() const { return live_ranges_; }
  const ir_info::InterferenceGraph& interference_graph() const { return interference_graph_; }
  const ir_info::InterferenceGraphColors& interference_graph_colors() const {
    return interference_graph_colors_;
  }
  const std::unordered_set<ir_info::color_t>& used_colors() const { return used_colors_; }

  void AddUsedColor(ir_info::color_t color) { used_colors_.insert(color); }

  x86_64::block_num_t x86_64_block_num_for_ir_block_num(ir::block_num_t ir_block_num) const;
  void set_x86_64_block_num_for_ir_block_num(ir::block_num_t ir_block_num,
                                             x86_64::block_num_t x86_64_block_num);

 private:
  ProgramContext& program_ctx_;

  const ir::Func* ir_func_;
  x86_64::Func* x86_64_func_;

  const ir_info::FuncLiveRanges& live_ranges_;
  const ir_info::InterferenceGraph& interference_graph_;
  const ir_info::InterferenceGraphColors& interference_graph_colors_;
  std::unordered_set<ir_info::color_t> used_colors_;

  std::unordered_map<ir::block_num_t, x86_64::block_num_t> ir_to_x86_64_block_nums_;
};

class BlockContext {
 public:
  BlockContext(FuncContext& func_ctx, const ir::Block* ir_block, x86_64::Block* x86_64_block)
      : func_ctx_(func_ctx),
        ir_block_(ir_block),
        x86_64_block_(x86_64_block),
        live_ranges_(func_ctx_.live_ranges().GetBlockLiveRanges(ir_block_->number())) {}

  FuncContext& func_ctx() const { return func_ctx_; }

  const ir::Func* ir_func() const { return func_ctx_.ir_func(); }
  x86_64::Func* x86_64_func() const { return func_ctx_.x86_64_func(); }

  const ir::Block* ir_block() const { return ir_block_; }
  x86_64::Block* x86_64_block() const { return x86_64_block_; }

  const ir_info::BlockLiveRanges& live_ranges() const { return live_ranges_; }

  bool IsTemporaryColorUsedDuringInstr(const ir::Instr* instr,
                                       ir_info::color_t temporary_color) const;
  void AddTemporaryColorUsedDuringInstr(const ir::Instr* instr, ir_info::color_t temporary_color);

 private:
  FuncContext& func_ctx_;

  const ir::Block* ir_block_;
  x86_64::Block* x86_64_block_;

  const ir_info::BlockLiveRanges& live_ranges_;

  std::unordered_map<const ir::Instr*, std::unordered_set<ir_info::color_t>>
      instr_temporary_colors_;
};

}  // namespace ir_to_x86_64_translator

#endif /* ir_to_x86_64_translator_context_h */
