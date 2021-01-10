//
//  ir_translator.cc
//  Katara
//
//  Created by Arne Philipeit on 2/15/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "ir_translator.h"

namespace x86_64_ir_translator {

IRTranslator::IRTranslator(
    ir::Prog* program, std::unordered_map<ir::Func*, ir_info::FuncLiveRangeInfo>& live_range_infos,
    std::unordered_map<ir::Func*, ir_info::InterferenceGraph>& inteference_graphs)
    : ir_program_(program),
      live_range_infos_(live_range_infos),
      interference_graphs_(inteference_graphs) {}

IRTranslator::~IRTranslator() {}

x86_64::Prog* IRTranslator::x86_64_program() const { return x86_64_program_builder_.prog(); }

x86_64::Func* IRTranslator::x86_64_main_func() const { return x86_64_main_func_; }

void IRTranslator::PrepareInterferenceGraphs() {
  for (ir::Func* ir_func : ir_program_->funcs()) {
    PrepareInterferenceGraph(ir_func);
  }
}

void IRTranslator::PrepareInterferenceGraph(ir::Func* ir_func) {
  ir_info::InterferenceGraph& interference_graph = interference_graphs_.at(ir_func);

  for (size_t i = 0; i < ir_func->args().size(); i++) {
    ir::Computed arg = ir_func->args().at(i);
    int64_t arg_reg;
    switch (i) {
      case 0:
        arg_reg = 7 - 2 /* rdi */;
        break;
      case 1:
        arg_reg = 6 - 2 /* rsi */;
        break;
      case 2:
        arg_reg = 2 /* rdx */;
        break;
      case 3:
        arg_reg = 1 /* rcx */;
        break;
      case 4:
        arg_reg = 8 - 2 /* r8 */;
        break;
      case 5:
        arg_reg = 9 - 2 /* r9 */;
        break;
      default:
        // TODO: use stack for additional args.
        throw "can not handle functions with more than six arguments";
    }

    interference_graph.SetRegister(arg, arg_reg);
  }

  for (ir::Block* ir_block : ir_func->blocks()) {
    for (ir::Instr* ir_instr : ir_block->instrs()) {
      ir::ReturnInstr* ir_return_instr = dynamic_cast<ir::ReturnInstr*>(ir_instr);
      if (ir_return_instr == nullptr) {
        continue;
      }

      for (size_t i = 0; i < ir_return_instr->args().size(); i++) {
        if (i > 0) {
          throw "can not handle functions with more than one return value";
        }
        ir::Value result = ir_return_instr->args().at(i);
        if (!result.is_computed()) {
          continue;
        }

        ir::Computed computed_result = result.computed();

        interference_graph.SetRegister(computed_result, 0);
      }
    }
  }
}

void IRTranslator::TranslateProgram() {
  for (ir::Func* ir_func : ir_program_->funcs()) {
    std::string ir_func_name = ir_func->name();
    if (ir_func_name.empty()) {
      ir_func_name = "Func" + std::to_string(ir_func->number());
    }

    x86_64::Func* x86_64_func =
        TranslateFunc(ir_func, x86_64_program_builder_.AddFunc(ir_func_name));

    if (ir_func == ir_program_->entry_func()) {
      x86_64_main_func_ = x86_64_func;
    }
  }
}

x86_64::Func* IRTranslator::TranslateFunc(ir::Func* ir_func,
                                          x86_64::FuncBuilder x86_64_func_builder) {
  std::vector<ir::Block*> ir_blocks(ir_func->blocks().begin(), ir_func->blocks().end());
  std::sort(ir_blocks.begin(), ir_blocks.end(), [=](ir::Block* lhs, ir::Block* rhs) -> bool {
    // entry block always first:
    if (lhs == ir_func->entry_block()) {
      return true;
    } else if (rhs == ir_func->entry_block()) {
      return false;
    }
    // otherwise sort by block number:
    return lhs->number() < rhs->number();
  });

  for (ir::Block* ir_block : ir_blocks) {
    x86_64::BlockBuilder x86_64_block_builder = x86_64_func_builder.AddBlock();

    if (ir_func->entry_block() == ir_block) {
      GenerateFuncPrologue(ir_func, x86_64_block_builder);
    }

    TranslateBlock(ir_block, x86_64_block_builder);
  }

  return x86_64_func_builder.func();
}

x86_64::Block* IRTranslator::TranslateBlock(ir::Block* ir_block,
                                            x86_64::BlockBuilder x86_64_block_builder) {
  for (ir::Instr* ir_instr : ir_block->instrs()) {
    TranslateInstr(ir_instr, ir_block, x86_64_block_builder);
  }

  return x86_64_block_builder.block();
}

void IRTranslator::TranslateInstr(ir::Instr* ir_instr, ir::Block* ir_block,
                                  x86_64::BlockBuilder& x86_64_block_builder) {
  if (ir::MovInstr* ir_mov_instr = dynamic_cast<ir::MovInstr*>(ir_instr)) {
    TranslateMovInstr(ir_mov_instr, ir_block, x86_64_block_builder);

  } else if (ir::UnaryALInstr* ir_unary_al_instr = dynamic_cast<ir::UnaryALInstr*>(ir_instr)) {
    TranslateUnaryALInstr(ir_unary_al_instr, ir_block, x86_64_block_builder);

  } else if (ir::BinaryALInstr* ir_binary_al_instr = dynamic_cast<ir::BinaryALInstr*>(ir_instr)) {
    TranslateBinaryALInstr(ir_binary_al_instr, ir_block, x86_64_block_builder);

  } else if (ir::CompareInstr* ir_compare_instr = dynamic_cast<ir::CompareInstr*>(ir_instr)) {
    TranslateCompareInstr(ir_compare_instr, ir_block, x86_64_block_builder);

  } else if (ir::JumpInstr* ir_jump_instr = dynamic_cast<ir::JumpInstr*>(ir_instr)) {
    TranslateJumpInstr(ir_jump_instr, ir_block, x86_64_block_builder);

  } else if (ir::JumpCondInstr* ir_jump_cond_instr = dynamic_cast<ir::JumpCondInstr*>(ir_instr)) {
    TranslateJumpCondInstr(ir_jump_cond_instr, ir_block, x86_64_block_builder);

  } else if (ir::CallInstr* ir_call_instr = dynamic_cast<ir::CallInstr*>(ir_instr)) {
    TranslateCallInstr(ir_call_instr, ir_block, x86_64_block_builder);

  } else if (ir::ReturnInstr* ir_return_instr = dynamic_cast<ir::ReturnInstr*>(ir_instr)) {
    TranslateReturnInstr(ir_return_instr, ir_block, x86_64_block_builder);

  } else {
    throw "unexpected ir::Instr";
  }
}

void IRTranslator::TranslateMovInstr(ir::MovInstr* ir_mov_instr, ir::Block* ir_block,
                                     x86_64::BlockBuilder& x86_64_block_builder) {
  ir::Computed ir_result = ir_mov_instr->result();
  ir::Value ir_origin = ir_mov_instr->origin();

  GenerateMovs(ir_result, ir_origin, ir_block, x86_64_block_builder);
}

void IRTranslator::TranslateUnaryALInstr(ir::UnaryALInstr* ir_unary_al_instr, ir::Block* ir_block,
                                         x86_64::BlockBuilder& x86_64_block_builder) {
  ir::Func* ir_func = ir_block->func();

  ir::Computed ir_result = ir_unary_al_instr->result();
  ir::Value ir_operand = ir_unary_al_instr->operand();

  GenerateMovs(ir_result, ir_operand, ir_block, x86_64_block_builder);

  x86_64::RM x86_64_operand = TranslateComputed(ir_result, ir_func);

  switch (ir_unary_al_instr->operation()) {
    case ir::UnaryALOperation::kNot:
      x86_64_block_builder.AddInstr(new x86_64::Not(x86_64_operand));
      break;
    case ir::UnaryALOperation::kNeg:
      x86_64_block_builder.AddInstr(new x86_64::Neg(x86_64_operand));
      break;
    default:
      throw "unexpected UnaryALOperation";
  }
}

void IRTranslator::TranslateBinaryALInstr(ir::BinaryALInstr* ir_binary_al_instr,
                                          ir::Block* ir_block,
                                          x86_64::BlockBuilder& x86_64_block_builder) {
  ir::Func* ir_func = ir_block->func();

  ir::Computed ir_result = ir_binary_al_instr->result();
  ir::Value ir_operand_a = ir_binary_al_instr->operand_a();
  ir::Value ir_operand_b = ir_binary_al_instr->operand_b();

  GenerateMovs(ir_result, ir_operand_a, ir_block, x86_64_block_builder);

  bool requires_tmp_reg = false;

  if (ir_operand_b.is_constant() && ir::size(ir_operand_b.type()) == 64) {
    int64_t v = ir_operand_b.constant().value();

    if (ir_operand_b.type() == ir::Type::kI64) {
      if (INT32_MIN <= v && v <= INT32_MAX) {
        ir_operand_b = ir::Constant(int32_t(v));
      } else {
        requires_tmp_reg = true;
      }
    } else if (ir_operand_b.type() == ir::Type::kU64) {
      if (0 <= v && v <= UINT32_MAX) {
        ir_operand_b = ir::Constant(uint32_t(v));
      } else {
        requires_tmp_reg = true;
      }
    } else {
      throw "unexpected operand type";
    }
  }

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_result, ir_func);
  x86_64::Operand x86_64_operand_b = TranslateValue(ir_operand_b, ir_func);

