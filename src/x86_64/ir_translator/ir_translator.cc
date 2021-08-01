//
//  ir_translator.cc
//  Katara
//
//  Created by Arne Philipeit on 2/15/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "ir_translator.h"

#include "src/common/logging.h"

namespace x86_64_ir_translator {

std::unique_ptr<x86_64::Program> IRTranslator::Translate(
    ir::Program* program,
    std::unordered_map<ir::Func*, ir_info::InterferenceGraph>& inteference_graphs) {
  IRTranslator translator(program, inteference_graphs);

  translator.AllocateRegisters();
  translator.TranslateProgram();

  return translator.x86_64_program_builder_.Build();
}

void IRTranslator::AllocateRegisters() {
  interference_graph_colors_.reserve(interference_graphs_.size());
  for (auto& ir_func : ir_program_->funcs()) {
    ir_info::InterferenceGraphColors colors =
        AllocateRegistersInFunc(ir_func.get(), interference_graphs_.at(ir_func.get()));
    interference_graph_colors_.insert({ir_func.get(), colors});
  }
}

void IRTranslator::TranslateProgram() {
  for (auto& ir_func : ir_program_->funcs()) {
    std::string ir_func_name = ir_func->name();
    if (ir_func_name.empty()) {
      ir_func_name = "Func" + std::to_string(ir_func->number());
    }

    x86_64::Func* x86_64_func =
        TranslateFunc(ir_func.get(), x86_64_program_builder_.AddFunc(ir_func_name));

    if (ir_func->number() == ir_program_->entry_func_num()) {
      x86_64_main_func_ = x86_64_func;
    }
  }
}

x86_64::Func* IRTranslator::TranslateFunc(ir::Func* ir_func,
                                          x86_64::FuncBuilder x86_64_func_builder) {
  std::vector<ir::Block*> ir_blocks;
  ir_blocks.reserve(ir_func->blocks().size());
  for (auto& ir_block : ir_func->blocks()) {
    ir_blocks.push_back(ir_block.get());
  }
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

    TranslateBlock(ir_block, ir_func, x86_64_block_builder);
  }

  return x86_64_func_builder.func();
}

x86_64::Block* IRTranslator::TranslateBlock(ir::Block* ir_block, ir::Func* ir_func,
                                            x86_64::BlockBuilder x86_64_block_builder) {
  for (auto& ir_instr : ir_block->instrs()) {
    TranslateInstr(ir_instr.get(), ir_func, x86_64_block_builder);
  }

  return x86_64_block_builder.block();
}

void IRTranslator::TranslateInstr(ir::Instr* ir_instr, ir::Func* ir_func,
                                  x86_64::BlockBuilder& x86_64_block_builder) {
  switch (ir_instr->instr_kind()) {
    case ir::InstrKind::kMov:
      TranslateMovInstr(static_cast<ir::MovInstr*>(ir_instr), ir_func, x86_64_block_builder);
      break;
    case ir::InstrKind::kBoolNot:
      TranslateBoolNotInstr(static_cast<ir::BoolNotInstr*>(ir_instr), ir_func,
                            x86_64_block_builder);
      break;
    case ir::InstrKind::kBoolBinary:
      TranslateBoolBinaryInstr(static_cast<ir::BoolBinaryInstr*>(ir_instr), ir_func,
                               x86_64_block_builder);
      break;
    case ir::InstrKind::kIntUnary:
      TranslateIntUnaryInstr(static_cast<ir::IntUnaryInstr*>(ir_instr), ir_func,
                             x86_64_block_builder);
      break;
    case ir::InstrKind::kIntCompare:
      TranslateIntCompareInstr(static_cast<ir::IntCompareInstr*>(ir_instr), ir_func,
                               x86_64_block_builder);
      break;
    case ir::InstrKind::kIntBinary:
      TranslateIntBinaryInstr(static_cast<ir::IntBinaryInstr*>(ir_instr), ir_func,
                              x86_64_block_builder);
      break;
    case ir::InstrKind::kIntShift:
      TranslateIntShiftInstr(static_cast<ir::IntShiftInstr*>(ir_instr), ir_func,
                             x86_64_block_builder);
      break;
    case ir::InstrKind::kJump:
      TranslateJumpInstr(static_cast<ir::JumpInstr*>(ir_instr), x86_64_block_builder);
      break;
    case ir::InstrKind::kJumpCond:
      TranslateJumpCondInstr(static_cast<ir::JumpCondInstr*>(ir_instr), ir_func,
                             x86_64_block_builder);
      break;
    case ir::InstrKind::kCall:
      TranslateCallInstr(static_cast<ir::CallInstr*>(ir_instr), ir_func, x86_64_block_builder);
      break;
    case ir::InstrKind::kReturn:
      TranslateReturnInstr(static_cast<ir::ReturnInstr*>(ir_instr), ir_func, x86_64_block_builder);
      break;
    default:
      common::fail("unexpected instr");
  }
}

