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
    ir::Program* program, std::unordered_map<ir::func_num_t, ir_info::FuncLiveRanges>& live_ranges,
    std::unordered_map<ir::func_num_t, ir_info::InterferenceGraph>& inteference_graphs) {
  IRTranslator translator(program, live_ranges, inteference_graphs);

  translator.AllocateRegisters();
  translator.TranslateProgram();

  return translator.x86_64_program_builder_.Build();
}

void IRTranslator::AllocateRegisters() {
  interference_graph_colors_.reserve(interference_graphs_.size());
  for (auto& ir_func : ir_program_->funcs()) {
    ir_info::InterferenceGraphColors colors =
        AllocateRegistersInFunc(ir_func.get(), interference_graphs_.at(ir_func->number()));
    interference_graph_colors_.insert({ir_func->number(), colors});
  }
}

void IRTranslator::TranslateProgram() {
  x86_64_program_builder_.DeclareFunc("malloc");
  x86_64_program_builder_.DeclareFunc("free");

  for (auto& ir_func : ir_program_->funcs()) {
    std::string ir_func_name = ir_func->name();
    if (ir_func_name.empty()) {
      ir_func_name = "Func" + std::to_string(ir_func->number());
    }

    x86_64::Func* x86_64_func =
        TranslateFunc(ir_func.get(), x86_64_program_builder_.DefineFunc(ir_func_name));

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
    case ir::InstrKind::kPointerOffset:
      TranslatePointerOffsetInstr(static_cast<ir::PointerOffsetInstr*>(ir_instr), ir_func,
                                  x86_64_block_builder);
      break;
    case ir::InstrKind::kNilTest:
      TranslateNilTestInstr(static_cast<ir::NilTestInstr*>(ir_instr), ir_func,
                            x86_64_block_builder);
      break;
    case ir::InstrKind::kMalloc:
      TranslateMallocInstr(static_cast<ir::MallocInstr*>(ir_instr), ir_func, x86_64_block_builder);
      break;
    case ir::InstrKind::kLoad:
      TranslateLoadInstr(static_cast<ir::LoadInstr*>(ir_instr), ir_func, x86_64_block_builder);
      break;
    case ir::InstrKind::kStore:
      TranslateStoreInstr(static_cast<ir::StoreInstr*>(ir_instr), ir_func, x86_64_block_builder);
      break;
    case ir::InstrKind::kFree:
      TranslateFreeInstr(static_cast<ir::FreeInstr*>(ir_instr), ir_func, x86_64_block_builder);
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
    case ir::InstrKind::kLangPanic:
      // TODO: add lowering pass
      break;
    default:
      common::fail("unexpected instr: " + ir_instr->ToString());
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
  auto ir_type = static_cast<const ir::IntType*>(ir_operand_a->type());
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
        auto ir_constant = ir::ToIntConstant(value.ConvertTo(common::IntType::kI32));
        x86_64_operand_b = TranslateIntConstant(ir_constant.get());
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
  auto ir_type = static_cast<const ir::IntType*>(ir_result->type());

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

  std::shared_ptr<ir::IntConstant> ir_operand_b_narrowed;
  if (ir_operand_b->kind() == ir::Value::Kind::kConstant &&
      (ir_type == ir::i64() || ir_type == ir::u64())) {
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
      ir_operand_b_narrowed = ir::ToIntConstant(v.ConvertTo(common::IntType::kI32));
      ir_operand_b = ir_operand_b_narrowed.get();
    } else if (common::IsUnsigned(v.type()) && v.CanConvertTo(common::IntType::kU32)) {
      ir_operand_b_narrowed = ir::ToIntConstant(v.ConvertTo(common::IntType::kU32));
      ir_operand_b = ir_operand_b_narrowed.get();
    } else {
      requires_tmp_reg = true;
    }
  }

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_result, ir_func);
  x86_64::Operand x86_64_operand_b = TranslateValue(ir_operand_b, ir_func);

  if (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem()) {
    requires_tmp_reg = true;
  }

  x86_64::Size x86_64_size = TranslateSizeOfIntType(ir_type);
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
  auto ir_type = static_cast<const ir::IntType*>(ir_result->type());

  if (ir_operand_a->kind() == ir::Value::Kind::kConstant) {
    std::swap(ir_operand_a, ir_operand_b);
  }

  bool requires_tmp_reg = false;
  std::shared_ptr<ir::IntConstant> ir_operand_b_narrowed;
  if (ir_operand_b->kind() == ir::Value::Kind::kConstant &&
      (ir_type == ir::i64() || ir_type == ir::u64())) {
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
      ir_operand_b_narrowed = ir::ToIntConstant(v.ConvertTo(common::IntType::kI32));
      ir_operand_b = ir_operand_b_narrowed.get();
    } else if (common::IsUnsigned(v.type()) && v.CanConvertTo(common::IntType::kU32)) {
      ir_operand_b_narrowed = ir::ToIntConstant(v.ConvertTo(common::IntType::kU32));
      ir_operand_b = ir_operand_b_narrowed.get();
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

  x86_64::Size x86_64_size = TranslateSizeOfIntType(ir_type);
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

void IRTranslator::TranslatePointerOffsetInstr(ir::PointerOffsetInstr* ir_pointer_offset_instr,
                                               ir::Func* ir_func,
                                               x86_64::BlockBuilder& x86_64_block_builder) {
  // Note: It is assumed that at least one operand is not a constant. A constant folding
  // optimization pass should ensure this.
  auto ir_result = ir_pointer_offset_instr->result().get();
  auto ir_pointer = ir_pointer_offset_instr->pointer().get();
  auto ir_offset = ir_pointer_offset_instr->offset().get();

  GenerateMovs(ir_result, ir_pointer, ir_func, x86_64_block_builder);

  bool requires_tmp_reg = false;

  std::shared_ptr<ir::IntConstant> ir_offset_narrowed;
  if (ir_offset->kind() == ir::Value::Kind::kConstant) {
    common::Int v = static_cast<ir::IntConstant*>(ir_offset)->value();
    if (v.CanConvertTo(common::IntType::kI32)) {
      ir_offset_narrowed = ir::ToIntConstant(v.ConvertTo(common::IntType::kI32));
      ir_offset = ir_offset_narrowed.get();
    } else {
      requires_tmp_reg = true;
    }
  }

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_result, ir_func);
  x86_64::Operand x86_64_operand_b = TranslateValue(ir_offset, ir_func);

  if (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem()) {
    requires_tmp_reg = true;
  }

  x86_64::Reg x86_64_tmp_reg = x86_64::rax;
  if (x86_64_operand_a == x86_64_tmp_reg) {
    x86_64_tmp_reg = x86_64::rdx;
  }

  if (requires_tmp_reg) {
    x86_64_block_builder.AddInstr<x86_64::Push>(x86_64_tmp_reg);
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_tmp_reg, x86_64_operand_b);
    x86_64_operand_b = x86_64_tmp_reg;
  }
  x86_64_block_builder.AddInstr<x86_64::Add>(x86_64_operand_a, x86_64_operand_b);
  if (requires_tmp_reg) {
    x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64_tmp_reg);
  }
}