  if (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem()) {
    requires_tmp_reg = true;
  }

  ir::Type ir_operand_b_type = ir_operand_b.type();
  x86_64::Size x86_64_size = x86_64::Size(ir::size(ir_operand_b_type));
  x86_64::Reg x86_64_tmp_reg(x86_64_size, 0);
  if (x86_64_operand_a.is_reg() && x86_64_operand_a.reg().reg() == 0) {
    x86_64_tmp_reg = x86_64::Reg(x86_64_size, 1);
  }

  if (requires_tmp_reg) {
    x86_64_block_builder.AddInstr(new x86_64::Push(x86_64_tmp_reg));
    x86_64_block_builder.AddInstr(new x86_64::Mov(x86_64_tmp_reg, x86_64_operand_b));
    x86_64_operand_b = x86_64_tmp_reg;
  }

  switch (ir_binary_al_instr->operation()) {
    case ir::BinaryALOperation::kAnd:
      x86_64_block_builder.AddInstr(new x86_64::And(x86_64_operand_a, x86_64_operand_b));
      break;

    case ir::BinaryALOperation::kOr:
      x86_64_block_builder.AddInstr(new x86_64::Or(x86_64_operand_a, x86_64_operand_b));
      break;

    case ir::BinaryALOperation::kXor:
      x86_64_block_builder.AddInstr(new x86_64::Xor(x86_64_operand_a, x86_64_operand_b));
      break;

    case ir::BinaryALOperation::kAdd:
      x86_64_block_builder.AddInstr(new x86_64::Add(x86_64_operand_a, x86_64_operand_b));
      break;

    case ir::BinaryALOperation::kSub:
      x86_64_block_builder.AddInstr(new x86_64::Sub(x86_64_operand_a, x86_64_operand_b));
      break;

    default:
      // TODO: implement mul, div, rem
      throw "unexpexted BinaryALOperartion";
  }

