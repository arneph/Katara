//
//  arithmetic_logic_instrs_translator.cc
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "arithmetic_logic_instrs_translator.h"

#include <memory>
#include <optional>
#include <utility>

#include "src/common/logging.h"
#include "src/ir/representation/values.h"
#include "src/x86_64/instrs/arithmetic_logic_instrs.h"
#include "src/x86_64/instrs/data_instrs.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/instrs/instr_cond.h"
#include "src/x86_64/ir_translator/mov_generator.h"
#include "src/x86_64/ir_translator/size_translator.h"
#include "src/x86_64/ir_translator/temporary_reg.h"
#include "src/x86_64/ir_translator/value_translator.h"
#include "src/x86_64/ops.h"

namespace ir_to_x86_64_translator {

void TranslateBoolNotInstr(ir::BoolNotInstr* ir_bool_not_instr, BlockContext& ctx) {
  auto ir_result = ir_bool_not_instr->result().get();
  auto ir_operand = ir_bool_not_instr->operand().get();

  GenerateMov(ir_result, ir_operand, ir_bool_not_instr, ctx);

  x86_64::RM x86_64_operand = TranslateComputed(ir_result, ctx.func_ctx());

  ctx.x86_64_block()->AddInstr<x86_64::Not>(x86_64_operand);
}

void TranslateBoolBinaryInstr(ir::BoolBinaryInstr* ir_bool_binary_instr, BlockContext& ctx) {
  switch (ir_bool_binary_instr->operation()) {
    case common::Bool::BinaryOp::kEq:
    case common::Bool::BinaryOp::kNeq:
      TranslateBoolCompareInstr(ir_bool_binary_instr, ctx);
      break;
    case common::Bool::BinaryOp::kAnd:
    case common::Bool::BinaryOp::kOr:
      TranslateBoolLogicInstr(ir_bool_binary_instr, ctx);
      break;
  }
}

void TranslateBoolCompareInstr(ir::BoolBinaryInstr* ir_bool_compare_instr, BlockContext& ctx) {
  // Note: It is assumed that neither operand is a constant. A constant folding optimization pass
  // should ensure this.
  auto ir_result = ir_bool_compare_instr->result().get();
  auto ir_operand_a = static_cast<ir::Computed*>(ir_bool_compare_instr->operand_a().get());
  auto ir_operand_b = static_cast<ir::Computed*>(ir_bool_compare_instr->operand_b().get());

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

  x86_64::RM x86_64_result = TranslateComputed(ir_result, ctx.func_ctx());
  x86_64::RM x86_64_operand_a = TranslateComputed(ir_operand_a, ctx.func_ctx());
  x86_64::RM x86_64_operand_b = TranslateComputed(ir_operand_b, ctx.func_ctx());

  std::optional<TemporaryReg> tmp;
  if (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem()) {
    tmp = TemporaryReg::Prepare(x86_64::Size::k8, /*can_be_result_reg=*/true, ir_bool_compare_instr,
                                ctx);
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(tmp->reg(), x86_64_operand_b);
    x86_64_operand_b = tmp->reg();
  }

  ctx.x86_64_block()->AddInstr<x86_64::Cmp>(x86_64_operand_a, x86_64_operand_b);
  ctx.x86_64_block()->AddInstr<x86_64::Setcc>(x86_64_cond, x86_64_result);

  if (tmp.has_value()) {
    tmp->Restore(ctx);
  }
}

void TranslateBoolLogicInstr(ir::BoolBinaryInstr* ir_bool_logic_instr, BlockContext& ctx) {
  // Note: It is assumed that neither operand is a constant. A constant folding optimization pass
  // should ensure this.
  auto ir_result = ir_bool_logic_instr->result().get();
  auto ir_operand_a = static_cast<ir::Computed*>(ir_bool_logic_instr->operand_a().get());
  auto ir_operand_b = static_cast<ir::Computed*>(ir_bool_logic_instr->operand_b().get());

  GenerateMov(ir_result, ir_operand_a, ir_bool_logic_instr, ctx);

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_result, ctx.func_ctx());
  x86_64::RM x86_64_operand_b = TranslateComputed(ir_operand_b, ctx.func_ctx());