void IRTranslator::TranslateNilTestInstr(ir::NilTestInstr* ir_nil_test_instr, ir::Func* ir_func,
                                         x86_64::BlockBuilder& x86_64_block_builder) {
  // Note: It is assumed that the tested operand is not constant. A constant folding optimization
  // pass should ensure this.
  auto ir_result = ir_nil_test_instr->result().get();
  auto ir_tested = ir_nil_test_instr->tested().get();

  x86_64::RM x86_64_result = TranslateComputed(ir_result, ir_func);
  x86_64::RM x86_64_tested = TranslateComputed(static_cast<ir::Computed*>(ir_tested), ir_func);

  x86_64_block_builder.AddInstr<x86_64::Cmp>(x86_64_tested, x86_64::Imm(0));
  x86_64_block_builder.AddInstr<x86_64::Setcc>(x86_64::InstrCond::kEqual, x86_64_result);
}

void IRTranslator::TranslateMallocInstr(ir::MallocInstr* ir_malloc_instr, ir::Func* ir_func,
                                        x86_64::BlockBuilder& x86_64_block_builder) {
  ir::Value* ir_size = ir_malloc_instr->size().get();
  ir::Computed* ir_result = ir_malloc_instr->result().get();

  x86_64::Operand x86_64_size = TranslateValue(ir_size, ir_func);
  x86_64::RM x86_64_result = TranslateComputed(ir_result, ir_func);

  x86_64::RM x86_64_size_location = OperandForArg(0, x86_64::Size::k64);
  x86_64::RM x86_64_result_location = OperandForResult(0, x86_64::k64);

  bool size_reg_needs_preservation = (x86_64_size != x86_64_size_location);
  bool result_reg_needs_preservation = (x86_64_result != x86_64_result_location);

  if (result_reg_needs_preservation) {
    x86_64_block_builder.AddInstr<x86_64::Push>(x86_64_result_location);
  }
  if (size_reg_needs_preservation) {
    x86_64_block_builder.AddInstr<x86_64::Push>(x86_64_size_location);
  }
  if (x86_64_size_location != x86_64_size) {
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_size_location, x86_64_size);
  }

  int64_t malloc_func_id = x86_64_program_builder_.program()->declared_funcs().at("malloc");
  x86_64_block_builder.AddInstr<x86_64::Call>(x86_64::FuncRef(malloc_func_id));
  if (x86_64_result_location != x86_64_result) {
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_result, x86_64_result_location);
  }
  if (size_reg_needs_preservation) {
    x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64_size_location);
  }
  if (result_reg_needs_preservation) {
    x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64_result_location);
  }
}

