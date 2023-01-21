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

#include "src/common/logging/logging.h"
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

using ::common::atomics::Bool;
using ::common::atomics::Int;
using ::common::logging::fail;

void TranslateBoolNotInstr(ir::BoolNotInstr* ir_bool_not_instr, BlockContext& ctx) {
  x86_64::RM x86_64_result = TranslateComputed(ir_bool_not_instr->result().get(), ctx.func_ctx());
  x86_64::Operand x86_64_operand =
      TranslateValue(ir_bool_not_instr->operand().get(), IntNarrowing::kNone, ctx.func_ctx());

  GenerateMov(x86_64_result, x86_64_operand, ir_bool_not_instr, ctx);

  ctx.x86_64_block()->AddInstr<x86_64::Not>(x86_64_result);
}

void TranslateBoolBinaryInstr(ir::BoolBinaryInstr* ir_bool_binary_instr, BlockContext& ctx) {
  switch (ir_bool_binary_instr->operation()) {
    case Bool::BinaryOp::kEq:
    case Bool::BinaryOp::kNeq:
      TranslateBoolCompareInstr(ir_bool_binary_instr, ctx);
      break;
    case Bool::BinaryOp::kAnd:
    case Bool::BinaryOp::kOr:
      TranslateBoolLogicInstr(ir_bool_binary_instr, ctx);
      break;
  }
}