  std::optional<TemporaryReg> tmp;
  if (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem()) {
    tmp = TemporaryReg::Prepare(x86_64::Size::k8, /*can_be_result_reg=*/false, ir_bool_logic_instr,
                                ctx);
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(tmp->reg(), x86_64_operand_b);
    x86_64_operand_b = tmp->reg();
  }

  switch (ir_bool_logic_instr->operation()) {
    case common::Bool::BinaryOp::kAnd:
      ctx.x86_64_block()->AddInstr<x86_64::And>(x86_64_operand_a, x86_64_operand_b);
      break;
    case common::Bool::BinaryOp::kOr:
      ctx.x86_64_block()->AddInstr<x86_64::Or>(x86_64_operand_a, x86_64_operand_b);
      break;
    default:
      common::fail("unexpexted bool logic op");
  }

  if (tmp.has_value()) {
    tmp->Restore(ctx);
  }
}

void TranslateIntUnaryInstr(ir::IntUnaryInstr* ir_int_unary_instr, BlockContext& ctx) {
  // Note: It is assumed that the operand is not a constant. A constant folding optimization pass
  // should ensure this.
  auto ir_result = ir_int_unary_instr->result().get();
  auto ir_operand_a = static_cast<ir::Computed*>(ir_int_unary_instr->operand().get());

  GenerateMov(ir_result, ir_operand_a, ir_int_unary_instr, ctx);

  x86_64::RM x86_64_operand = TranslateComputed(ir_result, ctx.func_ctx());

  switch (ir_int_unary_instr->operation()) {
    case common::Int::UnaryOp::kNot:
      ctx.x86_64_block()->AddInstr<x86_64::Not>(x86_64_operand);
      break;
    case common::Int::UnaryOp::kNeg:
      ctx.x86_64_block()->AddInstr<x86_64::Neg>(x86_64_operand);
      break;
    default:
      common::fail("unexpected unary int op");
  }
}

void TranslateIntCompareInstr(ir::IntCompareInstr* ir_int_compare_instr, BlockContext& ctx) {
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

  x86_64::RM x86_64_result = TranslateComputed(ir_result, ctx.func_ctx());
  x86_64::RM x86_64_operand_a =
      TranslateComputed(static_cast<ir::Computed*>(ir_operand_a), ctx.func_ctx());
  x86_64::Operand x86_64_operand_b = TranslateValue(ir_operand_b, ctx.func_ctx());

  std::optional<TemporaryReg> tmp;
  if ((x86_64_operand_b.is_imm() && x86_64_operand_b.size() == x86_64::k64) ||
      (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem())) {
    x86_64::Size x86_64_size = x86_64::Size(common::BitSizeOf(ir_type->int_type()));
    tmp = TemporaryReg::Prepare(x86_64_size, /*can_be_result_reg=*/true, ir_int_compare_instr, ctx);
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(tmp->reg(), x86_64_operand_b);
    x86_64_operand_b = tmp->reg();
  }

  ctx.x86_64_block()->AddInstr<x86_64::Cmp>(x86_64_operand_a, x86_64_operand_b);
  ctx.x86_64_block()->AddInstr<x86_64::Setcc>(x86_64_cond, x86_64_result);

  if (tmp.has_value()) {
    tmp->Restore(ctx);
  }
}

