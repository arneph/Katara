//
//  mov_generator_test.cc
//  Katara
//
//  Created by Arne Philipeit on 8/13/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "src/x86_64/ir_translator/mov_generator.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/common/logging/logging.h"
#include "src/ir/analyzers/interference_graph_builder.h"
#include "src/ir/analyzers/live_range_analyzer.h"
#include "src/ir/builder/block_builder.h"
#include "src/ir/builder/func_builder.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/x86_64/block.h"
#include "src/x86_64/func.h"
#include "src/x86_64/instrs/data_instrs.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/ir_translator/testing/instr_translator_test_setup.h"
#include "src/x86_64/ir_translator/value_translator.h"
#include "src/x86_64/ops.h"
#include "src/x86_64/program.h"

namespace ir_to_x86_64_translator {

using ::common::atomics::Int;
using ::common::logging::fail;
using ::testing::Contains;

class GenerateMovsTest : public InstrTranslatorTest {
 protected:
  static void CheckMem(x86_64::Mem mem) {
    ASSERT_THAT(mem.base_reg(), x86_64::rbp.reg());
    ASSERT_THAT(mem.index_reg(), 0xff);
    ASSERT_THAT(mem.scale(), x86_64::Scale::kS00);
    ASSERT_THAT(mem.disp() % 8, 0);
  }