void IRTranslator::TranslateLoadInstr(ir::LoadInstr* ir_load_instr, ir::Func* ir_func,
                                      x86_64::BlockBuilder& x86_64_block_builder) {
  ir::Value* ir_address = ir_load_instr->address().get();
  ir::Computed* ir_result = ir_load_instr->result().get();

  x86_64::Operand x86_64_address_holder = TranslateValue(ir_address, ir_func);
  x86_64::RM x86_64_result = TranslateComputed(ir_result, ir_func);

  bool requires_address_reg = false;
  bool address_reg_needs_preservation = !x86_64_result.is_reg();
  x86_64::Reg x86_64_address_reg = (x86_64_result.is_reg()) ? x86_64_result.reg() : x86_64::rax;
  if (x86_64_address_holder.is_mem()) {
    requires_address_reg = true;

  } else if (x86_64_address_holder.is_imm()) {
    common::Int v = common::Int(static_cast<ir::PointerConstant*>(ir_address)->value());
    requires_address_reg = !v.CanConvertTo(common::IntType::kI32);

  } else if (x86_64_address_holder.is_reg()) {
    x86_64_address_reg = x86_64_address_holder.reg();
    requires_address_reg = true;
    address_reg_needs_preservation = false;
  }

  if (requires_address_reg) {
    if (address_reg_needs_preservation) {
      x86_64_block_builder.AddInstr<x86_64::Push>(x86_64_address_reg);
    }
    if (x86_64_address_holder != x86_64_address_reg) {
      x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_address_reg, x86_64_address_holder);
    }
  }

  x86_64::Size x86_64_size = TranslateSizeOfType(ir_result->type());
  x86_64::Mem mem(x86_64_size, int32_t{0});
  if (requires_address_reg) {
    mem = x86_64::Mem(x86_64_size, /*base_reg=*/uint8_t(x86_64_address_reg.reg()));
  } else {
    if (!x86_64_address_holder.is_imm()) {
      common::fail("unexpected load address kind");
    }
    mem = x86_64::Mem(x86_64_size, /*disp=*/int32_t(x86_64_address_holder.imm().value()));
  }
  x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_result, mem);

  if (requires_address_reg && address_reg_needs_preservation) {
    x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64_address_reg);
  }
}