void TranslateIntBinaryInstr(ir::IntBinaryInstr* ir_int_binary_instr, BlockContext& ctx) {
  switch (ir_int_binary_instr->operation()) {
    case common::Int::BinaryOp::kAdd:
    case common::Int::BinaryOp::kSub:
    case common::Int::BinaryOp::kAnd:
    case common::Int::BinaryOp::kOr:
    case common::Int::BinaryOp::kXor:
      TranslateIntSimpleALInstr(ir_int_binary_instr, ctx);
      break;
    case common::Int::BinaryOp::kAndNot:
      common::fail("int andnot operation was not decomposed into separate instrs");
    case common::Int::BinaryOp::kMul:
      TranslateIntMulInstr(ir_int_binary_instr, ctx);
      break;
    case common::Int::BinaryOp::kDiv:
    case common::Int::BinaryOp::kRem:
      TranslateIntDivOrRemInstr(ir_int_binary_instr, ctx);
      break;
  }
}

void TranslateIntSimpleALInstr(ir::IntBinaryInstr* ir_int_binary_instr, BlockContext& ctx) {
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

  GenerateMov(ir_result, ir_operand_a, ir_int_binary_instr, ctx);

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_result, ctx.func_ctx());
  x86_64::Operand x86_64_operand_b = TranslateValue(ir_operand_b, ctx.func_ctx());

  std::optional<TemporaryReg> tmp;
  if ((x86_64_operand_b.is_imm() && x86_64_operand_b.size() == x86_64::k64) ||
      (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem())) {
    x86_64::Size x86_64_size = x86_64::Size(common::BitSizeOf(ir_type->int_type()));
    tmp = TemporaryReg::Prepare(x86_64_size, /*can_be_result_reg=*/false, ir_int_binary_instr, ctx);
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(tmp->reg(), x86_64_operand_b);
    x86_64_operand_b = tmp->reg();
  }

  switch (op) {
    case common::Int::BinaryOp::kAdd:
      ctx.x86_64_block()->AddInstr<x86_64::Add>(x86_64_operand_a, x86_64_operand_b);
      break;
    case common::Int::BinaryOp::kSub:
      ctx.x86_64_block()->AddInstr<x86_64::Sub>(x86_64_operand_a, x86_64_operand_b);
      break;
    case common::Int::BinaryOp::kAnd:
      ctx.x86_64_block()->AddInstr<x86_64::And>(x86_64_operand_a, x86_64_operand_b);
      break;
    case common::Int::BinaryOp::kOr:
      ctx.x86_64_block()->AddInstr<x86_64::Or>(x86_64_operand_a, x86_64_operand_b);
      break;
    case common::Int::BinaryOp::kXor:
      ctx.x86_64_block()->AddInstr<x86_64::Xor>(x86_64_operand_a, x86_64_operand_b);
      break;
    default:
      common::fail("unexpexted simple int binary operation");
  }

  if (tmp.has_value()) {
    tmp->Restore(ctx);
  }
}