  if (requires_tmp_reg) {
    x86_64_block_builder.AddInstr(new x86_64::Pop(x86_64_tmp_reg));
  }
}

void IRTranslator::TranslateCompareInstr(ir::CompareInstr* ir_compare_instr, ir::Block* ir_block,
                                         x86_64::BlockBuilder& x86_64_block_builder) {
  ir::Func* ir_func = ir_block->func();

  ir::CompareOperation ir_op = ir_compare_instr->operation();
  ir::Computed ir_result = ir_compare_instr->result();
  ir::Value ir_operand_a = ir_compare_instr->operand_a();
  ir::Value ir_operand_b = ir_compare_instr->operand_b();

  if (ir_operand_a.is_constant() && !ir_operand_b.is_constant()) {
    ir_op = ir::comuted(ir_op);
    std::swap(ir_operand_a, ir_operand_b);
  }

  x86_64::RM x86_64_result = TranslateComputed(ir_result, ir_func);
  x86_64::RM x86_64_operand_a = x86_64_result;
  x86_64::Operand x86_64_operand_b = TranslateValue(ir_operand_b, ir_func);

  if (ir_operand_a.is_constant()) {
    GenerateMovs(ir_result, ir_operand_a, ir_block, x86_64_block_builder);
  } else if (ir_operand_a.is_computed()) {
    x86_64_operand_a = TranslateComputed(ir_operand_a.computed(), ir_func);
  } else {
    throw "unexpected operand_a kind";
  }

  bool requires_tmp_reg = false;
  if (ir_operand_b.is_constant()) {
    requires_tmp_reg = true;

  } else if (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem()) {
    requires_tmp_reg = true;
  }

  bool required_tmp_reg_is_result_reg = false;
  if (!x86_64_operand_a.is_reg() || x86_64_operand_a.reg().reg() != x86_64_result.reg().reg()) {
    required_tmp_reg_is_result_reg = x86_64_result.is_reg();
  }

  ir::Type ir_type = ir_operand_b.type();
  x86_64::Size x86_64_size = x86_64::Size(ir::size(ir_type));
  x86_64::Reg x86_64_tmp_reg(x86_64_size, 0);
  if (!required_tmp_reg_is_result_reg && x86_64_operand_a.is_reg() &&
      x86_64_operand_a.reg().reg() == 0) {
    x86_64_tmp_reg = x86_64::Reg(x86_64_size, 1);
  } else if (required_tmp_reg_is_result_reg) {
    x86_64_tmp_reg = x86_64::Reg(x86_64_size, x86_64_result.reg().reg());
  }

  x86_64::InstrCond x86_64_cond = TranslateCompareOperation(ir_operand_a.type(), ir_op);

  if (requires_tmp_reg) {
    if (!required_tmp_reg_is_result_reg) {
      x86_64_block_builder.AddInstr(new x86_64::Push(x86_64_tmp_reg));
    }
    x86_64_block_builder.AddInstr(new x86_64::Mov(x86_64_tmp_reg, x86_64_operand_b));
    x86_64_operand_b = x86_64_tmp_reg;
  }

  x86_64_block_builder.AddInstr(new x86_64::Cmp(x86_64_operand_a, x86_64_operand_b));
  x86_64_block_builder.AddInstr(new x86_64::Setcc(x86_64_cond, x86_64_result));

  if (requires_tmp_reg && !required_tmp_reg_is_result_reg) {
    x86_64_block_builder.AddInstr(new x86_64::Pop(x86_64_tmp_reg));
  }
}

