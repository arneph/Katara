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

#include "src/ir/info/block_live_ranges.h"
#include "src/ir/info/func_live_ranges.h"
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
#include "src/x86_64/ir_translator/register_allocator.h"
#include "src/x86_64/ops.h"
#include "src/x86_64/program.h"

namespace x86_64_ir_translator {

class IRTranslator {
 public:
  static std::unique_ptr<x86_64::Program> Translate(
      ir::Program* program,
      std::unordered_map<ir::func_num_t, ir_info::FuncLiveRanges>& live_ranges,
      std::unordered_map<ir::func_num_t, ir_info::InterferenceGraph>& inteference_graphs);

 private:
  IRTranslator(ir::Program* program,
               std::unordered_map<ir::func_num_t, ir_info::FuncLiveRanges>& live_ranges,
               std::unordered_map<ir::func_num_t, ir_info::InterferenceGraph>& inteference_graphs)
      : ir_program_(program),
        func_live_ranges_(live_ranges),
        interference_graphs_(inteference_graphs) {}

  void AllocateRegisters();
  void TranslateProgram();

  void TranslateFunc(ir::Func* ir_func, x86_64::Func* x86_64_func);

  void TranslateBlock(ir::Block* ir_block, ir::Func* ir_func, x86_64::Block* x86_64_block);

  void TranslateInstr(ir::Instr* ir_instr, ir::Func* ir_func, x86_64::Block* x86_64_block);
  void TranslateMovInstr(ir::MovInstr* ir_mov_instr, ir::Func* ir_func,
                         x86_64::Block* x86_64_block);

  void TranslateBoolNotInstr(ir::BoolNotInstr* ir_bool_not_instr, ir::Func* ir_func,
                             x86_64::Block* x86_64_block);
  void TranslateBoolBinaryInstr(ir::BoolBinaryInstr* ir_bool_binary_instr, ir::Func* ir_func,
                                x86_64::Block* x86_64_block);
  void TranslateBoolCompareInstr(ir::BoolBinaryInstr* ir_bool_compare_instr, ir::Func* ir_func,
                                 x86_64::Block* x86_64_block);
  void TranslateBoolLogicInstr(ir::BoolBinaryInstr* ir_bool_logic_instr, ir::Func* ir_func,
                               x86_64::Block* x86_64_block);

  void TranslateIntUnaryInstr(ir::IntUnaryInstr* ir_int_unary_instr, ir::Func* ir_func,
                              x86_64::Block* x86_64_block);
  void TranslateIntCompareInstr(ir::IntCompareInstr* ir_int_compare_instr, ir::Func* ir_func,
                                x86_64::Block* x86_64_block);
  void TranslateIntBinaryInstr(ir::IntBinaryInstr* ir_int_binary_instr, ir::Func* ir_func,
                               x86_64::Block* x86_64_block);
  void TranslateIntSimpleALInstr(ir::IntBinaryInstr* ir_int_binary_instr, ir::Func* ir_func,
                                 x86_64::Block* x86_64_block);
  void TranslateIntMulInstr(ir::IntBinaryInstr* ir_int_binary_instr, ir::Func* ir_func,
                            x86_64::Block* x86_64_block);
  void TranslateIntDivOrRemInstr(ir::IntBinaryInstr* ir_int_binary_instr, ir::Func* ir_func,
                                 x86_64::Block* x86_64_block);
  void TranslateIntShiftInstr(ir::IntShiftInstr* ir_int_shift_instr, ir::Func* ir_func,
                              x86_64::Block* x86_64_block);

  void TranslatePointerOffsetInstr(ir::PointerOffsetInstr* ir_pointer_offset_instr, ir::Func* func,
                                   x86_64::Block* x86_64_block);
  void TranslateNilTestInstr(ir::NilTestInstr* ir_nil_test_instr, ir::Func* ir_func,
                             x86_64::Block* x86_64_block);

  void TranslateMallocInstr(ir::MallocInstr* ir_malloc_instr, ir::Func* ir_func,
                            x86_64::Block* x86_64_block);
  void TranslateLoadInstr(ir::LoadInstr* ir_load_instr, ir::Func* ir_func,
                          x86_64::Block* x86_64_block);
  void TranslateStoreInstr(ir::StoreInstr* ir_store_instr, ir::Func* ir_func,
                           x86_64::Block* x86_64_block);
  void TranslateFreeInstr(ir::FreeInstr* ir_free_instr, ir::Func* ir_func,
                          x86_64::Block* x86_64_block);

  void TranslateJumpInstr(ir::JumpInstr* ir_jump_instr, x86_64::Block* x86_64_block);
  void TranslateJumpCondInstr(ir::JumpCondInstr* ir_jump_cond_instr, ir::Func* ir_func,
                              x86_64::Block* x86_64_block);

  void TranslateCallInstr(ir::CallInstr* ir_call_instr, ir::Func* ir_func,
                          x86_64::Block* x86_64_block);
  void TranslateReturnInstr(ir::ReturnInstr* ir_return_instr, ir::Func* ir_func,
                            x86_64::Block* x86_64_block);

  void GenerateFuncPrologue(ir::Func* ir_func, x86_64::Block* x86_64_block);
  void GenerateFuncEpilogue(ir::Func* ir_func, x86_64::Block* x86_64_block);

  void GenerateMovs(ir::Computed* ir_result, ir::Value* ir_origin, ir::Func* ir_func,
                    x86_64::Block* x86_64_block);

  x86_64::Operand TranslateValue(ir::Value* value, ir::Func* ir_func);
  x86_64::Imm TranslateBoolConstant(ir::BoolConstant* constant);
  x86_64::Imm TranslateIntConstant(ir::IntConstant* constant);
  x86_64::Imm TranslatePointerConstant(ir::PointerConstant* constant);
  x86_64::Operand TranslateFuncConstant(ir::FuncConstant* constant);
  x86_64::RM TranslateComputed(ir::Computed* computed, ir::Func* ir_func);
  x86_64::BlockRef TranslateBlockValue(ir::block_num_t block_value);

  x86_64::Size TranslateSizeOfType(const ir::Type* ir_type);
  x86_64::Size TranslateSizeOfIntType(const ir::IntType* ir_int_type);
  x86_64::Size TranslateSizeOfIntType(common::IntType common_int_type);

  ir::Program* ir_program_;
  std::unordered_map<ir::func_num_t, ir_info::FuncLiveRanges>& func_live_ranges_;
  std::unordered_map<ir::func_num_t, ir_info::InterferenceGraph>& interference_graphs_;
  std::unordered_map<ir::func_num_t, ir_info::InterferenceGraphColors> interference_graph_colors_;

  std::unique_ptr<x86_64::Program> x86_64_program_;
  x86_64::Func* x86_64_main_func_;
};

}  // namespace x86_64_ir_translator

#endif /* x86_64_ir_translator_h */