  void CheckGeneratedInstrsForMoveOperations(std::vector<MoveOperation> operations) {
    typedef int64_t value_t;
    int64_t value_count = 0;
    // Prepare record keeping:
    std::unordered_map<int64_t, value_t> constants;
    std::unordered_map<int8_t, value_t> original_reg_values;
    std::unordered_map<int32_t, value_t> original_mem_values;
    std::vector<value_t> stack;
    std::unordered_set<int8_t> aux_regs;
    std::unordered_set<int32_t> aux_mems;
    std::unordered_map<int8_t, value_t> reg_values;
    std::unordered_map<int32_t, value_t> mem_values;
    for (MoveOperation& op : operations) {
      if (op.origin().is_imm()) {
        constants[op.origin().imm().value()] = value_t(value_count++);
      } else if (op.origin().is_reg()) {
        original_reg_values[op.origin().reg().reg()] = value_t(value_count++);
        reg_values[op.origin().reg().reg()] = original_reg_values.at(op.origin().reg().reg());
      } else if (op.origin().is_mem()) {
        CheckMem(op.origin().mem());
        original_mem_values[op.origin().mem().disp()] = value_t(value_count++);
        mem_values[op.origin().mem().disp()] = original_mem_values.at(op.origin().mem().disp());
      }
    }

    auto read = [&](x86_64::Operand src, bool allow_unknown = false) -> value_t {
      if (src.is_imm()) {
        return constants.at(src.imm().value());
      } else if (src.is_reg()) {
        if (!reg_values.contains(src.reg().reg())) {
          if (!allow_unknown) {
            ADD_FAILURE() << "Reading forbidden value from " << src.reg().ToString();
          }
          original_reg_values[src.reg().reg()] = value_t(value_count++);
          reg_values[src.reg().reg()] = original_reg_values.at(src.reg().reg());
        }
        return reg_values.at(src.reg().reg());
      } else if (src.is_mem()) {
        CheckMem(src.mem());
        if (!mem_values.contains(src.mem().disp())) {
          if (!allow_unknown) {
            ADD_FAILURE() << "Reading forbidden value from " << src.mem().ToString();
          }
          original_mem_values[src.mem().disp()] = value_t(value_count++);
          reg_values[src.mem().disp()] = original_mem_values.at(src.mem().disp());
        }
        return mem_values.at(src.mem().disp());
      } else {
        fail("unexpect src kind");
      }
    };
    auto write = [&](x86_64::RM dst, value_t value) {
      if (dst.is_reg()) {
        reg_values[dst.reg().reg()] = value;
      } else if (dst.is_mem()) {
        CheckMem(dst.mem());
        mem_values[dst.mem().disp()] = value;
      } else {
        fail("unexpect dst kind");
      }
    };

    for (auto& instr : x86_64_block()->instrs()) {
      std::string instr_str = instr->ToString();
      std::cout << instr_str << "\n";
      if (instr_str.starts_with("mov")) {
        auto mov_instr = static_cast<x86_64::Mov*>(instr.get());
        write(mov_instr->dst(), read(mov_instr->src()));
      } else if (instr_str.starts_with("xchg")) {
        auto xchg_instr = static_cast<x86_64::Xchg*>(instr.get());
        value_t a = read(xchg_instr->op_a());
        value_t b = read(xchg_instr->op_b());
        write(xchg_instr->op_a(), b);
        write(xchg_instr->op_b(), a);
      } else if (instr_str.starts_with("push")) {
        auto push_instr = static_cast<x86_64::Push*>(instr.get());
        stack.push_back(read(push_instr->op(), /*allow_unknown=*/true));
      } else if (instr_str.starts_with("pop")) {
        auto pop_instr = static_cast<x86_64::Pop*>(instr.get());
        value_t value = stack.back();
        stack.pop_back();
        write(pop_instr->op(), value);
      }
    }

    EXPECT_TRUE(stack.empty());
    for (int8_t aux_reg : aux_regs) {
      EXPECT_EQ(original_reg_values.at(aux_reg), reg_values.at(aux_reg));
    }
    for (int32_t aux_mem : aux_mems) {
      EXPECT_EQ(original_mem_values.at(aux_mem), mem_values.at(aux_mem));
    }

    for (MoveOperation& op : operations) {
      value_t expected_value = 0;
      if (op.origin().is_imm()) {
        expected_value = constants.at(op.origin().imm().value());
      } else if (op.origin().is_reg()) {
        expected_value = original_reg_values.at(op.origin().reg().reg());
      } else if (op.origin().is_mem()) {
        expected_value = original_mem_values.at(op.origin().mem().disp());
      } else {
        fail("unexpected origin kind");
      }
      value_t actual_value = 0;
      if (op.result().is_reg()) {
        actual_value = reg_values.at(op.result().reg().reg());
      } else if (op.result().is_mem()) {
        actual_value = mem_values.at(op.result().mem().disp());
      } else {
        fail("unexpected result kind");
      }
      EXPECT_EQ(actual_value, expected_value) << "expected (value of) " << op.origin().ToString()
                                              << " to end up in " << op.result().ToString();
    }
  }
};

TEST_F(GenerateMovsTest, GeneratesNoInstrsForNoOps) {
  std::shared_ptr<ir::Computed> ir_operand_a = ir_func_builder().AddArg(ir::i64());
  std::shared_ptr<ir::Computed> ir_operand_b = ir_func_builder().AddArg(ir::bool_type());
  std::shared_ptr<ir::Computed> ir_operand_c = ir_func_builder().AddArg(ir::u8());

  ir_func_builder().AddResultType(ir::i64());
  ir_func_builder().AddResultType(ir::bool_type());
  ir_func_builder().AddResultType(ir::u8());

  ir_block_builder().Call(ir::NilFunc(), {}, {ir_operand_a, ir_operand_b, ir_operand_c});
  ir_block_builder().Return({ir_operand_a, ir_operand_b, ir_operand_c});

  ir::CallInstr* ir_call_instr = static_cast<ir::CallInstr*>(ir_block()->instrs().front().get());

  GenerateIRInfo();

  interference_graph_colors().SetColor(ir_operand_a->number(), 0);
  interference_graph_colors().SetColor(ir_operand_b->number(), 1);
  interference_graph_colors().SetColor(ir_operand_c->number(), 2);

  GenerateTranslationContexts();

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_operand_a.get(), func_ctx());
  x86_64::RM x86_64_operand_b = TranslateComputed(ir_operand_b.get(), func_ctx());
  x86_64::RM x86_64_operand_c = TranslateComputed(ir_operand_c.get(), func_ctx());

