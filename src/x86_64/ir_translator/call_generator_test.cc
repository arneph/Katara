//
//  call_generator_test.cc
//  Katara
//
//  Created by Arne Philipeit on 1/23/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/x86_64/ir_translator/call_generator.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/builder/block_builder.h"
#include "src/ir/builder/func_builder.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/x86_64/ir_translator/testing/instr_translator_test_setup.h"

namespace ir_to_x86_64_translator {

class GenerateCallTest : public InstrTranslatorTest {};

TEST_F(GenerateCallTest, SavesCallerSavedRegs) {
  // Function to call:
  std::shared_ptr<ir::Computed> ir_operand_a = ir_func_builder().AddArg(ir::func_type());
  // Operand used only as argument for function call:
  std::shared_ptr<ir::Computed> ir_operand_b = ir_func_builder().AddArg(ir::bool_type());
  // Operand used as argument for function call and later:
  std::shared_ptr<ir::Computed> ir_operand_c = ir_func_builder().AddArg(ir::i64());
  // Operand in caller saved reg and not used as argument for function call but later:
  std::shared_ptr<ir::Computed> ir_operand_d = ir_func_builder().AddArg(ir::u8());
  // Operand in callee saved reg and not used as argument for function call but later:
  std::shared_ptr<ir::Computed> ir_operand_e = ir_func_builder().AddArg(ir::i8());

  ir_func_builder().AddResultType(ir::func_type());
  ir_func_builder().AddResultType(ir::i64());
  ir_func_builder().AddResultType(ir::u8());
  ir_func_builder().AddResultType(ir::i8());
  ir_func_builder().AddResultType(ir::i32());

  std::vector<std::shared_ptr<ir::Computed>> call_results =
      ir_block_builder().Call(ir_operand_a, /*result_types=*/{ir::i32()},
                              /*args=*/{ir_operand_b, ir_operand_c});
  std::shared_ptr<ir::Computed> ir_operand_f = call_results.front();
  ir_block_builder().Return({ir_operand_a, ir_operand_c, ir_operand_d, ir_operand_e, ir_operand_f});

  GenerateIRInfo();

  interference_graph_colors().SetColor(ir_operand_a->number(), 1);  // rcx - called func
  interference_graph_colors().SetColor(ir_operand_b->number(), 5);  // rdi - 1st arg of called func
  interference_graph_colors().SetColor(ir_operand_c->number(), 4);  // rsi - 2nd arg of called func
  interference_graph_colors().SetColor(ir_operand_d->number(), 2);  // rdx - caller saved
  interference_graph_colors().SetColor(ir_operand_e->number(), 3);  // rbx - callee saved
  interference_graph_colors().SetColor(ir_operand_f->number(), 0);  // rax - result of called func

  GenerateTranslationContexts();

  GenerateCall(ir_block()->instrs().front().get(), ir_operand_a.get(),
               /*results=*/{ir_operand_f.get()},
               /*args=*/{ir_operand_b.get(), ir_operand_c.get()}, block_ctx());

  EXPECT_EQ(x86_64_block()->instrs().size(), 7);
  EXPECT_EQ(x86_64_block()->instrs().at(0)->ToString(), "push rcx");
  EXPECT_EQ(x86_64_block()->instrs().at(1)->ToString(), "push rdx");
  EXPECT_EQ(x86_64_block()->instrs().at(2)->ToString(), "push rsi");
  EXPECT_EQ(x86_64_block()->instrs().at(3)->ToString(), "call rcx");
  EXPECT_EQ(x86_64_block()->instrs().at(4)->ToString(), "pop rsi");
  EXPECT_EQ(x86_64_block()->instrs().at(5)->ToString(), "pop rdx");
  EXPECT_EQ(x86_64_block()->instrs().at(6)->ToString(), "pop rcx");
}

}  // namespace ir_to_x86_64_translator