void TranslateIntMulInstr(ir::IntBinaryInstr* ir_int_binary_instr, BlockContext& ctx) {
  // Note: It is assumed that at least one operand is not a constant. A constant folding
  // optimization pass should ensure this.
  auto ir_result = ir_int_binary_instr->result().get();
  auto ir_operand_a = ir_int_binary_instr->operand_a().get();
  auto ir_operand_b = ir_int_binary_instr->operand_b().get();
  auto ir_type = static_cast<const ir::IntType*>(ir_result->type());

  if (ir_operand_a->kind() == ir::Value::Kind::kConstant) {
    std::swap(ir_operand_a, ir_operand_b);
  }

  x86_64::RM x86_64_result = TranslateComputed(ir_result, ctx.func_ctx());
  x86_64::RM x86_64_operand_a =
      TranslateComputed(static_cast<ir::Computed*>(ir_operand_a), ctx.func_ctx());
  x86_64::Operand x86_64_operand_b = TranslateValue(ir_operand_b, ctx.func_ctx());

  std::optional<TemporaryReg> tmp;
  if ((x86_64_operand_b.is_imm() && x86_64_operand_b.size() == x86_64::k64) ||
      (!x86_64_result.is_reg() && x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem())) {
    x86_64::Size x86_64_size = TranslateSizeOfIntType(ir_type);
    tmp = TemporaryReg::Prepare(x86_64_size, /*can_be_result_reg=*/true, ir_int_binary_instr, ctx);
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(tmp->reg(), x86_64_operand_b);
    x86_64_operand_b = tmp->reg();
  }

  if (x86_64_result.is_reg()) {
    if (x86_64_operand_b.is_imm()) {
      ctx.x86_64_block()->AddInstr<x86_64::Imul>(x86_64_result.reg(), x86_64_operand_a,
                                                 x86_64_operand_b.imm());
    } else {
      if (x86_64_result != x86_64_operand_b) {
        ctx.x86_64_block()->AddInstr<x86_64::Mov>(x86_64_result.reg(), x86_64_operand_b);
      }
      ctx.x86_64_block()->AddInstr<x86_64::Imul>(x86_64_result.reg(), x86_64_operand_a);
    }

  } else {
    ctx.x86_64_block()->AddInstr<x86_64::Imul>(tmp->reg(), x86_64_operand_a);
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(x86_64_result, tmp->reg());
  }
  if (tmp.has_value()) {
    tmp->Restore(ctx);
  }
}

void TranslateIntDivOrRemInstr(ir::IntBinaryInstr* ir_int_binary_instr, BlockContext& ctx) {
  // TODO: implement
}

void TranslateIntShiftInstr(ir::IntShiftInstr* ir_int_shift_instr, BlockContext& ctx) {
  // TODO: implement
}

void TranslatePointerOffsetInstr(ir::PointerOffsetInstr* ir_pointer_offset_instr,
                                 BlockContext& ctx) {
  // Note: It is assumed that at least one operand is not a constant. A constant folding
  // optimization pass should ensure this.
  auto ir_result = ir_pointer_offset_instr->result().get();
  auto ir_pointer = ir_pointer_offset_instr->pointer().get();
  auto ir_offset = ir_pointer_offset_instr->offset().get();

  GenerateMov(ir_result, ir_pointer, ir_pointer_offset_instr, ctx);

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_result, ctx.func_ctx());
  x86_64::Operand x86_64_operand_b = TranslateValue(ir_offset, ctx.func_ctx());

  std::optional<TemporaryReg> tmp;
  if ((x86_64_operand_b.is_imm() && x86_64_operand_b.size() == x86_64::k64) ||
      (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem())) {
    tmp = TemporaryReg::Prepare(x86_64::Size::k64, /*can_be_result_reg=*/false,
                                ir_pointer_offset_instr, ctx);
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(tmp->reg(), x86_64_operand_b);
    x86_64_operand_b = tmp->reg();
  }

  ctx.x86_64_block()->AddInstr<x86_64::Add>(x86_64_operand_a, x86_64_operand_b);

  if (tmp.has_value()) {
    tmp->Restore(ctx);
  }
}

void TranslateNilTestInstr(ir::NilTestInstr* ir_nil_test_instr, BlockContext& ctx) {
  // Note: It is assumed that the tested operand is not constant. A constant folding optimization
  // pass should ensure this.
  auto ir_result = ir_nil_test_instr->result().get();
  auto ir_tested = ir_nil_test_instr->tested().get();

  x86_64::RM x86_64_result = TranslateComputed(ir_result, ctx.func_ctx());
  x86_64::RM x86_64_tested =
      TranslateComputed(static_cast<ir::Computed*>(ir_tested), ctx.func_ctx());

  ctx.x86_64_block()->AddInstr<x86_64::Cmp>(x86_64_tested, x86_64::Imm(0));
  ctx.x86_64_block()->AddInstr<x86_64::Setcc>(x86_64::InstrCond::kEqual, x86_64_result);
}

}  // namespace ir_to_x86_64_translator