void IRTranslator::TranslateMovInstr(ir::MovInstr* ir_mov_instr, ir::Func* ir_func,
                                     x86_64::BlockBuilder& x86_64_block_builder) {
  auto ir_result = ir_mov_instr->result().get();
  auto ir_origin = ir_mov_instr->origin().get();

  GenerateMovs(ir_result, ir_origin, ir_func, x86_64_block_builder);
}

void IRTranslator::TranslateBoolNotInstr(ir::BoolNotInstr* ir_bool_not_instr, ir::Func* ir_func,
                                         x86_64::BlockBuilder& x86_64_block_builder) {
  auto ir_result = ir_bool_not_instr->result().get();
  auto ir_operand = ir_bool_not_instr->operand().get();

  GenerateMovs(ir_result, ir_operand, ir_func, x86_64_block_builder);

  x86_64::RM x86_64_operand = TranslateComputed(ir_result, ir_func);

  x86_64_block_builder.AddInstr<x86_64::Not>(x86_64_operand);
}

void IRTranslator::TranslateBoolBinaryInstr(ir::BoolBinaryInstr* ir_bool_binary_instr,
                                            ir::Func* ir_func,
                                            x86_64::BlockBuilder& x86_64_block_builder) {
  switch (ir_bool_binary_instr->operation()) {
    case common::Bool::BinaryOp::kEq:
    case common::Bool::BinaryOp::kNeq:
      TranslateBoolCompareInstr(ir_bool_binary_instr, ir_func, x86_64_block_builder);
      break;
    case common::Bool::BinaryOp::kAnd:
    case common::Bool::BinaryOp::kOr:
      TranslateBoolLogicInstr(ir_bool_binary_instr, ir_func, x86_64_block_builder);
      break;
  }
}

void IRTranslator::TranslateBoolCompareInstr(ir::BoolBinaryInstr* ir_bool_compare_instr,
                                             ir::Func* ir_func,
                                             x86_64::BlockBuilder& x86_64_block_builder) {
  // Note: It is assumed that neither operand is a constant. A constant folding optimization pass
  // should ensure this.
  auto ir_result = ir_bool_compare_instr->result().get();
  auto ir_operand_a = static_cast<ir::Computed*>(ir_bool_compare_instr->operand_a().get());
  auto ir_operand_b = static_cast<ir::Computed*>(ir_bool_compare_instr->operand_b().get());

  x86_64::RM x86_64_result = TranslateComputed(ir_result, ir_func);
  x86_64::RM x86_64_operand_a = TranslateComputed(ir_operand_a, ir_func);
  x86_64::RM x86_64_operand_b = TranslateComputed(ir_operand_b, ir_func);

  bool requires_tmp_reg = x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem();
  bool required_tmp_reg_is_result_reg = false;
  x86_64::Reg x86_64_tmp_reg(x86_64::Size::k8, 0);
  if (x86_64_result.is_reg()) {
    required_tmp_reg_is_result_reg = true;
    x86_64_tmp_reg = x86_64::Reg(x86_64::Size::k8, x86_64_result.reg().reg());
  }

  x86_64::InstrCond x86_64_cond = [](common::Bool::BinaryOp op) {
    switch (op) {
      case common::Bool::BinaryOp::kEq:
        return x86_64::InstrCond::kEqual;
      case common::Bool::BinaryOp::kNeq:
        return x86_64::InstrCond::kNotEqual;
      default:
        common::fail("unexpected bool compare op");
    }
  }(ir_bool_compare_instr->operation());

  if (requires_tmp_reg) {
    if (!required_tmp_reg_is_result_reg) {
      x86_64_block_builder.AddInstr<x86_64::Push>(x86_64_tmp_reg);
    }
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_tmp_reg, x86_64_operand_b);
    x86_64_operand_b = x86_64_tmp_reg;
  }

  x86_64_block_builder.AddInstr<x86_64::Cmp>(x86_64_operand_a, x86_64_operand_b);
  x86_64_block_builder.AddInstr<x86_64::Setcc>(x86_64_cond, x86_64_result);

  if (requires_tmp_reg && !required_tmp_reg_is_result_reg) {
    x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64_tmp_reg);
  }
}

