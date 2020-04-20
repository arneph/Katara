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

#include "ir/prog.h"
#include "ir/func.h"
#include "ir/block.h"
#include "ir/instr.h"
#include "ir/value.h"

#include "ir_info/func_live_range_info.h"
#include "ir_info/block_live_range_info.h"
#include "ir_info/interference_graph.h"

#include "x86_64/prog.h"
#include "x86_64/func.h"
#include "x86_64/block.h"
#include "x86_64/instr.h"
#include "x86_64/instrs/al_instrs.h"
#include "x86_64/instrs/cf_instrs.h"
#include "x86_64/instrs/data_instrs.h"
#include "x86_64/ops.h"

namespace x86_64_ir_translator {

class IRTranslator {
public:
    IRTranslator(ir::Prog *program,
                 std::unordered_map<ir::Func *,
                                    ir_info::FuncLiveRangeInfo>&
                    live_range_infos,
                 std::unordered_map<ir::Func *,
                                    ir_info::InterferenceGraph>&
                    inteference_graphs);
    ~IRTranslator();
    
    x86_64::Prog * x86_64_program() const;
    x86_64::Func * x86_64_main_func() const;
    
    void PrepareInterferenceGraphs();
    void TranslateProgram();
    
private:
    void PrepareInterferenceGraph(ir::Func *ir_func);
    
    x86_64::Func *
    TranslateFunc(ir::Func *ir_func,
                  x86_64::FuncBuilder x86_64_func_builder);
    
    x86_64::Block *
    TranslateBlock(ir::Block *ir_block,
                   x86_64::BlockBuilder x86_64_block_builder);
    
    void TranslateInstr(
        ir::Instr *ir_instr,
        ir::Block *ir_block,
        x86_64::BlockBuilder &x86_64_block_builder);
    void TranslateMovInstr(
        ir::MovInstr *ir_mov_instr,
        ir::Block *ir_block,
        x86_64::BlockBuilder &x86_64_block_builder);
    void TranslateUnaryALInstr(
        ir::UnaryALInstr *ir_unary_al_instr,
        ir::Block *ir_block,
        x86_64::BlockBuilder &x86_64_block_builder);
    void TranslateBinaryALInstr(
        ir::BinaryALInstr *ir_binary_al_instr,
        ir::Block *ir_block,
        x86_64::BlockBuilder &x86_64_block_builder);
    void TranslateCompareInstr(
        ir::CompareInstr *ir_compare_instr,
        ir::Block *ir_block,
        x86_64::BlockBuilder &x86_64_block_builder);
    void TranslateJumpInstr(
        ir::JumpInstr *ir_jump_instr,
        ir::Block *ir_block,
        x86_64::BlockBuilder &x86_64_block_builder);
    void TranslateJumpCondInstr(
        ir::JumpCondInstr *ir_jump_cond_instr,
        ir::Block *ir_block,
        x86_64::BlockBuilder &x86_64_block_builder);
    void TranslateCallInstr(
        ir::CallInstr *ir_call_instr,
        ir::Block *ir_block,
        x86_64::BlockBuilder &x86_64_block_builder);
    void TranslateReturnInstr(
        ir::ReturnInstr *ir_return_instr,
        ir::Block *ir_block,
        x86_64::BlockBuilder &x86_64_block_builder);
    
    void GenerateFuncPrologue(
        ir::Func *ir_func,
        x86_64::BlockBuilder &x86_64_block_builder);
    void GenerateFuncEpilogue(
        ir::Func *ir_func,
        x86_64::BlockBuilder &x86_64_block_builder);
    
    void GenerateMovs(ir::Computed ir_result,
                      ir::Value ir_origin,
                      ir::Block *ir_block,
                      x86_64::BlockBuilder &x86_64_block_builder);
    
    x86_64::Operand TranslateValue(ir::Value value,
                                   ir::Func *ir_func);
    x86_64::Imm TranslateConstant(ir::Constant constant);
    x86_64::RM TranslateComputed(ir::Computed computed,
                                 ir::Func *ir_func);
    x86_64::BlockRef TranslateBlockValue(ir::BlockValue block_value);
    x86_64::FuncRef TranslateFuncValue(ir::Value func_value);
    
    x86_64::InstrCond TranslateCompareOperation(ir::Type type,
                                                ir::CompareOperation op);
    
    ir::Prog *ir_program_;
    std::unordered_map<ir::Func *,
                       ir_info::FuncLiveRangeInfo>&
        live_range_infos_;
    std::unordered_map<ir::Func *,
                       ir_info::InterferenceGraph>& interference_graphs_;
    
    x86_64::ProgBuilder x86_64_program_builder_;
    x86_64::Func *x86_64_main_func_;
};

}

#endif /* x86_64_ir_translator_h */