  GenerateMovs({MoveOperation(x86_64_operand_a, x86_64_operand_a),
                MoveOperation(x86_64_operand_b, x86_64_operand_b),
                MoveOperation(x86_64_operand_c, x86_64_operand_c)},
               ir_call_instr, block_ctx());

  EXPECT_TRUE(x86_64_block()->instrs().empty());
}

TEST_F(GenerateMovsTest, GeneratesInstrsForSimpleImmMoves) {
  const int64_t kA = 0x123456789;
  const int8_t kB = 0x47;
  const int16_t kC = 0x321;
  ir_block_builder().Call(
      ir::NilFunc(), {},
      {ir::ToIntConstant(Int(kA)), ir::ToIntConstant(Int(kB)), ir::ToIntConstant(Int(kC))});
  ir_block_builder().Return();

  ir::CallInstr* ir_call_instr = static_cast<ir::CallInstr*>(ir_block()->instrs().front().get());

  GenerateIRInfo();
  GenerateTranslationContexts();

  std::vector<MoveOperation> operations{MoveOperation(x86_64::rax, x86_64::Imm(kA)),
                                        MoveOperation(x86_64::cl, x86_64::Imm(kB)),
                                        MoveOperation(x86_64::dx, x86_64::Imm(kC))};
  GenerateMovs(operations, ir_call_instr, block_ctx());

  EXPECT_EQ(x86_64_block()->instrs().size(), 3);
  CheckGeneratedInstrsForMoveOperations(operations);
}

TEST_F(GenerateMovsTest, GeneratesInstrsForSimpleRegMoves) {
  std::shared_ptr<ir::Computed> ir_operand_a = ir_func_builder().AddArg(ir::i64());
  std::shared_ptr<ir::Computed> ir_operand_b = ir_func_builder().AddArg(ir::bool_type());
  std::shared_ptr<ir::Computed> ir_operand_c = ir_func_builder().AddArg(ir::u8());

  ir_func_builder().AddResultType(ir::i64());
  ir_func_builder().AddResultType(ir::bool_type());
  ir_func_builder().AddResultType(ir::u8());

  ir_block_builder().Call(ir::NilFunc(), {}, {ir_operand_a, ir_operand_b, ir_operand_c});
  ir_block_builder().Return({ir_operand_a, ir_operand_b, ir_operand_c});

  ir::CallInstr* ir_call_instr = static_cast<ir::CallInstr*>(ir_block()->instrs().front().get());

  GenerateIRInfo();

  interference_graph_colors().SetColor(ir_operand_a->number(), 0);
  interference_graph_colors().SetColor(ir_operand_b->number(), 1);
  interference_graph_colors().SetColor(ir_operand_c->number(), 2);

  GenerateTranslationContexts();

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_operand_a.get(), func_ctx());
  x86_64::RM x86_64_operand_b = TranslateComputed(ir_operand_b.get(), func_ctx());
  x86_64::RM x86_64_operand_c = TranslateComputed(ir_operand_c.get(), func_ctx());

  std::vector<MoveOperation> operations{MoveOperation(x86_64::r8, x86_64_operand_a),
                                        MoveOperation(x86_64::r9b, x86_64_operand_b),
                                        MoveOperation(x86_64::r10b, x86_64_operand_c)};

  GenerateMovs(operations, ir_call_instr, block_ctx());

  EXPECT_EQ(x86_64_block()->instrs().size(), 3);
  CheckGeneratedInstrsForMoveOperations(operations);
}

