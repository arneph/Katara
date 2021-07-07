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

#include "src/ir/info/block_live_range_info.h"
#include "src/ir/info/func_live_range_info.h"
#include "src/ir/info/interference_graph.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/values.h"
#include "src/x86_64/block.h"
#include "src/x86_64/func.h"
#include "src/x86_64/instrs/al_instrs.h"
#include "src/x86_64/instrs/cf_instrs.h"
#include "src/x86_64/instrs/data_instrs.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/ops.h"
#include "src/x86_64/program.h"

namespace x86_64_ir_translator {

class IRTranslator {
 public:
  static std::unique_ptr<x86_64::Program> Translate(
      ir::Program* program,
      std::unordered_map<ir::Func*, ir_info::InterferenceGraph>& inteference_graphs);

 private:
  IRTranslator(ir::Program* program,
               std::unordered_map<ir::Func*, ir_info::InterferenceGraph>& inteference_graphs)
      : ir_program_(program), interference_graphs_(inteference_graphs) {}

  void PrepareInterferenceGraphs();
  void PrepareInterferenceGraph(ir::Func* ir_func);

  void TranslateProgram();

  x86_64::Func* TranslateFunc(ir::Func* ir_func, x86_64::FuncBuilder x86_64_func_builder);

  x86_64::Block* TranslateBlock(ir::Block* ir_block, ir::Func* ir_func,
                                x86_64::BlockBuilder x86_64_block_builder);

  void TranslateInstr(ir::Instr* ir_instr, ir::Func* ir_func,
                      x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateMovInstr(ir::MovInstr* ir_mov_instr, ir::Func* ir_func,
                         x86_64::BlockBuilder& x86_64_block_builder);

  void TranslateBoolNotInstr(ir::BoolNotInstr* ir_bool_not_instr, ir::Func* ir_func,
                             x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateBoolBinaryInstr(ir::BoolBinaryInstr* ir_bool_binary_instr, ir::Func* ir_func,
                                x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateBoolCompareInstr(ir::BoolBinaryInstr* ir_bool_compare_instr, ir::Func* ir_func,
                                 x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateBoolLogicInstr(ir::BoolBinaryInstr* ir_bool_logic_instr, ir::Func* ir_func,
                               x86_64::BlockBuilder& x86_64_block_builder);

  void TranslateIntUnaryInstr(ir::IntUnaryInstr* ir_int_unary_instr, ir::Func* ir_func,
                              x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateIntCompareInstr(ir::IntCompareInstr* ir_int_compare_instr, ir::Func* ir_func,
                                x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateIntBinaryInstr(ir::IntBinaryInstr* ir_int_binary_instr, ir::Func* ir_func,
                               x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateIntSimpleALInstr(ir::IntBinaryInstr* ir_int_binary_instr, ir::Func* ir_func,
                                 x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateIntMulInstr(ir::IntBinaryInstr* ir_int_binary_instr, ir::Func* ir_func,
                            x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateIntDivOrRemInstr(ir::IntBinaryInstr* ir_int_binary_instr, ir::Func* ir_func,
                                 x86_64::BlockBuilder& x86_64_block_builder);
  void TranslateIntShiftInstr(ir::IntShiftInstr* ir_int_shift_instr, ir::Func* ir_func,
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

  void GenerateMovs(ir::Computed* ir_result, ir::Value* ir_origin, ir::Func* ir_func,
                    x86_64::BlockBuilder& x86_64_block_builder);

  x86_64::Operand TranslateValue(ir::Value* value, ir::Func* ir_func);
  x86_64::Imm TranslateBoolConstant(ir::BoolConstant* constant);
  x86_64::Imm TranslateIntConstant(ir::IntConstant* constant);
  x86_64::Imm TranslatePointerConstant(ir::PointerConstant* constant);
  x86_64::Imm TranslateFuncConstant(ir::FuncConstant* constant);
  x86_64::RM TranslateComputed(ir::Computed* computed, ir::Func* ir_func);
  x86_64::BlockRef TranslateBlockValue(ir::block_num_t block_value);
  x86_64::FuncRef TranslateFuncValue(ir::Value func_value);

  ir::Program* ir_program_;
  std::unordered_map<ir::Func*, ir_info::InterferenceGraph>& interference_graphs_;

  x86_64::ProgramBuilder x86_64_program_builder_;
  x86_64::Func* x86_64_main_func_;
};

}  // namespace x86_64_ir_translator

#endif /* x86_64_ir_translator_h */