void TranslateBoolCompareInstr(ir::BoolBinaryInstr* ir_bool_compare_instr, BlockContext& ctx) {
  // Note: It is assumed that neither operand is a constant. A constant folding optimization pass
  // should ensure this.
  x86_64::InstrCond x86_64_cond = [](Bool::BinaryOp op) {
    switch (op) {
      case Bool::BinaryOp::kEq:
        return x86_64::InstrCond::kEqual;
      case Bool::BinaryOp::kNeq:
        return x86_64::InstrCond::kNotEqual;
      default:
        fail("unexpected bool compare op");
    }
  }(ir_bool_compare_instr->operation());
  x86_64::RM x86_64_result =
      TranslateComputed(ir_bool_compare_instr->result().get(), ctx.func_ctx());
  x86_64::RM x86_64_operand_a = TranslateComputed(
      static_cast<ir::Computed*>(ir_bool_compare_instr->operand_a().get()), ctx.func_ctx());
  x86_64::RM x86_64_operand_b = TranslateComputed(
      static_cast<ir::Computed*>(ir_bool_compare_instr->operand_b().get()), ctx.func_ctx());

  std::optional<TemporaryReg> tmp;
  if (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem()) {
    tmp = TemporaryReg::ForOperand(x86_64_operand_b, /*can_be_result_reg=*/true,
                                   ir_bool_compare_instr, ctx);
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
  x86_64::RM x86_64_result = TranslateComputed(ir_bool_logic_instr->result().get(), ctx.func_ctx());
  x86_64::RM x86_64_operand_a = TranslateComputed(
      static_cast<ir::Computed*>(ir_bool_logic_instr->operand_a().get()), ctx.func_ctx());
  x86_64::RM x86_64_operand_b = TranslateComputed(
      static_cast<ir::Computed*>(ir_bool_logic_instr->operand_b().get()), ctx.func_ctx());

  if (x86_64_result == x86_64_operand_b) {
    x86_64_operand_b = x86_64_operand_a;
  } else {
    GenerateMov(x86_64_result, x86_64_operand_a, ir_bool_logic_instr, ctx);
  }

  std::optional<TemporaryReg> tmp;
  if (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem()) {
    tmp = TemporaryReg::ForOperand(x86_64_operand_b, /*can_be_result_reg=*/false,
                                   ir_bool_logic_instr, ctx);
    x86_64_operand_b = tmp->reg();
  }

  switch (ir_bool_logic_instr->operation()) {
    case Bool::BinaryOp::kAnd:
      ctx.x86_64_block()->AddInstr<x86_64::And>(x86_64_result, x86_64_operand_b);
      break;
    case Bool::BinaryOp::kOr:
      ctx.x86_64_block()->AddInstr<x86_64::Or>(x86_64_result, x86_64_operand_b);
      break;
    default:
      fail("unexpexted bool logic op");
  }

  if (tmp.has_value()) {
    tmp->Restore(ctx);
  }
}

void TranslateIntUnaryInstr(ir::IntUnaryInstr* ir_int_unary_instr, BlockContext& ctx) {
  // Note: It is assumed that the operand is not a constant. A constant folding optimization pass
  // should ensure this.
  x86_64::RM x86_64_result = TranslateComputed(ir_int_unary_instr->result().get(), ctx.func_ctx());
  x86_64::RM x86_64_operand = TranslateComputed(
      static_cast<ir::Computed*>(ir_int_unary_instr->operand().get()), ctx.func_ctx());

  GenerateMov(x86_64_result, x86_64_operand, ir_int_unary_instr, ctx);

  switch (ir_int_unary_instr->operation()) {
    case Int::UnaryOp::kNot:
      ctx.x86_64_block()->AddInstr<x86_64::Not>(x86_64_operand);
      break;
    case Int::UnaryOp::kNeg:
      ctx.x86_64_block()->AddInstr<x86_64::Neg>(x86_64_operand);
      break;
    default:
      fail("unexpected unary int op");
  }
}

void TranslateIntCompareInstr(ir::IntCompareInstr* ir_int_compare_instr, BlockContext& ctx) {
  // Note: It is assumed that at least one operand is not a constant. A constant folding
  // optimization pass should ensure this.
  Int::CompareOp op = ir_int_compare_instr->operation();
  auto ir_result = ir_int_compare_instr->result().get();
  auto ir_operand_a = ir_int_compare_instr->operand_a().get();
  auto ir_operand_b = ir_int_compare_instr->operand_b().get();
  auto ir_type = static_cast<const ir::IntType*>(ir_operand_a->type());
  bool is_signed = common::atomics::IsSigned(ir_type->int_type());

  if (ir_operand_a->kind() == ir::Value::Kind::kConstant) {
    op = common::atomics::Flipped(op);
    std::swap(ir_operand_a, ir_operand_b);
  }

  x86_64::InstrCond x86_64_cond = [op, is_signed]() {
    switch (op) {
      case Int::CompareOp::kEq:
        return x86_64::InstrCond::kEqual;
      case Int::CompareOp::kNeq:
        return x86_64::InstrCond::kNotEqual;
      case Int::CompareOp::kLss:
        return is_signed ? x86_64::InstrCond::kLess : x86_64::InstrCond::kBelow;
      case Int::CompareOp::kLeq:
        return is_signed ? x86_64::InstrCond::kLessOrEqual : x86_64::InstrCond::kBelowOrEqual;
      case Int::CompareOp::kGeq:
        return is_signed ? x86_64::InstrCond::kGreaterOrEqual : x86_64::InstrCond::kAboveOrEqual;
      case Int::CompareOp::kGtr:
        return is_signed ? x86_64::InstrCond::kGreater : x86_64::InstrCond::kAbove;
    }
  }();

  x86_64::RM x86_64_result = TranslateComputed(ir_result, ctx.func_ctx());
  x86_64::RM x86_64_operand_a =
      TranslateComputed(static_cast<ir::Computed*>(ir_operand_a), ctx.func_ctx());
  x86_64::Operand x86_64_operand_b =
      TranslateValue(ir_operand_b, IntNarrowing::k64To32BitIfPossible, ctx.func_ctx());

  std::optional<TemporaryReg> tmp;
  if ((x86_64_operand_b.is_imm() && x86_64_operand_b.size() == x86_64::k64) ||
      (x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem())) {
    tmp = TemporaryReg::ForOperand(x86_64_operand_b, /*can_be_result_reg=*/true,
                                   ir_int_compare_instr, ctx);
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
    case Int::BinaryOp::kAdd:
    case Int::BinaryOp::kAnd:
    case Int::BinaryOp::kOr:
    case Int::BinaryOp::kXor:
      TranslateIntCommutativeALInstr(ir_int_binary_instr, ctx);
      break;
    case Int::BinaryOp::kSub:
      TranslateIntSubInstr(ir_int_binary_instr, ctx);
      break;
    case Int::BinaryOp::kMul:
      TranslateIntMulInstr(ir_int_binary_instr, ctx);
      break;
    case Int::BinaryOp::kDiv:
    case Int::BinaryOp::kRem:
      TranslateIntDivOrRemInstr(ir_int_binary_instr, ctx);
      break;
    case Int::BinaryOp::kAndNot:
      fail("int andnot operation was not decomposed into separate instrs");
  }
}

void TranslateIntCommutativeALInstr(ir::IntBinaryInstr* ir_int_binary_instr, BlockContext& ctx) {
  // Note: It is assumed that at least one operand is not a constant. A constant folding
  // optimization pass should ensure this.
  x86_64::RM x86_64_result = TranslateComputed(ir_int_binary_instr->result().get(), ctx.func_ctx());
  x86_64::Operand x86_64_operand_a = TranslateValue(
      ir_int_binary_instr->operand_a().get(), IntNarrowing::k64To32BitIfPossible, ctx.func_ctx());
  x86_64::Operand x86_64_operand_b = TranslateValue(
      ir_int_binary_instr->operand_b().get(), IntNarrowing::k64To32BitIfPossible, ctx.func_ctx());

  if (x86_64_operand_a.is_imm()) {
    std::swap(x86_64_operand_a, x86_64_operand_b);
  }

  if (x86_64_result == x86_64_operand_b) {
    x86_64_operand_b = x86_64_operand_a;
  } else {
    GenerateMov(x86_64_result, x86_64_operand_a, ir_int_binary_instr, ctx);
  }

  std::optional<TemporaryReg> tmp;
  if ((x86_64_operand_b.is_imm() && x86_64_operand_b.size() == x86_64::k64) ||
      (x86_64_operand_b.is_mem() && x86_64_result.is_mem())) {
    tmp = TemporaryReg::ForOperand(x86_64_operand_b, /*can_be_result_reg=*/false,
                                   ir_int_binary_instr, ctx);
    x86_64_operand_b = tmp->reg();
  }

  switch (ir_int_binary_instr->operation()) {
    case Int::BinaryOp::kAdd:
      ctx.x86_64_block()->AddInstr<x86_64::Add>(x86_64_result, x86_64_operand_b);
      break;
    case Int::BinaryOp::kAnd:
      ctx.x86_64_block()->AddInstr<x86_64::And>(x86_64_result, x86_64_operand_b);
      break;
    case Int::BinaryOp::kOr:
      ctx.x86_64_block()->AddInstr<x86_64::Or>(x86_64_result, x86_64_operand_b);
      break;
    case Int::BinaryOp::kXor:
      ctx.x86_64_block()->AddInstr<x86_64::Xor>(x86_64_result, x86_64_operand_b);
      break;
    default:
      fail("unexpexted simple int binary operation");
  }

  if (tmp.has_value()) {
    tmp->Restore(ctx);
  }
}

void TranslateIntSubInstr(ir::IntBinaryInstr* ir_int_binary_instr, BlockContext& ctx) {
  x86_64::RM x86_64_result = TranslateComputed(ir_int_binary_instr->result().get(), ctx.func_ctx());
  x86_64::Operand x86_64_operand_a = TranslateValue(
      ir_int_binary_instr->operand_a().get(), IntNarrowing::k64To32BitIfPossible, ctx.func_ctx());
  x86_64::Operand x86_64_operand_b = TranslateValue(
      ir_int_binary_instr->operand_b().get(), IntNarrowing::k64To32BitIfPossible, ctx.func_ctx());

  std::optional<TemporaryReg> tmp;
  if (x86_64_result == x86_64_operand_b) {
    x86_64::Reg r = x86_64::rax;
    if (x86_64_operand_a.is_reg() &&
        ir_int_binary_instr ==
            ctx.live_ranges().LastValueUseOf(
                static_cast<ir::Computed*>(ir_int_binary_instr->operand_a().get())->number())) {
      r = x86_64_operand_a.reg();
    } else {
      tmp = TemporaryReg::ForOperand(x86_64_operand_a, /*can_be_result_reg=*/false,
                                     ir_int_binary_instr, ctx);
      r = tmp->reg();
    }
    ctx.x86_64_block()->AddInstr<x86_64::Sub>(r, x86_64_operand_b);
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(x86_64_result, r);

  } else {
    GenerateMov(x86_64_result, x86_64_operand_a, ir_int_binary_instr, ctx);

    if ((x86_64_operand_b.is_imm() && x86_64_operand_b.size() == x86_64::k64) ||
        (x86_64_operand_b.is_mem() && x86_64_result.is_mem())) {
      tmp = TemporaryReg::ForOperand(x86_64_operand_b, /*can_be_result_reg=*/false,
                                     ir_int_binary_instr, ctx);
      x86_64_operand_b = tmp->reg();
    }

    ctx.x86_64_block()->AddInstr<x86_64::Sub>(x86_64_result, x86_64_operand_b);
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

  if (ir_operand_a->kind() == ir::Value::Kind::kConstant) {
    std::swap(ir_operand_a, ir_operand_b);
  }

  x86_64::RM x86_64_result = TranslateComputed(ir_result, ctx.func_ctx());
  x86_64::RM x86_64_operand_a =
      TranslateComputed(static_cast<ir::Computed*>(ir_operand_a), ctx.func_ctx());
  x86_64::Operand x86_64_operand_b =
      TranslateValue(ir_operand_b, IntNarrowing::k64To32BitIfPossible, ctx.func_ctx());

  std::optional<TemporaryReg> tmp;
  if ((x86_64_operand_b.is_imm() && x86_64_operand_b.size() == x86_64::k64) ||
      (!x86_64_result.is_reg() && x86_64_operand_a.is_mem() && x86_64_operand_b.is_mem())) {
    tmp = TemporaryReg::ForOperand(x86_64_operand_b, /*can_be_result_reg=*/true,
                                   ir_int_binary_instr, ctx);
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
  x86_64::RM x86_64_result =
      TranslateComputed(ir_pointer_offset_instr->result().get(), ctx.func_ctx());
  x86_64::Operand x86_64_operand_a = TranslateValue(
      ir_pointer_offset_instr->pointer().get(), IntNarrowing::k64To32BitIfPossible, ctx.func_ctx());
  x86_64::Operand x86_64_operand_b = TranslateValue(
      ir_pointer_offset_instr->offset().get(), IntNarrowing::k64To32BitIfPossible, ctx.func_ctx());

  if (x86_64_result == x86_64_operand_b) {
    x86_64_operand_b = x86_64_operand_a;
  } else {
    GenerateMov(x86_64_result, x86_64_operand_a, ir_pointer_offset_instr, ctx);
  }

  std::optional<TemporaryReg> tmp;
  if ((x86_64_operand_b.is_imm() && x86_64_operand_b.size() == x86_64::k64) ||
      (x86_64_operand_a.is_mem() && x86_64_result.is_mem())) {
    tmp = TemporaryReg::ForOperand(x86_64_operand_b, /*can_be_result_reg=*/false,
                                   ir_pointer_offset_instr, ctx);
    x86_64_operand_b = tmp->reg();
  }

  ctx.x86_64_block()->AddInstr<x86_64::Add>(x86_64_result, x86_64_operand_b);

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