TEST_F(GenerateMovsTest, GeneratesInstrsForSimpleMemMoves) {
  // A: mem to reg
  // B: reg to mem
  // C: mem to mem
  // D: small imm to mem
  // E: large imm to mem
  const int16_t kD = 0x321;
  const int64_t kE = 0x123456789;
  std::shared_ptr<ir::Computed> ir_operand_a = ir_func_builder().AddArg(ir::i32());
  std::shared_ptr<ir::Computed> ir_operand_b = ir_func_builder().AddArg(ir::bool_type());
  std::shared_ptr<ir::Computed> ir_operand_c = ir_func_builder().AddArg(ir::u8());

  ir_func_builder().AddResultType(ir::i32());
  ir_func_builder().AddResultType(ir::bool_type());
  ir_func_builder().AddResultType(ir::u8());

  ir_block_builder().Call(ir::NilFunc(), {},
                          {ir_operand_a, ir_operand_b, ir_operand_c, ir::ToIntConstant(Int(kD)),
                           ir::ToIntConstant(Int(kE))});
  ir_block_builder().Return({ir_operand_a, ir_operand_b, ir_operand_c});

  ir::CallInstr* ir_call_instr = static_cast<ir::CallInstr*>(ir_block()->instrs().front().get());

  GenerateIRInfo();

  interference_graph_colors().SetColor(
      ir_operand_a->number(), OperandToColor(x86_64::Mem::BasePointerDisp(x86_64::Size::k32, 0)));
  interference_graph_colors().SetColor(ir_operand_b->number(), 0);
  interference_graph_colors().SetColor(
      ir_operand_c->number(), OperandToColor(x86_64::Mem::BasePointerDisp(x86_64::Size::k8, -8)));

  GenerateTranslationContexts();

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_operand_a.get(), func_ctx());
  x86_64::RM x86_64_operand_b = TranslateComputed(ir_operand_b.get(), func_ctx());
  x86_64::RM x86_64_operand_c = TranslateComputed(ir_operand_c.get(), func_ctx());
  x86_64::Imm x86_64_operand_d(kD);
  x86_64::Imm x86_64_operand_e(kE);

  x86_64::Reg x86_64_result_a = x86_64::edx;
  x86_64::Mem x86_64_result_b = x86_64::Mem::BasePointerDisp(x86_64::Size::k8, -16);
  x86_64::Mem x86_64_result_c = x86_64::Mem::BasePointerDisp(x86_64::Size::k8, -24);
  x86_64::Mem x86_64_result_d = x86_64::Mem::BasePointerDisp(x86_64::Size::k16, -32);
  x86_64::Mem x86_64_result_e = x86_64::Mem::BasePointerDisp(x86_64::Size::k64, -40);

  std::vector<MoveOperation> operations{MoveOperation(x86_64_result_a, x86_64_operand_a),
                                        MoveOperation(x86_64_result_b, x86_64_operand_b),
                                        MoveOperation(x86_64_result_c, x86_64_operand_c),
                                        MoveOperation(x86_64_result_d, x86_64_operand_d),
                                        MoveOperation(x86_64_result_e, x86_64_operand_e)};

  GenerateMovs(operations, ir_call_instr, block_ctx());

  CheckGeneratedInstrsForMoveOperations(operations);
}

TEST_F(GenerateMovsTest, GeneratesInstrsForSmallMoveChain) {
  const int16_t kD = 0x321;
  std::shared_ptr<ir::Computed> ir_operand_a = ir_func_builder().AddArg(ir::i64());
  std::shared_ptr<ir::Computed> ir_operand_b = ir_func_builder().AddArg(ir::i64());

  ir_func_builder().AddResultType(ir::i64());
  ir_func_builder().AddResultType(ir::i64());

  ir_block_builder().Call(ir::NilFunc(), {}, {ir_operand_a, ir_operand_b});
  ir_block_builder().Return({ir_operand_a, ir_operand_b});

  ir::CallInstr* ir_call_instr = static_cast<ir::CallInstr*>(ir_block()->instrs().front().get());

  GenerateIRInfo();

  interference_graph_colors().SetColor(ir_operand_a->number(), 0);
  interference_graph_colors().SetColor(ir_operand_b->number(), 2);

  GenerateTranslationContexts();

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_operand_a.get(), func_ctx());
  x86_64::RM x86_64_operand_b = TranslateComputed(ir_operand_b.get(), func_ctx());

  x86_64::Reg x86_64_result_a = x86_64::rdx;
  x86_64::Reg x86_64_result_b = x86_64::rbx;

  std::vector<MoveOperation> operations{MoveOperation(x86_64_result_a, x86_64_operand_a),
                                        MoveOperation(x86_64_result_b, x86_64_operand_b)};

  GenerateMovs(operations, ir_call_instr, block_ctx());

  EXPECT_EQ(x86_64_block()->instrs().size(), 2);
  CheckGeneratedInstrsForMoveOperations(operations);
}