void IRTranslator::TranslateBoolLogicInstr(ir::BoolBinaryInstr* ir_bool_logic_instr,
                                           ir::Func* ir_func,
                                           x86_64::BlockBuilder& x86_64_block_builder) {
  // Note: It is assumed that neither operand is a constant. A constant folding optimization pass
  // should ensure this.
  auto ir_result = ir_bool_logic_instr->result().get();
  auto ir_operand_a = static_cast<ir::Computed*>(ir_bool_logic_instr->operand_a().get());
  auto ir_operand_b = static_cast<ir::Computed*>(ir_bool_logic_instr->operand_b().get());

  GenerateMovs(ir_result, ir_operand_a, ir_func, x86_64_block_builder);

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_result, ir_func);
  x86_64::RM x86_64_operand_b = TranslateComputed(ir_operand_b, ir_func);

  bool requires_tmp_reg = x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem();
  x86_64::Reg x86_64_tmp_reg(x86_64::Size::k8, 0);

  if (requires_tmp_reg) {
    x86_64_block_builder.AddInstr<x86_64::Push>(x86_64_tmp_reg);
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_tmp_reg, x86_64_operand_b);
    x86_64_operand_b = x86_64_tmp_reg;
  }

  switch (ir_bool_logic_instr->operation()) {
    case common::Bool::BinaryOp::kAnd:
      x86_64_block_builder.AddInstr<x86_64::And>(x86_64_operand_a, x86_64_operand_b);
      break;
    case common::Bool::BinaryOp::kOr:
      x86_64_block_builder.AddInstr<x86_64::Or>(x86_64_operand_a, x86_64_operand_b);
      break;
    default:
      common::fail("unexpexted bool logic op");
  }

  if (requires_tmp_reg) {
    x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64_tmp_reg);
  }
}

void IRTranslator::TranslateIntUnaryInstr(ir::IntUnaryInstr* ir_int_unary_instr, ir::Func* ir_func,
                                          x86_64::BlockBuilder& x86_64_block_builder) {
  // Note: It is assumed that the operand is not a constant. A constant folding optimization pass
  // should ensure this.
  auto ir_result = ir_int_unary_instr->result().get();
  auto ir_operand_a = static_cast<ir::Computed*>(ir_int_unary_instr->operand().get());

  GenerateMovs(ir_result, ir_operand_a, ir_func, x86_64_block_builder);

  x86_64::RM x86_64_operand = TranslateComputed(ir_result, ir_func);

  switch (ir_int_unary_instr->operation()) {
    case common::Int::UnaryOp::kNot:
      x86_64_block_builder.AddInstr<x86_64::Not>(x86_64_operand);
      break;
    case common::Int::UnaryOp::kNeg:
      x86_64_block_builder.AddInstr<x86_64::Neg>(x86_64_operand);
      break;
    default:
      common::fail("unexpected unary int op");
  }
}

