//
//  shared_pointer_lowerer.cc
//  Katara
//
//  Created by Arne Philipeit on 7/29/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "shared_pointer_lowerer.h"

#include <unordered_map>

namespace lang {
namespace ir_lowerers {
namespace {

constexpr common::Int kControlBlockSize{int64_t{24}};
constexpr common::Int kWeakRefCountPointerOffset{int64_t{8}};
constexpr common::Int kDestructorPointerOffset{int64_t{16}};

ir::func_num_t BuildMakeSharedPointerFunc(ir::Program* program) {
  ir::Func* func = program->AddFunc();
  func->set_name("make_shared");

  auto underlying_size = std::make_shared<ir::Computed>(&ir::kI64, func->next_computed_number());
  auto destructor = std::make_shared<ir::Computed>(&ir::kFunc, func->next_computed_number());

  func->args().push_back(underlying_size);
  func->args().push_back(destructor);
  func->result_types().push_back(&ir::kPointer);
  func->result_types().push_back(&ir::kPointer);

  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());

  auto control_block_size = std::make_shared<ir::IntConstant>(kControlBlockSize);
  auto total_size = std::make_shared<ir::Computed>(&ir::kI64, func->next_computed_number());
  block->instrs().push_back(std::make_unique<ir::IntBinaryInstr>(
      total_size, common::Int::BinaryOp::kAdd, control_block_size, underlying_size));

  auto control_block_pointer =
      std::make_shared<ir::Computed>(&ir::kPointer, func->next_computed_number());
  block->instrs().push_back(std::make_unique<ir::MallocInstr>(control_block_pointer, total_size));

  auto one = std::make_shared<ir::IntConstant>(common::Int{int64_t{1}});
  block->instrs().push_back(std::make_unique<ir::StoreInstr>(control_block_pointer, one));
  
  auto zero = std::make_shared<ir::IntConstant>(common::Int{int64_t{0}});
  auto weak_ref_count_offset = std::make_shared<ir::IntConstant>(kWeakRefCountPointerOffset);
  auto weak_ref_count_pointer =
      std::make_shared<ir::Computed>(&ir::kPointer, func->next_computed_number());
  block->instrs().push_back(std::make_unique<ir::PointerOffsetInstr>(
      weak_ref_count_pointer, control_block_pointer, weak_ref_count_offset));
  block->instrs().push_back(std::make_unique<ir::StoreInstr>(weak_ref_count_pointer, zero));
  
  auto destructor_offset = std::make_shared<ir::IntConstant>(kDestructorPointerOffset);
  auto destructor_pointer =
      std::make_shared<ir::Computed>(&ir::kPointer, func->next_computed_number());
  block->instrs().push_back(std::make_unique<ir::PointerOffsetInstr>(
      destructor_pointer, control_block_pointer, destructor_offset));
  block->instrs().push_back(std::make_unique<ir::StoreInstr>(destructor_pointer, destructor));

  auto underlying_pointer =
      std::make_shared<ir::Computed>(&ir::kPointer, func->next_computed_number());
  block->instrs().push_back(std::make_unique<ir::PointerOffsetInstr>(
      underlying_pointer, control_block_pointer, control_block_size));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>(
      std::vector<std::shared_ptr<ir::Value>>{control_block_pointer, underlying_pointer}));