TEST_F(GenerateMovsTest, GeneratesInstrsForLargeMoveChain) {
  const int16_t kD = 0x321;
  std::shared_ptr<ir::Computed> ir_operand_a = ir_func_builder().AddArg(ir::i64());
  std::shared_ptr<ir::Computed> ir_operand_b = ir_func_builder().AddArg(ir::bool_type());
  std::shared_ptr<ir::Computed> ir_operand_c = ir_func_builder().AddArg(ir::u32());

  ir_func_builder().AddResultType(ir::i64());
  ir_func_builder().AddResultType(ir::bool_type());
  ir_func_builder().AddResultType(ir::u32());

  ir_block_builder().Call(ir::NilFunc(), {},
                          {ir_operand_a, ir_operand_b, ir_operand_c, ir::ToIntConstant(Int(kD))});
  ir_block_builder().Return({ir_operand_a, ir_operand_b, ir_operand_c});

  ir::CallInstr* ir_call_instr = static_cast<ir::CallInstr*>(ir_block()->instrs().front().get());

  GenerateIRInfo();

  interference_graph_colors().SetColor(ir_operand_a->number(), 0);
  interference_graph_colors().SetColor(ir_operand_b->number(), 1);
  interference_graph_colors().SetColor(
      ir_operand_c->number(), OperandToColor(x86_64::Mem::BasePointerDisp(x86_64::Size::k32, 0)));

  GenerateTranslationContexts();

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_operand_a.get(), func_ctx());
  x86_64::RM x86_64_operand_b = TranslateComputed(ir_operand_b.get(), func_ctx());
  x86_64::RM x86_64_operand_c = TranslateComputed(ir_operand_c.get(), func_ctx());
  x86_64::Imm x86_64_operand_d(kD);

  x86_64::Mem x86_64_result_a = x86_64::Mem::BasePointerDisp(x86_64::Size::k64, 0);
  x86_64::Reg x86_64_result_b = x86_64::al;
  x86_64::Reg x86_64_result_c = x86_64::r8d;
  x86_64::Reg x86_64_result_d = x86_64::cx;

  std::vector<MoveOperation> operations{MoveOperation(x86_64_result_a, x86_64_operand_a),
                                        MoveOperation(x86_64_result_b, x86_64_operand_b),
                                        MoveOperation(x86_64_result_c, x86_64_operand_c),
                                        MoveOperation(x86_64_result_d, x86_64_operand_d)};

  GenerateMovs(operations, ir_call_instr, block_ctx());

  EXPECT_EQ(x86_64_block()->instrs().size(), 4);
  CheckGeneratedInstrsForMoveOperations(operations);
}