void IRTranslator::TranslateIntCompareInstr(ir::IntCompareInstr* ir_int_compare_instr,
                                            ir::Func* ir_func,
                                            x86_64::BlockBuilder& x86_64_block_builder) {
  // Note: It is assumed that at least one operand is not a constant. A constant folding
  // optimization pass should ensure this.
  common::Int::CompareOp op = ir_int_compare_instr->operation();
  auto ir_result = ir_int_compare_instr->result().get();
  auto ir_operand_a = ir_int_compare_instr->operand_a().get();
  auto ir_operand_b = ir_int_compare_instr->operand_b().get();
  auto ir_type = static_cast<const ir::IntType*>(ir_result->type());
  bool is_signed = common::IsSigned(ir_type->int_type());

  if (ir_operand_a->kind() == ir::Value::Kind::kConstant) {
    op = common::Flipped(op);
    std::swap(ir_operand_a, ir_operand_b);
  }

  x86_64::RM x86_64_result = TranslateComputed(ir_result, ir_func);
  x86_64::RM x86_64_operand_a =
      TranslateComputed(static_cast<ir::Computed*>(ir_operand_a), ir_func);
  x86_64::Operand x86_64_operand_b = TranslateValue(ir_operand_b, ir_func);

  bool requires_tmp_reg = false;
  if (ir_operand_b->kind() == ir::Value::Kind::kConstant) {
    common::Int value = static_cast<ir::IntConstant*>(ir_operand_b)->value();
    if (value.CanConvertTo(common::IntType::kI32)) {
      if (common::BitSizeOf(value.type()) > 32) {
        ir::IntConstant ir_constant(value.ConvertTo(common::IntType::kI32));
        x86_64_operand_b = TranslateIntConstant(&ir_constant);
      }
    } else {
      requires_tmp_reg = true;
    }
  } else if (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem()) {
    requires_tmp_reg = true;
  }

  bool required_tmp_reg_is_result_reg = false;
  if (!x86_64_operand_a.is_reg() || x86_64_operand_a.reg().reg() != x86_64_result.reg().reg()) {
    required_tmp_reg_is_result_reg = x86_64_result.is_reg();
  }

  x86_64::Size x86_64_size = x86_64::Size(common::BitSizeOf(ir_type->int_type()));
  x86_64::Reg x86_64_tmp_reg(x86_64_size, 0);
  if (!required_tmp_reg_is_result_reg && x86_64_operand_a.is_reg() &&
      x86_64_operand_a.reg().reg() == 0) {
    x86_64_tmp_reg = x86_64::Reg(x86_64_size, 1);
  } else if (required_tmp_reg_is_result_reg) {
    x86_64_tmp_reg = x86_64::Reg(x86_64_size, x86_64_result.reg().reg());
  }

  x86_64::InstrCond x86_64_cond = [op, is_signed]() {
    switch (op) {
      case common::Int::CompareOp::kEq:
        return x86_64::InstrCond::kEqual;
      case common::Int::CompareOp::kNeq:
        return x86_64::InstrCond::kNotEqual;
      case common::Int::CompareOp::kLss:
        return is_signed ? x86_64::InstrCond::kLess : x86_64::InstrCond::kBelow;
      case common::Int::CompareOp::kLeq:
        return is_signed ? x86_64::InstrCond::kLessOrEqual : x86_64::InstrCond::kBelowOrEqual;
      case common::Int::CompareOp::kGeq:
        return is_signed ? x86_64::InstrCond::kGreaterOrEqual : x86_64::InstrCond::kAboveOrEqual;
      case common::Int::CompareOp::kGtr:
        return is_signed ? x86_64::InstrCond::kGreater : x86_64::InstrCond::kAbove;
    }
  }();

  if (requires_tmp_reg) {
    if (!required_tmp_reg_is_result_reg) {
      x86_64_block_builder.AddInstr<x86_64::Push>(x86_64_tmp_reg);
    }
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_tmp_reg, x86_64_operand_b);
    x86_64_operand_b = x86_64_tmp_reg;
  }

  x86_64_block_builder.AddInstr<x86_64::Cmp>(x86_64_operand_a, x86_64_operand_b);
  x86_64_block_builder.AddInstr<x86_64::Setcc>(x86_64_cond, x86_64_result);

  if (requires_tmp_reg && !required_tmp_reg_is_result_reg) {
    x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64_tmp_reg);
  }
}

void IRTranslator::TranslateIntBinaryInstr(ir::IntBinaryInstr* ir_int_binary_instr,
                                           ir::Func* ir_func,
                                           x86_64::BlockBuilder& x86_64_block_builder) {
  switch (ir_int_binary_instr->operation()) {
    case common::Int::BinaryOp::kAdd:
    case common::Int::BinaryOp::kSub:
    case common::Int::BinaryOp::kAnd:
    case common::Int::BinaryOp::kOr:
    case common::Int::BinaryOp::kXor:
      TranslateIntSimpleALInstr(ir_int_binary_instr, ir_func, x86_64_block_builder);
      break;
    case common::Int::BinaryOp::kAndNot:
      common::fail("int andnot operation was not decomposed into separate instrs");
    case common::Int::BinaryOp::kMul:
      TranslateIntMulInstr(ir_int_binary_instr, ir_func, x86_64_block_builder);
      break;
    case common::Int::BinaryOp::kDiv:
    case common::Int::BinaryOp::kRem:
      TranslateIntDivOrRemInstr(ir_int_binary_instr, ir_func, x86_64_block_builder);
      break;
  }
}