void IRTranslator::TranslateJumpInstr(ir::JumpInstr* ir_jump_instr, ir::Block* ir_block,
                                      x86_64::BlockBuilder& x86_64_block_builder) {
  ir::BlockValue ir_destination = ir_jump_instr->destination();

  x86_64::BlockRef x86_64_destination = TranslateBlockValue(ir_destination);

  x86_64_block_builder.AddInstr(new x86_64::Jmp(x86_64_destination));
}

void IRTranslator::TranslateJumpCondInstr(ir::JumpCondInstr* ir_jump_cond_instr,
                                          ir::Block* ir_block,
                                          x86_64::BlockBuilder& x86_64_block_builder) {
  ir::Func* ir_func = ir_block->func();

  ir::Value ir_condition = ir_jump_cond_instr->condition();
  ir::BlockValue ir_destination_true = ir_jump_cond_instr->destination_true();
  ir::BlockValue ir_destination_false = ir_jump_cond_instr->destination_false();

  x86_64::BlockRef x86_64_destination_true = TranslateBlockValue(ir_destination_true);
  x86_64::BlockRef x86_64_destination_false = TranslateBlockValue(ir_destination_false);

  if (ir_condition.is_constant()) {
    if (ir_condition.constant().value()) {
      x86_64_block_builder.AddInstr(new x86_64::Jmp(x86_64_destination_true));
    } else {
      x86_64_block_builder.AddInstr(new x86_64::Jmp(x86_64_destination_false));
    }
    return;
  } else if (!ir_condition.is_computed()) {
    throw "unexpected condition value kind";
  }

  x86_64::RM x86_64_condition = TranslateComputed(ir_condition.computed(), ir_func);

  x86_64_block_builder.AddInstr(new x86_64::Test(x86_64_condition, x86_64::Imm(int8_t{-1})));
  x86_64_block_builder.AddInstr(
      new x86_64::Jcc(x86_64::InstrCond::kNoZero, x86_64_destination_true));
  x86_64_block_builder.AddInstr(new x86_64::Jmp(x86_64_destination_false));
}

