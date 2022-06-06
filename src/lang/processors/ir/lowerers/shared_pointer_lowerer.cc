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

struct DecomposedShared {
  std::shared_ptr<ir::Computed> control_block_pointer;
  std::shared_ptr<ir::Computed> underlying_pointer;
};

ir::func_num_t BuildMakeSharedFunc(ir::Program* program) {
  ir_builder::FuncBuilder fb = ir_builder::FuncBuilder::ForNewFuncInProgram(program);

  fb.SetName("make_shared");
  auto underlying_size = fb.AddArg(ir::i64());
  auto destructor = fb.AddArg(ir::func_type());
  fb.AddResultType(ir::pointer_type());
  fb.AddResultType(ir::pointer_type());

  ir_builder::BlockBuilder bb = fb.AddEntryBlock();

  auto control_block_size = ir::ToIntConstant(kControlBlockSize);
  auto total_size = bb.IntAdd(control_block_size, underlying_size);
  auto control_block_pointer = bb.Malloc(total_size);
  bb.Store(control_block_pointer, ir::I64One());

  auto weak_ref_count_offset = ir::ToIntConstant(kWeakRefCountPointerOffset);
  auto weak_ref_count_pointer = bb.OffsetPointer(control_block_pointer, weak_ref_count_offset);
  bb.Store(weak_ref_count_pointer, ir::I64Zero());

  auto destructor_offset = ir::ToIntConstant(kDestructorPointerOffset);
  auto destructor_pointer = bb.OffsetPointer(control_block_pointer, destructor_offset);
  bb.Store(destructor_pointer, destructor);

  auto underlying_pointer = bb.OffsetPointer(control_block_pointer, control_block_size);
  bb.Return({control_block_pointer, underlying_pointer});

  return fb.func_number();
}

std::unique_ptr<ir::CallInstr> CallMakeSharedFunc(ir::func_num_t make_shared_func_num,
                                                  DecomposedShared& decomposed_result) {
  return std::make_unique<ir::CallInstr>(
      ir::ToFuncConstant(make_shared_func_num),
      std::vector<std::shared_ptr<ir::Computed>>{decomposed_result.control_block_pointer,
                                                 decomposed_result.underlying_pointer},
      std::vector<std::shared_ptr<ir::Value>>{ir::I64Eight(), ir::NilFunc()});
  // TODO: use actual size and destructor of element type
}

ir::func_num_t BuildCopySharedFunc(ir::Program* program, bool copy_is_strong) {
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
    auto weak_ref_count_offset = ir::ToIntConstant(kWeakRefCountPointerOffset);
    ref_count_pointer = bb.OffsetPointer(control_block_pointer, weak_ref_count_offset);
  }

  auto old_ref_count = bb.Load(ir::i64(), ref_count_pointer);
  auto new_ref_count = bb.IntAdd(old_ref_count, ir::I64One());
  bb.Store(ref_count_pointer, new_ref_count);

  auto new_underlying_pointer = bb.OffsetPointer(old_underlying_pointer, underlying_pointer_offset);
  bb.Return({new_underlying_pointer});

  return fb.func_number();
}

std::unique_ptr<ir::CallInstr> CallCopySharedFunc(ir::func_num_t copy_shared_func_num,
                                                  DecomposedShared& decomposed_result,
                                                  DecomposedShared& decomposed_copied,
                                                  std::shared_ptr<ir::Value> offset) {
  return std::make_unique<ir::CallInstr>(
      ir::ToFuncConstant(copy_shared_func_num),
      std::vector<std::shared_ptr<ir::Computed>>{decomposed_result.underlying_pointer},
      std::vector<std::shared_ptr<ir::Value>>{decomposed_copied.control_block_pointer,
                                              decomposed_copied.underlying_pointer, offset});
}