void IRTranslator::TranslateIntSimpleALInstr(ir::IntBinaryInstr* ir_int_binary_instr,
                                             ir::Func* ir_func,
                                             x86_64::BlockBuilder& x86_64_block_builder) {
  // Note: It is assumed that at least one operand is not a constant. A constant folding
  // optimization pass should ensure this.
  common::Int::BinaryOp op = ir_int_binary_instr->operation();
  auto ir_result = ir_int_binary_instr->result().get();
  auto ir_operand_a = ir_int_binary_instr->operand_a().get();
  auto ir_operand_b = ir_int_binary_instr->operand_b().get();
  auto ir_type = static_cast<const ir::Type*>(ir_result->type());

  if (ir_operand_a->kind() == ir::Value::Kind::kConstant) {
    switch (op) {
      case common::Int::BinaryOp::kAdd:
      case common::Int::BinaryOp::kAnd:
      case common::Int::BinaryOp::kOr:
      case common::Int::BinaryOp::kXor:
        std::swap(ir_operand_a, ir_operand_b);
        break;
      default:
        break;
    }
  }

  GenerateMovs(ir_result, ir_operand_a, ir_func, x86_64_block_builder);

  bool requires_tmp_reg = false;

  ir::IntConstant ir_operand_b_narrowed(common::Int(int32_t{0}));
  if (ir_operand_b->kind() == ir::Value::Kind::kConstant &&
      (ir_type == ir::i64() || ir_type == ir::u64() || ir_type == ir::pointer_type() ||
       ir_type == ir::func_type())) {
    common::Int v{int64_t{0}};
    if (ir_operand_b->type() == ir::i64()) {
      v = static_cast<ir::IntConstant*>(ir_operand_b)->value();
    } else if (ir_operand_b->type() == ir::u64()) {
      v = static_cast<ir::IntConstant*>(ir_operand_b)->value();
    } else if (ir_operand_b->type() == ir::pointer_type()) {
      v = common::Int(static_cast<ir::PointerConstant*>(ir_operand_b)->value());
    } else if (ir_operand_b->type() == ir::func_type()) {
      v = common::Int(static_cast<ir::FuncConstant*>(ir_operand_b)->value());
    }
    if (common::IsSigned(v.type()) && v.CanConvertTo(common::IntType::kI32)) {
      ir_operand_b_narrowed = ir::IntConstant(v.ConvertTo(common::IntType::kI32));
      ir_operand_b = &ir_operand_b_narrowed;
    } else if (common::IsUnsigned(v.type()) && v.CanConvertTo(common::IntType::kU32)) {
      ir_operand_b_narrowed = ir::IntConstant(v.ConvertTo(common::IntType::kU32));
      ir_operand_b = &ir_operand_b_narrowed;
    } else {
      requires_tmp_reg = true;
    }
  }

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_result, ir_func);
  x86_64::Operand x86_64_operand_b = TranslateValue(ir_operand_b, ir_func);

  if (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem()) {
    requires_tmp_reg = true;
  }

  x86_64::Size x86_64_size = [ir_type]() {
    switch (ir_type->type_kind()) {
      case ir::TypeKind::kInt:
        return x86_64::Size(
            common::BitSizeOf(static_cast<const ir::IntType*>(ir_type)->int_type()));
      case ir::TypeKind::kPointer:
      case ir::TypeKind::kFunc:
        return x86_64::Size::k64;
      default:
        common::fail("unexpected int binary operand type");
    }
  }();
  x86_64::Reg x86_64_tmp_reg(x86_64_size, 0);
  if (x86_64_operand_a.is_reg() && x86_64_operand_a.reg().reg() == 0) {
    x86_64_tmp_reg = x86_64::Reg(x86_64_size, 1);
  }

  if (requires_tmp_reg) {
    x86_64_block_builder.AddInstr<x86_64::Push>(x86_64_tmp_reg);
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_tmp_reg, x86_64_operand_b);
    x86_64_operand_b = x86_64_tmp_reg;
  }

  switch (op) {
    case common::Int::BinaryOp::kAdd:
      x86_64_block_builder.AddInstr<x86_64::Add>(x86_64_operand_a, x86_64_operand_b);
      break;
    case common::Int::BinaryOp::kSub:
      x86_64_block_builder.AddInstr<x86_64::Sub>(x86_64_operand_a, x86_64_operand_b);
      break;

    case common::Int::BinaryOp::kAnd:
      x86_64_block_builder.AddInstr<x86_64::And>(x86_64_operand_a, x86_64_operand_b);
      break;
    case common::Int::BinaryOp::kOr:
      x86_64_block_builder.AddInstr<x86_64::Or>(x86_64_operand_a, x86_64_operand_b);
      break;
    case common::Int::BinaryOp::kXor:
      x86_64_block_builder.AddInstr<x86_64::Xor>(x86_64_operand_a, x86_64_operand_b);
      break;

    default:
      common::fail("unexpexted simple int binary operation");
  }

  if (requires_tmp_reg) {
    x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64_tmp_reg);
  }
}

