//
//  shared_pointer_test.cc
//  Katara
//
//  Created by Arne Philipeit on 10/23/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/lang/runtime/shared_pointer.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/ir/builder/block_builder.h"
#include "src/ir/builder/func_builder.h"
#include "src/ir/interpreter/interpreter.h"
#include "src/ir/representation/program.h"

namespace lang {
namespace runtime {
namespace {

TEST(SharedPointerImplTest, HandlesDeleteStrongNilPointer) {
  ir::Program program;
  SharedPointerLoweringFuncs lowering_funcs = AddSharedPointerLoweringFuncsToProgram(&program);
  ir_builder::FuncBuilder fb = ir_builder::FuncBuilder::ForNewFuncInProgram(&program);
  fb.AddResultType(ir::i64());
  ir_builder::BlockBuilder bb = fb.AddEntryBlock();
  bb.AddInstr<ir::CallInstr>(ir::ToFuncConstant(lowering_funcs.delete_strong_shared_func_num),
                             /*results=*/std::vector<std::shared_ptr<ir::Computed>>{},
                             /*args=*/std::vector<std::shared_ptr<ir::Value>>{ir::NilPointer()});
  bb.Return({ir::I64Zero()});

  program.set_entry_func_num(fb.func_number());

  ir_interpreter::Interpreter interpreter(&program, /*sanitize=*/true);
  interpreter.Run();

  EXPECT_EQ(interpreter.exit_code(), 0);
}

TEST(SharedPointerImplTest, HandlesDeleteWeakNilPointer) {
  ir::Program program;
  SharedPointerLoweringFuncs lowering_funcs = AddSharedPointerLoweringFuncsToProgram(&program);
  ir_builder::FuncBuilder fb = ir_builder::FuncBuilder::ForNewFuncInProgram(&program);
  fb.AddResultType(ir::i64());
  ir_builder::BlockBuilder bb = fb.AddEntryBlock();
  bb.AddInstr<ir::CallInstr>(ir::ToFuncConstant(lowering_funcs.delete_weak_shared_func_num),
                             /*results=*/std::vector<std::shared_ptr<ir::Computed>>{},
                             /*args=*/std::vector<std::shared_ptr<ir::Value>>{ir::NilPointer()});
  bb.Return({ir::I64Zero()});

  program.set_entry_func_num(fb.func_number());

  ir_interpreter::Interpreter interpreter(&program, /*sanitize=*/true);
  interpreter.Run();

  EXPECT_EQ(interpreter.exit_code(), 0);
}

}  // namespace
}  // namespace runtime
}  // namespace lang