ir::func_num_t BuildDeleteSharedFunc(ir::Program* program, bool pointer_is_strong) {
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
    auto weak_ref_count_offset = ir::ToIntConstant(kWeakRefCountPointerOffset);
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

    auto destructor_offset = ir::ToIntConstant(kDestructorPointerOffset);
    auto destructor_pointer =
        count_reaches_zero_bb.OffsetPointer(control_block_pointer, destructor_offset);
    auto destructor = count_reaches_zero_bb.Load(ir::func_type(), destructor_pointer);
    auto has_no_destructor = count_reaches_zero_bb.IsNil(destructor);
    count_reaches_zero_bb.JumpCond(has_no_destructor, check_weak_ref_count_bb.block_number(),
                                   destruct_underlying_bb.block_number());

    auto control_block_size = ir::ToIntConstant(kControlBlockSize);
    auto underlying_pointer =
        destruct_underlying_bb.OffsetPointer(control_block_pointer, control_block_size);
    destruct_underlying_bb.Call(destructor, {}, {underlying_pointer});
    destruct_underlying_bb.Jump(check_weak_ref_count_bb.block_number());

    auto weak_ref_count_offset = ir::ToIntConstant(kWeakRefCountPointerOffset);
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

std::unique_ptr<ir::CallInstr> CallDeleteSharedFunc(ir::func_num_t delete_shared_func_num,
                                                    DecomposedShared& decomposed_deleted) {
  return std::make_unique<ir::CallInstr>(
      ir::ToFuncConstant(delete_shared_func_num), std::vector<std::shared_ptr<ir::Computed>>{},
      std::vector<std::shared_ptr<ir::Value>>{decomposed_deleted.control_block_pointer});
}

ir::func_num_t BuildValidateWeakSharedFunc(ir::Program* program) {
  ir_builder::FuncBuilder fb = ir_builder::FuncBuilder::ForNewFuncInProgram(program);

  fb.SetName("validate_weak_shared");
  auto control_block_pointer = fb.AddArg(ir::pointer_type());

  ir_builder::BlockBuilder entry_bb = fb.AddEntryBlock();
  ir_builder::BlockBuilder ok_bb = fb.AddBlock();
  ir_builder::BlockBuilder panic_bb = fb.AddBlock();

  auto strong_ref_count = entry_bb.Load(ir::i64(), control_block_pointer);
  auto is_zero = entry_bb.IntEq(strong_ref_count, ir::I64Zero());
  entry_bb.JumpCond(is_zero, panic_bb.block_number(), ok_bb.block_number());

  ok_bb.Return();

  panic_bb.AddInstr<ir_ext::PanicInstr>(
      std::make_shared<ir_ext::StringConstant>("attempted to access deleted weak pointer"));

  return fb.func_number();
}

std::unique_ptr<ir::CallInstr> CallValidateWeakSharedFunc(
    ir::func_num_t validate_weak_shared_func_num, DecomposedShared& decomposed_validated) {
  return std::make_unique<ir::CallInstr>(
      ir::ToFuncConstant(validate_weak_shared_func_num),
      std::vector<std::shared_ptr<ir::Computed>>{},
      std::vector<std::shared_ptr<ir::Value>>{decomposed_validated.control_block_pointer});
}

struct LoweringFunctions {
  ir::func_num_t make_shared_func_num;
  ir::func_num_t strong_copy_shared_func_num;
  ir::func_num_t weak_copy_shared_func_num;
  ir::func_num_t delete_strong_shared_func_num;
  ir::func_num_t delete_weak_shared_func_num;
  ir::func_num_t validate_weak_shared_func_num;
};

void LowerSharedPointersInFunc(const LoweringFunctions& lowering_functions, ir::Func* func) {
  std::unordered_map<ir::value_num_t, DecomposedShared> decomposed_shared_pointers;
  func->ForBlocksInDominanceOrder([&](ir::Block* block) {
    for (auto it = block->instrs().begin(); it != block->instrs().end(); ++it) {
      ir::Instr* old_instr = it->get();
      switch (old_instr->instr_kind()) {
        case ir::InstrKind::kLangMakeSharedPointer: {
          auto make_shared_instr = static_cast<ir_ext::MakeSharedPointerInstr*>(old_instr);
          ir::value_num_t shared_pointer_num = make_shared_instr->result()->number();
          DecomposedShared decomposed{
              .control_block_pointer =
                  std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
              .underlying_pointer =
                  std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
          };

          it = block->instrs().erase(it);
          it = block->instrs().insert(
              it, CallMakeSharedFunc(lowering_functions.make_shared_func_num, decomposed));
          decomposed_shared_pointers.emplace(shared_pointer_num, decomposed);
          break;
        }
        case ir::InstrKind::kLangCopySharedPointer: {
          auto copy_shared_instr = static_cast<ir_ext::CopySharedPointerInstr*>(old_instr);
          ir::value_num_t copied_shared_pointer_num =
              copy_shared_instr->copied_shared_pointer()->number();
          ir::value_num_t result_shared_pointer_num = copy_shared_instr->result()->number();
          DecomposedShared decomposed_copied =
              decomposed_shared_pointers.at(copied_shared_pointer_num);
          DecomposedShared decomposed_result{
              .control_block_pointer = decomposed_copied.control_block_pointer,
              .underlying_pointer =
                  std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
          };
          std::shared_ptr<ir::Value> offset = copy_shared_instr->underlying_pointer_offset();

          ir::func_num_t func_num =
              static_cast<const ir_ext::SharedPointer*>(copy_shared_instr->result()->type())
                      ->is_strong()
                  ? lowering_functions.strong_copy_shared_func_num
                  : lowering_functions.weak_copy_shared_func_num;

          it = block->instrs().erase(it);
          it = block->instrs().insert(
              it, CallCopySharedFunc(func_num, decomposed_result, decomposed_copied, offset));

          decomposed_shared_pointers.emplace(result_shared_pointer_num, decomposed_result);
          break;
        }
        case ir::InstrKind::kLangDeleteSharedPointer: {
          auto delete_shared_instr = static_cast<ir_ext::DeleteSharedPointerInstr*>(old_instr);
          ir::value_num_t deleted_shared_pointer_num =
              delete_shared_instr->deleted_shared_pointer()->number();
          DecomposedShared decomposed_deleted =
              decomposed_shared_pointers.at(deleted_shared_pointer_num);

          ir::func_num_t func_num = static_cast<const ir_ext::SharedPointer*>(
                                        delete_shared_instr->deleted_shared_pointer()->type())
                                            ->is_strong()
                                        ? lowering_functions.delete_strong_shared_func_num
                                        : lowering_functions.delete_weak_shared_func_num;

          it = block->instrs().erase(it);
          it = block->instrs().insert(it, CallDeleteSharedFunc(func_num, decomposed_deleted));
          break;
        }
        case ir::InstrKind::kLoad: {
          auto load_shared_instr = static_cast<ir::LoadInstr*>(old_instr);
          if (load_shared_instr->address()->type()->type_kind() !=
              ir::TypeKind::kLangSharedPointer) {
            break;
          }
          ir::value_num_t accessed_shared_pointer_num =
              static_cast<ir::Computed*>(load_shared_instr->address().get())->number();
          DecomposedShared decomposed_accessed =
              decomposed_shared_pointers.at(accessed_shared_pointer_num);
          std::shared_ptr<ir::Computed> result = load_shared_instr->result();

          bool is_strong =
              static_cast<const ir_ext::SharedPointer*>(load_shared_instr->address()->type())
                  ->is_strong();

          it = block->instrs().erase(it);
          if (!is_strong) {
            it = block->instrs().insert(
                it, CallValidateWeakSharedFunc(lowering_functions.validate_weak_shared_func_num,
                                               decomposed_accessed));
          }
          it = block->instrs().insert(
              it, std::make_unique<ir::LoadInstr>(result, decomposed_accessed.underlying_pointer));
          break;
        }
        case ir::InstrKind::kStore: {
          auto store_shared_instr = static_cast<ir::StoreInstr*>(old_instr);
          if (store_shared_instr->address()->type()->type_kind() !=
              ir::TypeKind::kLangSharedPointer) {
            break;
          }
          ir::value_num_t accessed_shared_pointer_num =
              static_cast<ir::Computed*>(store_shared_instr->address().get())->number();
          DecomposedShared decomposed_accessed =
              decomposed_shared_pointers.at(accessed_shared_pointer_num);
          std::shared_ptr<ir::Value> value = store_shared_instr->value();

          bool is_strong =
              static_cast<const ir_ext::SharedPointer*>(store_shared_instr->address()->type())
                  ->is_strong();

          it = block->instrs().erase(it);
          if (!is_strong) {
            it = block->instrs().insert(
                it, CallValidateWeakSharedFunc(lowering_functions.validate_weak_shared_func_num,
                                               decomposed_accessed));
          }
          it = block->instrs().insert(
              it, std::make_unique<ir::StoreInstr>(decomposed_accessed.underlying_pointer, value));
          break;
        }
        default:
          continue;
      }
    }
  });
}

}  // namespace

void LowerSharedPointersInProgram(ir::Program* program) {
  LoweringFunctions lowering_funcs{
      .make_shared_func_num = BuildMakeSharedFunc(program),
      .strong_copy_shared_func_num = BuildCopySharedFunc(program, /*copy_is_strong=*/true),
      .weak_copy_shared_func_num = BuildCopySharedFunc(program, /*copy_is_strong=*/false),
      .delete_strong_shared_func_num = BuildDeleteSharedFunc(program, /*pointer_is_strong=*/true),
      .delete_weak_shared_func_num = BuildDeleteSharedFunc(program, /*pointer_is_strong=*/false),
      .validate_weak_shared_func_num = BuildValidateWeakSharedFunc(program),
  };

  for (auto& func : program->funcs()) {
    LowerSharedPointersInFunc(lowering_funcs, func.get());
  }
}

}  // namespace ir_lowerers
}  // namespace lang