void IRTranslator::TranslateIntMulInstr(ir::IntBinaryInstr* ir_int_binary_instr, ir::Func* ir_func,
                                        x86_64::BlockBuilder& x86_64_block_builder) {
  // Note: It is assumed that at least one operand is not a constant. A constant folding
  // optimization pass should ensure this.
  auto ir_result = ir_int_binary_instr->result().get();
  auto ir_operand_a = ir_int_binary_instr->operand_a().get();
  auto ir_operand_b = ir_int_binary_instr->operand_b().get();
  auto ir_type = static_cast<const ir::Type*>(ir_result->type());

  if (ir_operand_a->kind() == ir::Value::Kind::kConstant) {
    std::swap(ir_operand_a, ir_operand_b);
  }

  bool requires_tmp_reg = false;
  ir::IntConstant ir_operand_b_narrowed(common::Int(int32_t{0}));
  if (ir_operand_b->kind() == ir::Value::Kind::kConstant &&
      (ir_type == ir::i64() || ir_type == ir::u64() || ir_type == ir::pointer_type() ||
       ir_type == ir::func_type())) {
    common::Int v{int64_t{0}};
    if (ir_operand_b->type() == ir::i64()) {
      v = static_cast<ir::IntConstant*>(ir_operand_b)->value();
    } else if (ir_operand_b->type() == ir::u64()) {
      v = static_cast<ir::IntConstant*>(ir_operand_b)->value();
    } else if (ir_operand_b->type() == ir::pointer_type()) {
      v = common::Int(static_cast<ir::PointerConstant*>(ir_operand_b)->value());
    } else if (ir_operand_b->type() == ir::func_type()) {
      v = common::Int(static_cast<ir::FuncConstant*>(ir_operand_b)->value());
    }
    if (common::IsSigned(v.type()) && v.CanConvertTo(common::IntType::kI32)) {
      ir_operand_b_narrowed = ir::IntConstant(v.ConvertTo(common::IntType::kI32));
      ir_operand_b = &ir_operand_b_narrowed;
    } else if (common::IsUnsigned(v.type()) && v.CanConvertTo(common::IntType::kU32)) {
      ir_operand_b_narrowed = ir::IntConstant(v.ConvertTo(common::IntType::kU32));
      ir_operand_b = &ir_operand_b_narrowed;
    } else {
      requires_tmp_reg = true;
    }
  }

  x86_64::RM x86_64_result = TranslateComputed(ir_result, ir_func);
  x86_64::RM x86_64_operand_a =
      TranslateComputed(static_cast<ir::Computed*>(ir_operand_a), ir_func);
  x86_64::Operand x86_64_operand_b = TranslateValue(ir_operand_b, ir_func);

  if (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem()) {
    requires_tmp_reg = true;
  }

  x86_64::Size x86_64_size = [ir_type]() {
    switch (ir_type->type_kind()) {
      case ir::TypeKind::kInt:
        return x86_64::Size(
            common::BitSizeOf(static_cast<const ir::IntType*>(ir_type)->int_type()));
      case ir::TypeKind::kPointer:
      case ir::TypeKind::kFunc:
        return x86_64::Size::k64;
      default:
        common::fail("unexpected int binary operand type");
    }
  }();
  x86_64::Reg x86_64_tmp_reg(x86_64_size, 0);
  while ((x86_64_operand_a.is_reg() && x86_64_operand_a.reg().reg() == x86_64_tmp_reg.reg()) ||
         (x86_64_operand_b.is_reg() && x86_64_operand_b.reg().reg() == x86_64_tmp_reg.reg())) {
    x86_64_tmp_reg = x86_64::Reg(x86_64_size, x86_64_tmp_reg.reg() + 1);
  }

  if (x86_64_result.is_reg()) {
    if (x86_64_operand_b.is_imm() && !requires_tmp_reg) {
      x86_64_block_builder.AddInstr<x86_64::Imul>(x86_64_result.reg(), x86_64_operand_a,
                                                  x86_64_operand_b.imm());
    } else {
      x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_result.reg(), x86_64_operand_b);
      x86_64_block_builder.AddInstr<x86_64::Imul>(x86_64_result.reg(), x86_64_operand_a);
    }

  } else {
    x86_64_block_builder.AddInstr<x86_64::Push>(x86_64_tmp_reg);
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_tmp_reg, x86_64_operand_b);
    x86_64_block_builder.AddInstr<x86_64::Imul>(x86_64_tmp_reg, x86_64_operand_a);
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_result, x86_64_tmp_reg);
    x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64_tmp_reg);
  }
}

