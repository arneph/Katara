//
//  ir_translator.h
//  Katara
//
//  Created by Arne Philipeit on 2/15/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_ir_translator_h
#define x86_64_ir_translator_h

#include <memory>
#include <unordered_map>

#include "ir/info/block_live_range_info.h"
#include "ir/info/func_live_range_info.h"
#include "ir/info/interference_graph.h"
#include "ir/representation/block.h"
#include "ir/representation/func.h"
#include "ir/representation/instr.h"
#include "ir/representation/program.h"
#include "ir/representation/values.h"
#include "x86_64/block.h"
#include "x86_64/func.h"
#include "x86_64/instr.h"
#include "x86_64/instrs/al_instrs.h"
#include "x86_64/instrs/cf_instrs.h"
#include "x86_64/instrs/data_instrs.h"
#include "x86_64/ops.h"
#include "x86_64/prog.h"

namespace x86_64_ir_translator {

class IRTranslator {
 public:
  IRTranslator(ir::Program* program,
               std::unordered_map<ir::Func*, ir_info::FuncLiveRangeInfo>& live_range_infos,
               std::unordered_map<ir::Func*, ir_info::InterferenceGraph>& inteference_graphs);
  ~IRTranslator();

  x86_64::Prog* x86_64_program() const;
  x86_64::Func* x86_64_main_func() const;

  void PrepareInterferenceGraphs();
  void TranslateProgram();

 private:
  void PrepareInterferenceGraph(ir::Func* ir_func);

  x86_64::Func* TranslateFunc(ir::Func* ir_func, x86_64::FuncBuilder x86_64_func_builder);

  x86_64::Block* TranslateBlock(ir::Block* ir_block, ir::Func* ir_func,
                                x86_64::BlockBuilder x86_64_block_builder);

  void TranslateInstr(ir::Instr* ir_instr, ir::Func* ir_func,
                      x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateMovInstr(ir::MovInstr* ir_mov_instr, ir::Func* ir_func,
                         x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateUnaryALInstr(ir::UnaryALInstr* ir_unary_al_instr, ir::Func* ir_func,
                             x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateBinaryALInstr(ir::BinaryALInstr* ir_binary_al_instr, ir::Func* ir_func,
                              x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateCompareInstr(ir::CompareInstr* ir_compare_instr, ir::Func* ir_func,
                             x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateJumpInstr(ir::JumpInstr* ir_jump_instr, x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateJumpCondInstr(ir::JumpCondInstr* ir_jump_cond_instr, ir::Func* ir_func,
                              x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateCallInstr(ir::CallInstr* ir_call_instr, ir::Func* ir_func,
                          x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateReturnInstr(ir::ReturnInstr* ir_return_instr, ir::Func* ir_func,
                            x86_64::BlockBuilder& x86_64_block_builder);

  void GenerateFuncPrologue(ir::Func* ir_func, x86_64::BlockBuilder& x86_64_block_builder);
  void GenerateFuncEpilogue(ir::Func* ir_func, x86_64::BlockBuilder& x86_64_block_builder);

  void GenerateMovs(std::shared_ptr<ir::Computed> ir_result, std::shared_ptr<ir::Value> ir_origin,
                    ir::Func* ir_func, x86_64::BlockBuilder& x86_64_block_builder);

  x86_64::Operand TranslateValue(std::shared_ptr<ir::Value> value, ir::Func* ir_func);
  x86_64::Imm TranslateConstant(std::shared_ptr<ir::Constant> constant);
  x86_64::RM TranslateComputed(std::shared_ptr<ir::Computed> computed, ir::Func* ir_func);
  x86_64::BlockRef TranslateBlockValue(ir::block_num_t block_value);
  x86_64::FuncRef TranslateFuncValue(ir::Value func_value);

  x86_64::InstrCond TranslateCompareOperation(ir::AtomicType* type, ir::CompareOperation op);

  ir::Program* ir_program_;
  // std::unordered_map<ir::Func*, ir_info::FuncLiveRangeInfo>& live_range_infos_;
  std::unordered_map<ir::Func*, ir_info::InterferenceGraph>& interference_graphs_;

  x86_64::ProgBuilder x86_64_program_builder_;
  x86_64::Func* x86_64_main_func_;
};

}  // namespace x86_64_ir_translator

#endif /* x86_64_ir_translator_h */