TEST_F(GenerateMovsTest, GeneratesInstrForRegSwap) {
  std::shared_ptr<ir::Computed> ir_operand_a = ir_func_builder().AddArg(ir::i64());
  std::shared_ptr<ir::Computed> ir_operand_b = ir_func_builder().AddArg(ir::bool_type());

  ir_func_builder().AddResultType(ir::i64());
  ir_func_builder().AddResultType(ir::bool_type());

  ir_block_builder().Call(ir::NilFunc(), {}, {ir_operand_a, ir_operand_b});
  ir_block_builder().Return({ir_operand_a, ir_operand_b});

  ir::CallInstr* ir_call_instr = static_cast<ir::CallInstr*>(ir_block()->instrs().front().get());

  GenerateIRInfo();

  interference_graph_colors().SetColor(ir_operand_a->number(), 0);
  interference_graph_colors().SetColor(ir_operand_b->number(), 1);

  GenerateTranslationContexts();

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_operand_a.get(), func_ctx());
  x86_64::RM x86_64_operand_b = TranslateComputed(ir_operand_b.get(), func_ctx());

  std::vector<MoveOperation> operations{MoveOperation(x86_64::rcx, x86_64_operand_a),
                                        MoveOperation(x86_64::al, x86_64_operand_b)};

  GenerateMovs(operations, ir_call_instr, block_ctx());

  EXPECT_EQ(x86_64_block()->instrs().size(), 1);
  CheckGeneratedInstrsForMoveOperations(operations);
}