void IRTranslator::TranslateStoreInstr(ir::StoreInstr* ir_store_instr, ir::Func* ir_func,
                                       x86_64::BlockBuilder& x86_64_block_builder) {
  ir::Value* ir_address = ir_store_instr->address().get();
  ir::Value* ir_value = ir_store_instr->value().get();

  x86_64::Operand x86_64_address_holder = TranslateValue(ir_address, ir_func);
  x86_64::Operand x86_64_value = TranslateValue(ir_value, ir_func);

  bool requires_value_reg = false;
  x86_64::Reg x86_64_value_reg = x86_64::rax;
  if (x86_64_value.is_imm() && x86_64_value.imm().size() == x86_64::k64) {
    common::Int v = common::Int(x86_64_value.imm().value());
    if (v.CanConvertTo(common::IntType::kI32)) {
      v.ConvertTo(common::IntType::kI32);
      x86_64_value = x86_64::Imm(int32_t(v.AsInt64()));
    } else {
      requires_value_reg = true;
    }
  }

  bool requires_address_reg = false;
  bool address_reg_needs_preservation = true;
  x86_64::Reg x86_64_address_reg = x86_64::rax;
  if (x86_64_address_holder.is_mem()) {
    requires_address_reg = true;

  } else if (x86_64_address_holder.is_imm()) {
    common::Int v = common::Int(static_cast<ir::PointerConstant*>(ir_address)->value());
    if (v.CanConvertTo(common::IntType::kI32)) {
      v = v.ConvertTo(common::IntType::kI32);
      x86_64_address_holder = x86_64::Imm(int32_t(v.AsInt64()));
    } else {
      requires_address_reg = true;
    }

  } else if (x86_64_address_holder.is_reg()) {
    x86_64_address_reg = x86_64_address_holder.reg();
    requires_address_reg = true;
    address_reg_needs_preservation = false;
  }

  if (x86_64_value_reg == x86_64_address_reg) {
    x86_64_value_reg = x86_64::rdx;
  }

  if (requires_value_reg) {
    x86_64_block_builder.AddInstr<x86_64::Push>(x86_64_value_reg);
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_value_reg, x86_64_value);
    x86_64_value = x86_64_value_reg;
  }

  if (requires_address_reg) {
    if (address_reg_needs_preservation) {
      x86_64_block_builder.AddInstr<x86_64::Push>(x86_64_address_reg);
    }
    if (x86_64_address_holder != x86_64_address_reg) {
      x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_address_reg, x86_64_address_holder);
    }
  }

  x86_64::Size x86_64_size = TranslateSizeOfType(ir_value->type());
  x86_64::Mem mem(x86_64_size, int32_t{0});
  if (requires_address_reg) {
    mem = x86_64::Mem(x86_64_size, /*base_reg=*/uint8_t(x86_64_address_reg.reg()));
  } else {
    if (!x86_64_address_holder.is_imm()) {
      common::fail("unexpected load address kind");
    }
    mem = x86_64::Mem(x86_64_size, /*disp=*/int32_t(x86_64_address_holder.imm().value()));
  }
  x86_64_block_builder.AddInstr<x86_64::Mov>(mem, x86_64_value);

  if (requires_address_reg && address_reg_needs_preservation) {
    x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64_address_reg);
  }
  if (requires_value_reg) {
    x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64_value_reg);
  }
}