void IRTranslator::TranslateCallInstr(ir::CallInstr* ir_call_instr, ir::Block* ir_block,
                                      x86_64::BlockBuilder& x86_64_block_builder) {
  // TODO: implement
}

void IRTranslator::TranslateReturnInstr(ir::ReturnInstr* ir_return_instr, ir::Block* ir_block,
                                        x86_64::BlockBuilder& x86_64_block_builder) {
  GenerateFuncEpilogue(ir_block->func(), x86_64_block_builder);
}

void IRTranslator::GenerateFuncPrologue(ir::Func* ir_func,
                                        x86_64::BlockBuilder& x86_64_block_builder) {
  x86_64_block_builder.AddInstr(new x86_64::Push(x86_64::rbp));
  x86_64_block_builder.AddInstr(new x86_64::Mov(x86_64::rbp, x86_64::rsp));
  // TODO: reserve stack space
}

void IRTranslator::GenerateFuncEpilogue(ir::Func* ir_func,
                                        x86_64::BlockBuilder& x86_64_block_builder) {
  // TODO: revert stack pointer
  x86_64_block_builder.AddInstr(new x86_64::Pop(x86_64::rbp));
  x86_64_block_builder.AddInstr(new x86_64::Ret());
}

void IRTranslator::GenerateMovs(ir::Computed ir_result, ir::Value ir_origin, ir::Block* ir_block,
                                x86_64::BlockBuilder& x86_64_block_builder) {
  ir::Func* ir_func = ir_block->func();
  x86_64::RM x86_64_result = TranslateComputed(ir_result, ir_func);
  x86_64::Operand x86_64_origin = TranslateValue(ir_origin, ir_func);

  if (x86_64_result == x86_64_origin) {
    return;
  }

  if (!x86_64_result.is_mem() || !x86_64_origin.is_mem()) {
    x86_64_block_builder.AddInstr(new x86_64::Mov(x86_64_result, x86_64_origin));

  } else {
    ir::Type ir_type = ir_result.type();
    x86_64::Size x86_64_size = (x86_64::Size)ir::size(ir_type);
    x86_64::Reg x86_64_tmp_reg = x86_64::Reg(x86_64_size, 0);

    x86_64_block_builder.AddInstr(new x86_64::Push(x86_64_tmp_reg));
    x86_64_block_builder.AddInstr(new x86_64::Mov(x86_64_tmp_reg, x86_64_origin));
    x86_64_block_builder.AddInstr(new x86_64::Mov(x86_64_result, x86_64_tmp_reg));
    x86_64_block_builder.AddInstr(new x86_64::Pop(x86_64_tmp_reg));
  }
}