void IRTranslator::TranslateIntDivOrRemInstr(ir::IntBinaryInstr* ir_int_binary_instr,
                                             ir::Func* ir_func,
                                             x86_64::BlockBuilder& x86_64_block_builder) {
  // TODO: implement
}

void IRTranslator::TranslateIntShiftInstr(ir::IntShiftInstr* ir_int_shift_instr, ir::Func* ir_func,
                                          x86_64::BlockBuilder& x86_64_block_builder) {
  // TODO: implement
}

void IRTranslator::TranslateJumpInstr(ir::JumpInstr* ir_jump_instr,
                                      x86_64::BlockBuilder& x86_64_block_builder) {
  ir::block_num_t ir_destination = ir_jump_instr->destination();
  x86_64::BlockRef x86_64_destination = TranslateBlockValue(ir_destination);

  x86_64_block_builder.AddInstr<x86_64::Jmp>(x86_64_destination);
}

void IRTranslator::TranslateJumpCondInstr(ir::JumpCondInstr* ir_jump_cond_instr, ir::Func* ir_func,
                                          x86_64::BlockBuilder& x86_64_block_builder) {
  auto ir_condition = ir_jump_cond_instr->condition().get();
  ir::block_num_t ir_destination_true = ir_jump_cond_instr->destination_true();
  ir::block_num_t ir_destination_false = ir_jump_cond_instr->destination_false();

  x86_64::BlockRef x86_64_destination_true = TranslateBlockValue(ir_destination_true);
  x86_64::BlockRef x86_64_destination_false = TranslateBlockValue(ir_destination_false);

  switch (ir_condition->kind()) {
    case ir::Value::Kind::kConstant: {
      auto ir_condition_constant = static_cast<ir::BoolConstant*>(ir_condition);
      if (ir_condition_constant->value()) {
        x86_64_block_builder.AddInstr<x86_64::Jmp>(x86_64_destination_true);
      } else {
        x86_64_block_builder.AddInstr<x86_64::Jmp>(x86_64_destination_false);
      }
      return;
    }
    case ir::Value::Kind::kComputed: {
      auto ir_condition_computed = static_cast<ir::Computed*>(ir_condition);
      x86_64::RM x86_64_condition = TranslateComputed(ir_condition_computed, ir_func);

      x86_64_block_builder.AddInstr<x86_64::Test>(x86_64_condition, x86_64::Imm(int8_t{-1}));
      x86_64_block_builder.AddInstr<x86_64::Jcc>(x86_64::InstrCond::kNoZero,
                                                 x86_64_destination_true);
      x86_64_block_builder.AddInstr<x86_64::Jmp>(x86_64_destination_false);
      return;
    }
    case ir::Value::Kind::kInherited:
      common::fail("unexpected condition value kind");
  }
}

void IRTranslator::TranslateCallInstr(ir::CallInstr* ir_call_instr, ir::Func* ir_func,
                                      x86_64::BlockBuilder& x86_64_block_builder) {
  // TODO: implement
}

void IRTranslator::TranslateReturnInstr(ir::ReturnInstr* ir_return_instr, ir::Func* ir_func,
                                        x86_64::BlockBuilder& x86_64_block_builder) {
  // TODO: implement
  GenerateFuncEpilogue(ir_func, x86_64_block_builder);
}

void IRTranslator::GenerateFuncPrologue(ir::Func* /*ir_func*/,
                                        x86_64::BlockBuilder& x86_64_block_builder) {
  x86_64_block_builder.AddInstr<x86_64::Push>(x86_64::rbp);
  x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64::rbp, x86_64::rsp);
  // TODO: reserve stack space
}

