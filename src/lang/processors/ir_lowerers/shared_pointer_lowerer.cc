//
//  shared_pointer_lowerer.cc
//  Katara
//
//  Created by Arne Philipeit on 7/29/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "shared_pointer_lowerer.h"

#include <unordered_map>

#include "src/ir/builder/block_builder.h"
#include "src/ir/builder/func_builder.h"

namespace lang {
namespace ir_lowerers {
namespace {

constexpr common::Int kControlBlockSize{int64_t{24}};
constexpr common::Int kWeakRefCountPointerOffset{int64_t{8}};
constexpr common::Int kDestructorPointerOffset{int64_t{16}};

ir::func_num_t BuildMakeSharedPointerFunc(ir::Program* program) {
  ir_builder::FuncBuilder fb = ir_builder::FuncBuilder::ForNewFuncInProgram(program);

  fb.SetName("make_shared");
  auto underlying_size = fb.AddArg(ir::i64());
  auto destructor = fb.AddArg(ir::func_type());
  fb.AddResultType(ir::pointer_type());
  fb.AddResultType(ir::pointer_type());

  ir_builder::BlockBuilder bb = fb.AddEntryBlock();

  auto control_block_size = std::make_shared<ir::IntConstant>(kControlBlockSize);
  auto total_size = bb.IntAdd(control_block_size, underlying_size);
  auto control_block_pointer = bb.Malloc(total_size);
  bb.Store(control_block_pointer, ir::I64One());

  auto weak_ref_count_offset = std::make_shared<ir::IntConstant>(kWeakRefCountPointerOffset);
  auto weak_ref_count_pointer = bb.OffsetPointer(control_block_pointer, weak_ref_count_offset);
  bb.Store(weak_ref_count_pointer, ir::I64Zero());

  auto destructor_offset = std::make_shared<ir::IntConstant>(kDestructorPointerOffset);
  auto destructor_pointer = bb.OffsetPointer(control_block_pointer, destructor_offset);
  bb.Store(destructor_pointer, destructor);

  auto underlying_pointer = bb.OffsetPointer(control_block_pointer, control_block_size);
  bb.Return({control_block_pointer, underlying_pointer});

  return fb.func_number();
}

ir::func_num_t BuildCopySharedPointerFunc(ir::Program* program, bool copy_is_strong) {
  ir_builder::FuncBuilder fb = ir_builder::FuncBuilder::ForNewFuncInProgram(program);

  fb.SetName(copy_is_strong ? "strong_copy_shared" : "weak_copy_shared");

  auto control_block_pointer = fb.AddArg(ir::pointer_type());
  auto old_underlying_pointer = fb.AddArg(ir::pointer_type());
  auto underlying_pointer_offset = fb.AddArg(ir::i64());
  fb.AddResultType(ir::pointer_type());

  ir_builder::BlockBuilder bb = fb.AddEntryBlock();

  std::shared_ptr<ir::Computed> ref_count_pointer;
  if (copy_is_strong) {
    ref_count_pointer = std::move(control_block_pointer);
  } else {
    auto weak_ref_count_offset = std::make_shared<ir::IntConstant>(kWeakRefCountPointerOffset);
    ref_count_pointer = bb.OffsetPointer(control_block_pointer, weak_ref_count_offset);
  }

  auto old_ref_count = bb.Load(ir::i64(), ref_count_pointer);
  auto new_ref_count = bb.IntAdd(old_ref_count, ir::I64One());
  bb.Store(ref_count_pointer, new_ref_count);

  auto new_underlying_pointer = bb.OffsetPointer(old_underlying_pointer, underlying_pointer_offset);
  bb.Return({new_underlying_pointer});

  return fb.func_number();
}

ir::func_num_t BuildDeleteSharedPointerFunc(ir::Program* program, bool pointer_is_strong) {
  ir_builder::FuncBuilder fb = ir_builder::FuncBuilder::ForNewFuncInProgram(program);

  fb.SetName(pointer_is_strong ? "delete_strong_shared" : "delete_weak_shared");

  auto control_block_pointer = fb.AddArg(ir::pointer_type());

  ir_builder::BlockBuilder entry_bb = fb.AddEntryBlock();
  ir_builder::BlockBuilder update_count_bb = fb.AddBlock();
  ir_builder::BlockBuilder count_reaches_zero_bb = fb.AddBlock();

  std::shared_ptr<ir::Computed> ref_count_pointer;
  if (pointer_is_strong) {
    ref_count_pointer = control_block_pointer;
  } else {
    auto weak_ref_count_offset = std::make_shared<ir::IntConstant>(kWeakRefCountPointerOffset);
    ref_count_pointer = entry_bb.OffsetPointer(control_block_pointer, weak_ref_count_offset);
  }

  auto old_ref_count = entry_bb.Load(ir::i64(), ref_count_pointer);
  auto is_one = entry_bb.IntEq(old_ref_count, ir::I64One());
  entry_bb.JumpCond(is_one, count_reaches_zero_bb.block_number(), update_count_bb.block_number());

  auto new_ref_count = update_count_bb.IntSub(old_ref_count, ir::I64One());
  update_count_bb.Store(ref_count_pointer, new_ref_count);
  update_count_bb.Return();

  auto build_check_other_ref_count = [&fb, control_block_pointer](
                                         ir_builder::BlockBuilder& check_other_ref_count_bb,
                                         std::shared_ptr<ir::Computed> other_ref_count) {
    ir_builder::BlockBuilder keep_heap_bb = fb.AddBlock();
    ir_builder::BlockBuilder free_heap_bb = fb.AddBlock();

    auto is_zero = check_other_ref_count_bb.IntEq(other_ref_count, ir::I64Zero());
    check_other_ref_count_bb.JumpCond(is_zero, free_heap_bb.block_number(),
                                      keep_heap_bb.block_number());

    keep_heap_bb.Return();

    free_heap_bb.Free(control_block_pointer);
    free_heap_bb.Return();
  };

  if (pointer_is_strong) {
    ir_builder::BlockBuilder destruct_underlying_bb = fb.AddBlock();
    ir_builder::BlockBuilder check_weak_ref_count_bb = fb.AddBlock();

    auto destructor_offset = std::make_shared<ir::IntConstant>(kDestructorPointerOffset);
    auto destructor_pointer =
        count_reaches_zero_bb.OffsetPointer(control_block_pointer, destructor_offset);
    auto destructor = count_reaches_zero_bb.Load(ir::func_type(), destructor_pointer);
    auto has_no_destructor = count_reaches_zero_bb.IsNil(destructor);
    count_reaches_zero_bb.JumpCond(has_no_destructor, check_weak_ref_count_bb.block_number(),
                                   destruct_underlying_bb.block_number());

    auto control_block_size = std::make_shared<ir::IntConstant>(kControlBlockSize);
    auto underlying_pointer =
        destruct_underlying_bb.OffsetPointer(control_block_pointer, control_block_size);
    destruct_underlying_bb.Call(destructor, {}, {underlying_pointer});
    destruct_underlying_bb.Jump(check_weak_ref_count_bb.block_number());

    auto weak_ref_count_offset = std::make_shared<ir::IntConstant>(kWeakRefCountPointerOffset);
    auto weak_ref_count_pointer =
        check_weak_ref_count_bb.OffsetPointer(control_block_pointer, weak_ref_count_offset);
    auto weak_ref_count = check_weak_ref_count_bb.Load(ir::i64(), weak_ref_count_pointer);
    build_check_other_ref_count(check_weak_ref_count_bb, weak_ref_count);

  } else {
    auto strong_ref_count = count_reaches_zero_bb.Load(ir::i64(), control_block_pointer);
    build_check_other_ref_count(count_reaches_zero_bb, strong_ref_count);
  }

  return fb.func_number();
}

struct DecomposedSharedPointer {
  std::shared_ptr<ir::Computed> control_block_pointer;
  std::shared_ptr<ir::Computed> underlying_pointer;
};

void LowerSharedPointersInFunc(ir::Func* func) {
  std::unordered_map<ir::value_num_t, DecomposedSharedPointer> decomposed_shared_pointers;
  func->ForBlocksInDominanceOrder([&](ir::Block* block) {
    for (auto it = block->instrs().begin(); it != block->instrs().end(); ++it) {
      ir::Instr* old_instr = it->get();
      switch (old_instr->instr_kind()) {
        default:
          continue;
      }
    }
  });
}

}  // namespace

void LowerSharedPointersInProgram(ir::Program* program) {
  ir::func_num_t make_shared_func_num = BuildMakeSharedPointerFunc(program);
  ir::func_num_t strong_copy_shared_func_num =
      BuildCopySharedPointerFunc(program, /*copy_is_strong=*/true);
  ir::func_num_t weak_copy_shared_func_num =
      BuildCopySharedPointerFunc(program, /*copy_is_strong=*/false);
  ir::func_num_t delete_strong_shared_func_num =
      BuildDeleteSharedPointerFunc(program, /*pointer_is_strong=*/true);
  ir::func_num_t delete_weak_shared_func_num =
      BuildDeleteSharedPointerFunc(program, /*pointer_is_strong=*/false);

  for (auto& func : program->funcs()) {
    LowerSharedPointersInFunc(func.get());
  }
}

}  // namespace ir_lowerers
}  // namespace lang