x86_64::Operand IRTranslator::TranslateValue(ir::Value value, ir::Func* ir_func) {
  switch (value.kind()) {
    case ir::Value::Kind::kConstant:
      return TranslateConstant(value.constant());
    case ir::Value::Kind::kComputed:
      return TranslateComputed(value.computed(), ir_func);
    case ir::Value::Kind::kBlockValue:
      return TranslateBlockValue(value.block_value());
  }
}

x86_64::Imm IRTranslator::TranslateConstant(ir::Constant constant) {
  switch (constant.type()) {
    case ir::Type::kBool:
      if (constant.value()) {
        return x86_64::Imm(int8_t{1});
      } else {
        return x86_64::Imm(int8_t{0});
      }

    case ir::Type::kI8:
    case ir::Type::kU8:
      return x86_64::Imm(int8_t(constant.value()));

    case ir::Type::kI16:
    case ir::Type::kU16:
      return x86_64::Imm(int16_t(constant.value()));

    case ir::Type::kI32:
    case ir::Type::kU32:
      return x86_64::Imm(int32_t(constant.value()));

    case ir::Type::kI64:
    case ir::Type::kU64:
      return x86_64::Imm(int64_t(constant.value()));

    default:
      // Note: ir::Type::kFunc handled separately in
      // TranslateFuncValue().
      throw "unexpected constant type";
  }
}

x86_64::RM IRTranslator::TranslateComputed(ir::Computed computed, ir::Func* ir_func) {
  ir_info::InterferenceGraph& graph = interference_graphs_.at(ir_func);

  int64_t ir_reg = graph.GetRegister(computed);
  int8_t ir_size = ir::size(computed.type());

  x86_64::Size x86_64_size = (x86_64::Size)ir_size;

  if (0 <= ir_reg && ir_reg <= 3) {
    return x86_64::Reg(x86_64_size, ir_reg);
  } else if (4 <= ir_reg && ir_reg <= 13) {
    return x86_64::Reg(x86_64_size, ir_reg + 2);
  } else {
    return x86_64::Mem(x86_64_size,
                       /* base_reg= */ 5 /* (base pointer) */,
                       /* disp= */ int32_t(-8 * (ir_reg - 14)));
  }
}

x86_64::BlockRef IRTranslator::TranslateBlockValue(ir::BlockValue block_value) {
  return x86_64::BlockRef(block_value.block());
}

x86_64::FuncRef IRTranslator::TranslateFuncValue(ir::Value func_value) {
  // TODO: keep track of func nums from ir to x86_64
  return x86_64::FuncRef(-1);
}

x86_64::InstrCond IRTranslator::TranslateCompareOperation(ir::Type type, ir::CompareOperation op) {
  switch (op) {
    case ir::CompareOperation::kEqual:
      return x86_64::InstrCond::kEqual;

    case ir::CompareOperation::kNotEqual:
      return x86_64::InstrCond::kNotEqual;

    case ir::CompareOperation::kGreater:
      if (ir::is_unsigned(type)) {
        return x86_64::InstrCond::kAbove;
      } else {
        return x86_64::InstrCond::kGreater;
      }

    case ir::CompareOperation::kGreaterOrEqual:
      if (ir::is_unsigned(type)) {
        return x86_64::InstrCond::kAboveOrEqual;
      } else {
        return x86_64::InstrCond::kGreaterOrEqual;
      }

    case ir::CompareOperation::kLessOrEqual:
      if (ir::is_unsigned(type)) {
        return x86_64::InstrCond::kBelowOrEqual;
      } else {
        return x86_64::InstrCond::kLessOrEqual;
      }

    case ir::CompareOperation::kLess:
      if (ir::is_unsigned(type)) {
        return x86_64::InstrCond::kBelow;
      } else {
        return x86_64::InstrCond::kLess;
      }
  }
}

}  // namespace x86_64_ir_translator