void IRTranslator::TranslateFreeInstr(ir::FreeInstr* ir_free_instr, ir::Func* ir_func,
                                      x86_64::BlockBuilder& x86_64_block_builder) {
  ir::Value* ir_address = ir_free_instr->address().get();
  x86_64::Operand x86_64_address = TranslateValue(ir_address, ir_func);

  x86_64::RM x86_64_address_location = OperandForArg(0, x86_64::Size::k64);

  bool address_reg_needs_preservation = (x86_64_address != x86_64_address_location);

  if (address_reg_needs_preservation) {
    x86_64_block_builder.AddInstr<x86_64::Push>(x86_64_address_location);
  }
  if (x86_64_address_location != x86_64_address) {
    x86_64_block_builder.AddInstr<x86_64::Mov>(x86_64_address_location, x86_64_address);
  }

  int64_t free_func_id = x86_64_program_builder_.program()->declared_funcs().at("free");
  x86_64_block_builder.AddInstr<x86_64::Call>(x86_64::FuncRef(free_func_id));
  if (address_reg_needs_preservation) {
    x86_64_block_builder.AddInstr<x86_64::Pop>(x86_64_address_location);
  }
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

void IRTranslator::TranslateCallInstr(ir::CallInstr* ir_call_instr, ir::Func* ir_calling_func,
                                      x86_64::BlockBuilder& x86_64_block_builder) {
  struct ArgInfo {
    x86_64::Operand x86_64_arg_value;
    x86_64::RM x86_64_arg_location;
  };
  std::vector<ArgInfo> arg_infos;
  arg_infos.reserve(ir_call_instr->args().size());
  for (std::size_t arg_index = 0; arg_index < ir_call_instr->args().size(); arg_index++) {
    ir::Value* ir_arg_value = ir_call_instr->args().at(arg_index).get();
    x86_64::Operand x86_64_arg_value = TranslateValue(ir_arg_value, ir_calling_func);
    x86_64::Size x86_64_arg_size = TranslateSizeOfType(ir_arg_value->type());
    x86_64::RM x86_64_arg_location = OperandForArg(int(arg_index), x86_64_arg_size);
    arg_infos.push_back(ArgInfo{
        .x86_64_arg_value = x86_64_arg_value,
        .x86_64_arg_location = x86_64_arg_location,
    });
  }

  struct ResultInfo {
    x86_64::RM x86_64_result;
    x86_64::RM x86_64_result_location;
  };
  std::vector<ResultInfo> result_infos;
  result_infos.reserve(ir_call_instr->results().size());
  for (std::size_t result_index = 0; result_index < ir_call_instr->results().size();
       result_index++) {
    ir::Computed* ir_result = ir_call_instr->results().at(result_index).get();
    x86_64::RM x86_64_result = TranslateComputed(ir_result, ir_calling_func);
    x86_64::Size x86_64_result_size = TranslateSizeOfType(ir_result->type());
    x86_64::RM x86_64_result_location = OperandForResult(int(result_index), x86_64_result_size);
    result_infos.push_back(ResultInfo{
        .x86_64_result = x86_64_result,
        .x86_64_result_location = x86_64_result_location,
    });
  }

  for (ArgInfo& arg_info : arg_infos) {
    if (arg_info.x86_64_arg_location != arg_info.x86_64_arg_value) {
      x86_64_block_builder.AddInstr<x86_64::Mov>(arg_info.x86_64_arg_location,
                                                 arg_info.x86_64_arg_value);
    }
  }

  ir::Value* ir_called_func = ir_call_instr->func().get();
  x86_64::Operand x86_64_called_func = TranslateValue(ir_called_func, ir_calling_func);
  if (x86_64_called_func.is_func_ref()) {
    x86_64_block_builder.AddInstr<x86_64::Call>(x86_64_called_func.func_ref());
  } else if (x86_64_called_func.is_rm()) {
    x86_64_block_builder.AddInstr<x86_64::Call>(x86_64_called_func.rm());
  } else {
    common::fail("unexpected func operand");
  }

  for (ResultInfo& result_info : result_infos) {
    if (result_info.x86_64_result_location != result_info.x86_64_result) {
      x86_64_block_builder.AddInstr<x86_64::Mov>(result_info.x86_64_result,
                                                 result_info.x86_64_result_location);
    }
  }

  // TODO: improve
}

void IRTranslator::TranslateReturnInstr(ir::ReturnInstr* ir_return_instr, ir::Func* ir_func,
                                        x86_64::BlockBuilder& x86_64_block_builder) {
  struct ArgInfo {
    x86_64::Operand x86_64_arg_value;
    x86_64::RM x86_64_arg_location;
  };
  std::vector<ArgInfo> arg_infos;
  arg_infos.reserve(ir_return_instr->args().size());
  for (std::size_t arg_index = 0; arg_index < ir_return_instr->args().size(); arg_index++) {
    ir::Value* ir_arg_value = ir_return_instr->args().at(arg_index).get();
    x86_64::Operand x86_64_arg_value = TranslateValue(ir_arg_value, ir_func);
    x86_64::Size x86_64_arg_size = TranslateSizeOfType(ir_arg_value->type());
    x86_64::RM x86_64_arg_location = OperandForResult(int(arg_index), x86_64_arg_size);
    arg_infos.push_back(ArgInfo{
        .x86_64_arg_value = x86_64_arg_value,
        .x86_64_arg_location = x86_64_arg_location,
    });
  }

  for (ArgInfo& arg_info : arg_infos) {
    if (arg_info.x86_64_arg_location != arg_info.x86_64_arg_value) {
      x86_64_block_builder.AddInstr<x86_64::Mov>(arg_info.x86_64_arg_location,
                                                 arg_info.x86_64_arg_value);
    }
  }

  GenerateFuncEpilogue(ir_func, x86_64_block_builder);

  // TODO: improve
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

x86_64::Operand IRTranslator::TranslateFuncConstant(ir::FuncConstant* constant) {
  if (constant == ir::NilFunc().get()) {
    return x86_64::Imm(int32_t{0});
  }
  return x86_64::FuncRef(constant->value() +
                         x86_64_program_builder_.program()->declared_funcs().size());
}

x86_64::RM IRTranslator::TranslateComputed(ir::Computed* computed, ir::Func* ir_func) {
  ir_info::InterferenceGraphColors& colors = interference_graph_colors_.at(ir_func->number());
  ir_info::color_t color = colors.GetColor(computed->number());
  int8_t ir_size = static_cast<const ir::AtomicType*>(computed->type())->bit_size();
  x86_64::Size x86_64_size = x86_64::Size(ir_size);
  return ColorAndSizeToOperand(color, x86_64_size);
}

x86_64::BlockRef IRTranslator::TranslateBlockValue(ir::block_num_t block_value) {
  return x86_64::BlockRef(block_value);
}

x86_64::Size IRTranslator::TranslateSizeOfType(const ir::Type* ir_type) {
  switch (ir_type->type_kind()) {
    case ir::TypeKind::kBool:
      return x86_64::Size::k8;
    case ir::TypeKind::kInt:
      return TranslateSizeOfIntType(static_cast<const ir::IntType*>(ir_type));
    case ir::TypeKind::kPointer:
    case ir::TypeKind::kFunc:
      return x86_64::Size::k64;
    default:
      common::fail("unexpected int binary operand type");
  }
}

x86_64::Size IRTranslator::TranslateSizeOfIntType(const ir::IntType* ir_int_type) {
  return TranslateSizeOfIntType(ir_int_type->int_type());
}

x86_64::Size IRTranslator::TranslateSizeOfIntType(common::IntType common_int_type) {
  return x86_64::Size(common::BitSizeOf(common_int_type));
}

}  // namespace x86_64_ir_translator