TEST_F(GenerateMovsTest, GeneratesInstrsForCyclesWithAttchedAndSeparateChains) {
  const int64_t kJ = 0x123456789;
  std::shared_ptr<ir::Computed> ir_operand_a = ir_func_builder().AddArg(ir::i32());
  std::shared_ptr<ir::Computed> ir_operand_b = ir_func_builder().AddArg(ir::bool_type());
  std::shared_ptr<ir::Computed> ir_operand_c = ir_func_builder().AddArg(ir::u8());
  std::shared_ptr<ir::Computed> ir_operand_d = ir_func_builder().AddArg(ir::u64());
  std::shared_ptr<ir::Computed> ir_operand_e = ir_func_builder().AddArg(ir::func_type());
  std::shared_ptr<ir::Computed> ir_operand_f = ir_func_builder().AddArg(ir::pointer_type());
  std::shared_ptr<ir::Computed> ir_operand_g = ir_func_builder().AddArg(ir::i16());
  std::shared_ptr<ir::Computed> ir_operand_h = ir_func_builder().AddArg(ir::u16());
  std::shared_ptr<ir::Computed> ir_operand_i = ir_func_builder().AddArg(ir::i64());

  ir_func_builder().AddResultType(ir::i32());
  ir_func_builder().AddResultType(ir::bool_type());
  ir_func_builder().AddResultType(ir::u8());
  ir_func_builder().AddResultType(ir::u64());
  ir_func_builder().AddResultType(ir::func_type());
  ir_func_builder().AddResultType(ir::pointer_type());
  ir_func_builder().AddResultType(ir::i16());
  ir_func_builder().AddResultType(ir::u16());

  ir_block_builder().Call(
      ir::NilFunc(), {},
      {ir_operand_a, ir_operand_b, ir_operand_c, ir_operand_d, ir_operand_e, ir_operand_f,
       ir_operand_g, ir_operand_h, ir_operand_i, ir::ToIntConstant(Int(kJ))});
  ir_block_builder().Return({ir_operand_a, ir_operand_b, ir_operand_c, ir_operand_d, ir_operand_e,
                             ir_operand_f, ir_operand_g, ir_operand_h, ir_operand_i});

  ir::CallInstr* ir_call_instr = static_cast<ir::CallInstr*>(ir_block()->instrs().front().get());

  GenerateIRInfo();

  interference_graph_colors().SetColor(
      ir_operand_a->number(), OperandToColor(x86_64::Mem::BasePointerDisp(x86_64::Size::k32, 0)));
  interference_graph_colors().SetColor(ir_operand_b->number(), 0);
  interference_graph_colors().SetColor(ir_operand_c->number(), 5);
  interference_graph_colors().SetColor(ir_operand_d->number(), 11);
  interference_graph_colors().SetColor(
      ir_operand_e->number(), OperandToColor(x86_64::Mem::BasePointerDisp(x86_64::Size::k64, -8)));
  interference_graph_colors().SetColor(
      ir_operand_f->number(), OperandToColor(x86_64::Mem::BasePointerDisp(x86_64::Size::k64, -16)));
  interference_graph_colors().SetColor(
      ir_operand_g->number(), OperandToColor(x86_64::Mem::BasePointerDisp(x86_64::Size::k16, -32)));
  interference_graph_colors().SetColor(ir_operand_h->number(), 2);
  interference_graph_colors().SetColor(ir_operand_i->number(), 13);

  GenerateTranslationContexts();

  x86_64::RM x86_64_operand_a = TranslateComputed(ir_operand_a.get(), func_ctx());
  x86_64::RM x86_64_operand_b = TranslateComputed(ir_operand_b.get(), func_ctx());
  x86_64::RM x86_64_operand_c = TranslateComputed(ir_operand_c.get(), func_ctx());
  x86_64::RM x86_64_operand_d = TranslateComputed(ir_operand_d.get(), func_ctx());
  x86_64::RM x86_64_operand_e = TranslateComputed(ir_operand_e.get(), func_ctx());
  x86_64::RM x86_64_operand_f = TranslateComputed(ir_operand_f.get(), func_ctx());
  x86_64::RM x86_64_operand_g = TranslateComputed(ir_operand_g.get(), func_ctx());
  x86_64::RM x86_64_operand_h = TranslateComputed(ir_operand_h.get(), func_ctx());
  x86_64::RM x86_64_operand_i = TranslateComputed(ir_operand_i.get(), func_ctx());
  x86_64::Imm x86_64_operand_j(kJ);

  x86_64::Reg x86_64_result_a_1 = x86_64::eax;
  x86_64::Reg x86_64_result_a_2 = x86_64::r13d;
  x86_64::Reg x86_64_result_b = x86_64::dil;
  x86_64::Mem x86_64_result_c = x86_64::Mem::BasePointerDisp(x86_64::Size::k8, 0);
  x86_64::Mem x86_64_result_d = x86_64::Mem::BasePointerDisp(x86_64::Size::k64, -8);
  x86_64::Mem x86_64_result_e = x86_64::Mem::BasePointerDisp(x86_64::Size::k64, -40);
  x86_64::Mem x86_64_result_f = x86_64::Mem::BasePointerDisp(x86_64::Size::k64, -32);
  x86_64::Mem x86_64_result_g_1 = x86_64::Mem::BasePointerDisp(x86_64::Size::k16, -16);
  x86_64::Reg x86_64_result_g_2 = x86_64::dx;
  x86_64::Mem x86_64_result_h = x86_64::Mem::BasePointerDisp(x86_64::Size::k16, -48);
  x86_64::Reg x86_64_result_i = x86_64::rcx;
  x86_64::Reg x86_64_result_j = x86_64::r15;

  std::vector<MoveOperation> operations{MoveOperation(x86_64_result_a_1, x86_64_operand_a),
                                        MoveOperation(x86_64_result_a_2, x86_64_operand_a),
                                        MoveOperation(x86_64_result_b, x86_64_operand_b),
                                        MoveOperation(x86_64_result_c, x86_64_operand_c),
                                        MoveOperation(x86_64_result_d, x86_64_operand_d),
                                        MoveOperation(x86_64_result_e, x86_64_operand_e),
                                        MoveOperation(x86_64_result_f, x86_64_operand_f),
                                        MoveOperation(x86_64_result_g_1, x86_64_operand_g),
                                        MoveOperation(x86_64_result_g_2, x86_64_operand_g),
                                        MoveOperation(x86_64_result_h, x86_64_operand_h),
                                        MoveOperation(x86_64_result_i, x86_64_operand_i),
                                        MoveOperation(x86_64_result_j, x86_64_operand_j)};
  GenerateMovs(operations, ir_call_instr, block_ctx());
  CheckGeneratedInstrsForMoveOperations(operations);
}

}  // namespace ir_to_x86_64_translator