void IRTranslator::GenerateFuncEpilogue(ir::Func* /*ir_func*/,
                                        x86_64::BlockBuilder& x86_64_block_builder) {
  // TODO: revert stack pointer
  x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64::rbp);
  x86_64_block_builder.AddInstr<x86_64::Ret>();
}

void IRTranslator::GenerateMovs(ir::Computed* ir_result, ir::Value* ir_origin, ir::Func* ir_func,
                                x86_64::BlockBuilder& x86_64_block_builder) {
  x86_64::RM x86_64_result = TranslateComputed(ir_result, ir_func);
  x86_64::Operand x86_64_origin = TranslateValue(ir_origin, ir_func);

  if (x86_64_result == x86_64_origin) {
    return;
  }

  if (!x86_64_result.is_mem() || !x86_64_origin.is_mem()) {
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_result, x86_64_origin);

  } else {
    auto ir_type = static_cast<const ir::AtomicType*>(ir_result->type());
    x86_64::Size x86_64_size = x86_64::Size(ir_type->bit_size());
    x86_64::Reg x86_64_tmp_reg = x86_64::Reg(x86_64_size, 0);

    x86_64_block_builder.AddInstr<x86_64::Push>(x86_64_tmp_reg);
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_tmp_reg, x86_64_origin);
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_result, x86_64_tmp_reg);
    x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64_tmp_reg);
  }
}

x86_64::Operand IRTranslator::TranslateValue(ir::Value* value, ir::Func* ir_func) {
  switch (value->kind()) {
    case ir::Value::Kind::kConstant:
      switch (static_cast<const ir::AtomicType*>(value->type())->type_kind()) {
        case ir::TypeKind::kBool:
          return TranslateBoolConstant(static_cast<ir::BoolConstant*>(value));
        case ir::TypeKind::kInt:
          return TranslateIntConstant(static_cast<ir::IntConstant*>(value));
        case ir::TypeKind::kPointer:
          return TranslatePointerConstant(static_cast<ir::PointerConstant*>(value));
        case ir::TypeKind::kFunc:
          return TranslateFuncConstant(static_cast<ir::FuncConstant*>(value));
        default:
          common::fail("unsupported constant kind");
      }
    case ir::Value::Kind::kComputed:
      return TranslateComputed(static_cast<ir::Computed*>(value), ir_func);
    default:
      common::fail("unsupported value kind");
  }
}

x86_64::Imm IRTranslator::TranslateBoolConstant(ir::BoolConstant* constant) {
  return constant->value() ? x86_64::Imm(int8_t{1}) : x86_64::Imm(int8_t{0});
}

x86_64::Imm IRTranslator::TranslateIntConstant(ir::IntConstant* constant) {
  switch (constant->value().type()) {
    case common::IntType::kI8:
    case common::IntType::kU8:
      return x86_64::Imm(int8_t(constant->value().AsInt64()));
    case common::IntType::kI16:
    case common::IntType::kU16:
      return x86_64::Imm(int16_t(constant->value().AsInt64()));
    case common::IntType::kI32:
    case common::IntType::kU32:
      return x86_64::Imm(int32_t(constant->value().AsInt64()));
    case common::IntType::kI64:
    case common::IntType::kU64:
      return x86_64::Imm(int64_t(constant->value().AsInt64()));
  }
}

x86_64::Imm IRTranslator::TranslatePointerConstant(ir::PointerConstant* constant) {
  return x86_64::Imm(constant->value());
}

x86_64::Imm IRTranslator::TranslateFuncConstant(ir::FuncConstant* constant) {
  return x86_64::Imm(constant->value());
}

x86_64::RM IRTranslator::TranslateComputed(ir::Computed* computed, ir::Func* ir_func) {
  ir_info::InterferenceGraphColors& colors = interference_graph_colors_.at(ir_func);
  ir_info::color_t color = colors.GetColor(computed->number());
  int8_t ir_size = static_cast<const ir::AtomicType*>(computed->type())->bit_size();
  x86_64::Size x86_64_size = x86_64::Size(ir_size);
  return ColorAndSizeToOperand(color, x86_64_size);
}

x86_64::BlockRef IRTranslator::TranslateBlockValue(ir::block_num_t block_value) {
  return x86_64::BlockRef(block_value);
}

x86_64::FuncRef IRTranslator::TranslateFuncValue(ir::Value /*func_value*/) {
  // TODO: keep track of func nums from ir to x86_64
  return x86_64::FuncRef(-1);
}

}  // namespace x86_64_ir_translator