  return func->number();
}

ir::func_num_t BuildCopySharedPointerFunc(ir::Program* program, bool copy_is_strong) {
  ir::Func* func = program->AddFunc();
  func->set_name(copy_is_strong ? "strong_copy_shared" : "weak_copy_shared");

  auto control_block_pointer =
      std::make_shared<ir::Computed>(&ir::kPointer, func->next_computed_number());
  auto old_underlying_pointer =
      std::make_shared<ir::Computed>(&ir::kPointer, func->next_computed_number());
  auto underlying_pointer_offset =
      std::make_shared<ir::Computed>(&ir::kI64, func->next_computed_number());

  func->args().push_back(control_block_pointer);
  func->args().push_back(old_underlying_pointer);
  func->args().push_back(underlying_pointer_offset);
  func->result_types().push_back(&ir::kPointer);

  ir::Block* block = func->AddBlock();
  func->set_entry_block_num(block->number());

  std::shared_ptr<ir::Computed> ref_count_pointer;
  if (copy_is_strong) {
    ref_count_pointer = std::move(control_block_pointer);
  } else {
    auto weak_ref_count_offset = std::make_shared<ir::IntConstant>(kWeakRefCountPointerOffset);
    ref_count_pointer = std::make_shared<ir::Computed>(&ir::kPointer, func->next_computed_number());
    block->instrs().push_back(std::make_unique<ir::PointerOffsetInstr>(
        ref_count_pointer, control_block_pointer, weak_ref_count_offset));
  }

  auto old_ref_count = std::make_shared<ir::Computed>(&ir::kI64, func->next_computed_number());
  block->instrs().push_back(std::make_unique<ir::LoadInstr>(old_ref_count, ref_count_pointer));

  auto ref_count_delta = std::make_shared<ir::IntConstant>(common::Int{int64_t{1}});
  auto new_ref_count = std::make_shared<ir::Computed>(&ir::kI64, func->number());
  block->instrs().push_back(std::make_unique<ir::IntBinaryInstr>(
      new_ref_count, common::Int::BinaryOp::kAdd, old_ref_count, ref_count_delta));
  block->instrs().push_back(std::make_unique<ir::StoreInstr>(ref_count_pointer, new_ref_count));

  auto new_underlying_pointer =
      std::make_shared<ir::Computed>(&ir::kPointer, func->next_computed_number());
  block->instrs().push_back(std::make_unique<ir::PointerOffsetInstr>(
      new_underlying_pointer, old_underlying_pointer, underlying_pointer_offset));
  block->instrs().push_back(std::make_unique<ir::ReturnInstr>(
      std::vector<std::shared_ptr<ir::Value>>{new_underlying_pointer}));

  return func->number();
}

ir::func_num_t BuildDeleteSharedPointerFunc(ir::Program* program, bool pointer_is_strong) {
  ir::Func* func = program->AddFunc();
  func->set_name(pointer_is_strong ? "delete_strong_shared" : "delete_weak_shared");

  auto control_block_pointer =
      std::make_shared<ir::Computed>(&ir::kPointer, func->next_computed_number());

  func->args().push_back(control_block_pointer);

  ir::Block* entry_block = func->AddBlock();
  ir::Block* update_count_block = func->AddBlock();
  ir::Block* count_reaches_zero_block = func->AddBlock();
  ir::Block* keep_heap_block = func->AddBlock();
  ir::Block* free_heap_block = func->AddBlock();
  func->set_entry_block_num(entry_block->number());

  std::shared_ptr<ir::Computed> ref_count_pointer;
  if (pointer_is_strong) {
    ref_count_pointer = control_block_pointer;
  } else {
    auto weak_ref_count_offset = std::make_shared<ir::IntConstant>(kWeakRefCountPointerOffset);
    ref_count_pointer = std::make_shared<ir::Computed>(&ir::kPointer, func->next_computed_number());
    entry_block->instrs().push_back(std::make_unique<ir::PointerOffsetInstr>(
        ref_count_pointer, control_block_pointer, weak_ref_count_offset));
  }

  auto old_ref_count = std::make_shared<ir::Computed>(&ir::kI64, func->next_computed_number());
  entry_block->instrs().push_back(
      std::make_unique<ir::LoadInstr>(old_ref_count, ref_count_pointer));

  auto one = std::make_shared<ir::IntConstant>(common::Int{int64_t{1}});
  auto is_one = std::make_shared<ir::Computed>(&ir::kBool, func->next_computed_number());
  entry_block->instrs().push_back(std::make_unique<ir::IntCompareInstr>(
      is_one, common::Int::CompareOp::kEq, old_ref_count, one));
  entry_block->instrs().push_back(std::make_unique<ir::JumpCondInstr>(
      is_one, count_reaches_zero_block->number(), update_count_block->number()));
  func->AddControlFlow(entry_block->number(), count_reaches_zero_block->number());
  func->AddControlFlow(entry_block->number(), update_count_block->number());

  auto new_ref_count = std::make_shared<ir::Computed>(&ir::kI64, func->next_computed_number());
  update_count_block->instrs().push_back(std::make_unique<ir::IntBinaryInstr>(
      new_ref_count, common::Int::BinaryOp::kSub, old_ref_count, one));
  update_count_block->instrs().push_back(
      std::make_unique<ir::StoreInstr>(ref_count_pointer, new_ref_count));
  update_count_block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  ir::Block* current_block;
  auto other_ref_count = std::make_shared<ir::Computed>(&ir::kI64, func->next_computed_number());
  if (pointer_is_strong) {
    ir::Block* destruct_underlying_block = func->AddBlock();
    ir::Block* check_weak_ref_count_block = func->AddBlock();

    auto destructor_offset = std::make_shared<ir::IntConstant>(kDestructorPointerOffset);
    auto destructor_pointer =
        std::make_shared<ir::Computed>(&ir::kPointer, func->next_computed_number());
    count_reaches_zero_block->instrs().push_back(std::make_unique<ir::PointerOffsetInstr>(
        destructor_pointer, control_block_pointer, destructor_offset));
    auto destructor = std::make_shared<ir::Computed>(&ir::kFunc, func->next_computed_number());
    count_reaches_zero_block->instrs().push_back(
        std::make_unique<ir::LoadInstr>(destructor, destructor_pointer));
    auto has_no_destructor =
        std::make_shared<ir::Computed>(&ir::kBool, func->next_computed_number());
    count_reaches_zero_block->instrs().push_back(
        std::make_unique<ir::NilTestInstr>(has_no_destructor, destructor));
    count_reaches_zero_block->instrs().push_back(
        std::make_unique<ir::JumpCondInstr>(has_no_destructor, check_weak_ref_count_block->number(),
                                            destruct_underlying_block->number()));
    func->AddControlFlow(count_reaches_zero_block->number(), check_weak_ref_count_block->number());
    func->AddControlFlow(count_reaches_zero_block->number(), destruct_underlying_block->number());

    auto control_block_size = std::make_shared<ir::IntConstant>(kControlBlockSize);
    auto underlying_pointer =
        std::make_shared<ir::Computed>(&ir::kPointer, func->next_computed_number());
    destruct_underlying_block->instrs().push_back(std::make_unique<ir::PointerOffsetInstr>(
        underlying_pointer, control_block_pointer, control_block_size));
    destruct_underlying_block->instrs().push_back(std::make_unique<ir::CallInstr>(
        destructor, /*results=*/std::vector<std::shared_ptr<ir::Computed>>{},
        /*args=*/std::vector<std::shared_ptr<ir::Value>>{underlying_pointer}));
    destruct_underlying_block->instrs().push_back(
        std::make_unique<ir::JumpInstr>(check_weak_ref_count_block->number()));
    func->AddControlFlow(destruct_underlying_block->number(), check_weak_ref_count_block->number());

    auto weak_ref_count_offset = std::make_shared<ir::IntConstant>(kWeakRefCountPointerOffset);
    auto weak_ref_count_pointer =
        std::make_shared<ir::Computed>(&ir::kPointer, func->next_computed_number());
    check_weak_ref_count_block->instrs().push_back(std::make_unique<ir::PointerOffsetInstr>(
        weak_ref_count_pointer, control_block_pointer, weak_ref_count_offset));
    check_weak_ref_count_block->instrs().push_back(
        std::make_unique<ir::LoadInstr>(other_ref_count, weak_ref_count_pointer));
    current_block = check_weak_ref_count_block;

  } else {
    count_reaches_zero_block->instrs().push_back(
        std::make_unique<ir::LoadInstr>(other_ref_count, control_block_pointer));
    current_block = count_reaches_zero_block;
  }

  auto zero = std::make_shared<ir::IntConstant>(common::Int{int64_t{0}});
  auto is_zero = std::make_shared<ir::Computed>(&ir::kBool, func->next_computed_number());
  current_block->instrs().push_back(std::make_unique<ir::IntCompareInstr>(
      is_zero, common::Int::CompareOp::kEq, other_ref_count, zero));
  current_block->instrs().push_back(std::make_unique<ir::JumpCondInstr>(
      is_zero, free_heap_block->number(), keep_heap_block->number()));
  func->AddControlFlow(current_block->number(), free_heap_block->number());
  func->AddControlFlow(current_block->number(), keep_heap_block->number());

  keep_heap_block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  free_heap_block->instrs().push_back(std::make_unique<ir::FreeInstr>(control_block_pointer));
  free_heap_block->instrs().push_back(std::make_unique<ir::ReturnInstr>());

  return func->number();
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
