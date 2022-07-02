//
//  shared_pointer_lowerer.cc
//  Katara
//
//  Created by Arne Philipeit on 7/29/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "shared_pointer_lowerer.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

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

struct LoweringFuncs {
  ir::func_num_t make_shared_func_num;
  ir::func_num_t strong_copy_shared_func_num;
  ir::func_num_t weak_copy_shared_func_num;
  ir::func_num_t delete_strong_shared_func_num;
  ir::func_num_t delete_weak_shared_func_num;
  ir::func_num_t validate_weak_shared_func_num;
};

void LowerSharedPointerArgsOfFunc(
    ir::Func* func,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers) {
  for (auto it = func->args().begin(); it != func->args().end(); ++it) {
    if ((*it)->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
      continue;
    }
    ir::value_num_t shared_ptr_num = (*it)->number();
    DecomposedShared decomposed{
        .control_block_pointer =
            std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
        .underlying_pointer =
            std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
    };
    it = func->args().erase(it);
    it = func->args().insert(it, decomposed.control_block_pointer);
    ++it;
    it = func->args().insert(it, decomposed.underlying_pointer);
    decomposed_shared_pointers.emplace(shared_ptr_num, decomposed);
  }
}

void LowerSharedPointerResultsOfFunc(ir::Func* func) {
  for (auto it = func->result_types().begin(); it != func->result_types().end(); ++it) {
    if ((*it)->type_kind() != ir::TypeKind::kLangSharedPointer) {
      return;
    }
    it = func->result_types().erase(it);
    it = func->result_types().insert(it, ir::pointer_type());
    ++it;
    it = func->result_types().insert(it, ir::pointer_type());
  }
}

void LowerMakeSharedPointerInstr(
    ir::Func* func, ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers,
    const LoweringFuncs& lowering_funcs) {
  auto make_shared_instr = static_cast<ir_ext::MakeSharedPointerInstr*>(it->get());
  ir::value_num_t shared_pointer_num = make_shared_instr->result()->number();
  DecomposedShared decomposed{
      .control_block_pointer =
          std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
      .underlying_pointer =
          std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
  };

  it = block->instrs().erase(it);
  it = block->instrs().insert(it,
                              CallMakeSharedFunc(lowering_funcs.make_shared_func_num, decomposed));
  decomposed_shared_pointers.emplace(shared_pointer_num, decomposed);
}

void LowerCopySharedPointerInstr(
    ir::Func* func, ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers,
    const LoweringFuncs& lowering_funcs) {
  auto copy_shared_instr = static_cast<ir_ext::CopySharedPointerInstr*>(it->get());
  ir::value_num_t copied_shared_pointer_num = copy_shared_instr->copied_shared_pointer()->number();
  ir::value_num_t result_shared_pointer_num = copy_shared_instr->result()->number();
  DecomposedShared decomposed_copied = decomposed_shared_pointers.at(copied_shared_pointer_num);
  DecomposedShared decomposed_result{
      .control_block_pointer = decomposed_copied.control_block_pointer,
      .underlying_pointer =
          std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
  };
  std::shared_ptr<ir::Value> offset = copy_shared_instr->underlying_pointer_offset();

  ir::func_num_t func_num =
      static_cast<const ir_ext::SharedPointer*>(copy_shared_instr->result()->type())->is_strong()
          ? lowering_funcs.strong_copy_shared_func_num
          : lowering_funcs.weak_copy_shared_func_num;

  it = block->instrs().erase(it);
  it = block->instrs().insert(
      it, CallCopySharedFunc(func_num, decomposed_result, decomposed_copied, offset));

  decomposed_shared_pointers.emplace(result_shared_pointer_num, decomposed_result);
}

void LowerDeleteSharedPointerInstr(
    ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers,
    const LoweringFuncs& lowering_funcs) {
  auto delete_shared_instr = static_cast<ir_ext::DeleteSharedPointerInstr*>(it->get());
  ir::value_num_t deleted_shared_pointer_num =
      delete_shared_instr->deleted_shared_pointer()->number();
  DecomposedShared& decomposed_deleted = decomposed_shared_pointers.at(deleted_shared_pointer_num);

  ir::func_num_t func_num = static_cast<const ir_ext::SharedPointer*>(
                                delete_shared_instr->deleted_shared_pointer()->type())
                                    ->is_strong()
                                ? lowering_funcs.delete_strong_shared_func_num
                                : lowering_funcs.delete_weak_shared_func_num;

  it = block->instrs().erase(it);
  it = block->instrs().insert(it, CallDeleteSharedFunc(func_num, decomposed_deleted));
}

void LowerLoadFromSharedPointerInstr(
    ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers,
    const LoweringFuncs& lowering_funcs) {
  auto load_shared_instr = static_cast<ir::LoadInstr*>(it->get());
  if (load_shared_instr->address()->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    return;
  }
  ir::value_num_t accessed_shared_pointer_num =
      static_cast<ir::Computed*>(load_shared_instr->address().get())->number();
  DecomposedShared& decomposed_accessed =
      decomposed_shared_pointers.at(accessed_shared_pointer_num);
  std::shared_ptr<ir::Computed> result = load_shared_instr->result();

  bool is_strong =
      static_cast<const ir_ext::SharedPointer*>(load_shared_instr->address()->type())->is_strong();

  it = block->instrs().erase(it);
  if (!is_strong) {
    it = block->instrs().insert(
        it, CallValidateWeakSharedFunc(lowering_funcs.validate_weak_shared_func_num,
                                       decomposed_accessed));
  }
  it = block->instrs().insert(
      it, std::make_unique<ir::LoadInstr>(result, decomposed_accessed.underlying_pointer));
}

void LowerStoreInSharedPointerInstr(
    ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers,
    const LoweringFuncs& lowering_funcs) {
  auto store_shared_instr = static_cast<ir::StoreInstr*>(it->get());
  if (store_shared_instr->address()->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    return;
  }
  ir::value_num_t accessed_shared_pointer_num =
      static_cast<ir::Computed*>(store_shared_instr->address().get())->number();
  DecomposedShared& decomposed_accessed =
      decomposed_shared_pointers.at(accessed_shared_pointer_num);
  std::shared_ptr<ir::Value> value = store_shared_instr->value();

  bool is_strong =
      static_cast<const ir_ext::SharedPointer*>(store_shared_instr->address()->type())->is_strong();

  it = block->instrs().erase(it);
  if (!is_strong) {
    it = block->instrs().insert(
        it, CallValidateWeakSharedFunc(lowering_funcs.validate_weak_shared_func_num,
                                       decomposed_accessed));
  }
  it = block->instrs().insert(
      it, std::make_unique<ir::StoreInstr>(decomposed_accessed.underlying_pointer, value));
}

void LowerMovSharedPointerInstr(
    ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers) {
  auto mov_instr = static_cast<ir::MovInstr*>(it->get());
  if (mov_instr->result()->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    return;
  }
  ir::value_num_t origin_shared_pointer_num =
      static_cast<ir::Computed*>(mov_instr->origin().get())->number();
  ir::value_num_t result_shared_pointer_num = mov_instr->result()->number();
  DecomposedShared& decomposed_origin = decomposed_shared_pointers.at(origin_shared_pointer_num);
  it = block->instrs().erase(it);
  --it;
  decomposed_shared_pointers.emplace(result_shared_pointer_num, decomposed_origin);
}

struct PhiInstrLoweringInfo {
  ir::value_num_t result_shared_pointer_num;
  std::unordered_map<ir::block_num_t, ir::value_num_t> arg_shared_pointer_nums;
  ir::PhiInstr* control_block_pointer_phi_instr;
  ir::PhiInstr* underlying_pointer_phi_instr;
};

std::optional<PhiInstrLoweringInfo> LowerSharedPointerDefinitionsInPhiInstr(
    ir::Func* func, ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers) {
  auto old_phi_instr = static_cast<ir::PhiInstr*>(it->get());
  if (old_phi_instr->result()->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    return std::nullopt;
  }
  PhiInstrLoweringInfo info;
  info.result_shared_pointer_num = old_phi_instr->result()->number();
  for (std::shared_ptr<ir::InheritedValue>& arg : old_phi_instr->args()) {
    info.arg_shared_pointer_nums.try_emplace(
        arg->origin(), static_cast<ir::Computed*>(arg->value().get())->number());
  }
  DecomposedShared decomposed_result{
      .control_block_pointer =
          std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
      .underlying_pointer =
          std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
  };

  it = block->instrs().erase(it);
  it = block->instrs().insert(
      it, std::make_unique<ir::PhiInstr>(decomposed_result.control_block_pointer,
                                         std::vector<std::shared_ptr<ir::InheritedValue>>{}));
  info.control_block_pointer_phi_instr = static_cast<ir::PhiInstr*>(it->get());
  ++it;
  it = block->instrs().insert(
      it, std::make_unique<ir::PhiInstr>(decomposed_result.underlying_pointer,
                                         std::vector<std::shared_ptr<ir::InheritedValue>>{}));
  info.underlying_pointer_phi_instr = static_cast<ir::PhiInstr*>(it->get());
  decomposed_shared_pointers.emplace(info.result_shared_pointer_num, decomposed_result);
  return info;
}

void LowerSharedPointerArgsForPhiInstr(
    PhiInstrLoweringInfo& info,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers) {
  for (auto& [origin, arg_shared_pointer_num] : info.arg_shared_pointer_nums) {
    DecomposedShared& decomposed_result = decomposed_shared_pointers.at(arg_shared_pointer_num);
    info.control_block_pointer_phi_instr->args().push_back(
        std::make_shared<ir::InheritedValue>(decomposed_result.control_block_pointer, origin));
    info.underlying_pointer_phi_instr->args().push_back(
        std::make_shared<ir::InheritedValue>(decomposed_result.underlying_pointer, origin));
  }
}

void LowerSharedPointersInCallInstr(
    ir::Func* func, ir::CallInstr* call_instr,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers) {
  for (auto it = call_instr->args().begin(); it != call_instr->args().end(); ++it) {
    ir::Value* old_arg = it->get();
    if (old_arg->kind() != ir::Value::Kind::kComputed ||
        old_arg->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
      continue;
    }
    ir::value_num_t arg_shared_pointer_num = static_cast<ir::Computed*>(old_arg)->number();
    DecomposedShared& decomposed_arg = decomposed_shared_pointers.at(arg_shared_pointer_num);
    it = call_instr->args().erase(it);
    it = call_instr->args().insert(it, decomposed_arg.control_block_pointer);
    ++it;
    it = call_instr->args().insert(it, decomposed_arg.underlying_pointer);
  }
  for (auto it = call_instr->results().begin(); it != call_instr->results().end(); ++it) {
    ir::Computed* old_result = it->get();
    if (old_result->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
      continue;
    }
    ir::value_num_t result_shared_pointer_num = old_result->number();
    DecomposedShared decomposed{
        .control_block_pointer =
            std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
        .underlying_pointer =
            std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
    };

    it = call_instr->results().erase(it);
    it = call_instr->results().insert(it, decomposed.control_block_pointer);
    ++it;
    it = call_instr->results().insert(it, decomposed.underlying_pointer);
    decomposed_shared_pointers.emplace(result_shared_pointer_num, decomposed);
  }
}

void LowerSharedPointersInReturnInstr(
    ir::ReturnInstr* return_instr,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers) {
  for (auto it = return_instr->args().begin(); it != return_instr->args().end(); ++it) {
    ir::Value* old_arg = it->get();
    if (old_arg->kind() != ir::Value::Kind::kComputed ||
        old_arg->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
      continue;
    }
    ir::value_num_t arg_shared_pointer_num = static_cast<ir::Computed*>(old_arg)->number();
    DecomposedShared& decomposed_arg = decomposed_shared_pointers.at(arg_shared_pointer_num);
    it = return_instr->args().erase(it);
    it = return_instr->args().insert(it, decomposed_arg.control_block_pointer);
    ++it;
    it = return_instr->args().insert(it, decomposed_arg.underlying_pointer);
  }
}

void LowerSharedPointersInFunc(const LoweringFuncs& lowering_funcs, ir::Func* func) {
  std::unordered_map<ir::value_num_t, DecomposedShared> decomposed_shared_pointers;
  std::vector<PhiInstrLoweringInfo> phi_instrs_lowering_info;
  LowerSharedPointerArgsOfFunc(func, decomposed_shared_pointers);
  LowerSharedPointerResultsOfFunc(func);
  // TODO: implement lowering of load/store results that are shared pointers
  func->ForBlocksInDominanceOrder([&](ir::Block* block) {
    for (auto it = block->instrs().begin(); it != block->instrs().end(); ++it) {
      ir::Instr* old_instr = it->get();
      switch (old_instr->instr_kind()) {
        case ir::InstrKind::kLangMakeSharedPointer:
          LowerMakeSharedPointerInstr(func, block, it, decomposed_shared_pointers, lowering_funcs);
          break;
        case ir::InstrKind::kLangCopySharedPointer:
          LowerCopySharedPointerInstr(func, block, it, decomposed_shared_pointers, lowering_funcs);
          break;
        case ir::InstrKind::kLangDeleteSharedPointer:
          LowerDeleteSharedPointerInstr(block, it, decomposed_shared_pointers, lowering_funcs);
          break;
        case ir::InstrKind::kLoad:
          LowerLoadFromSharedPointerInstr(block, it, decomposed_shared_pointers, lowering_funcs);
          break;
        case ir::InstrKind::kStore:
          LowerStoreInSharedPointerInstr(block, it, decomposed_shared_pointers, lowering_funcs);
          break;
        case ir::InstrKind::kMov:
          LowerMovSharedPointerInstr(block, it, decomposed_shared_pointers);
          break;
        case ir::InstrKind::kPhi:
          if (std::optional<PhiInstrLoweringInfo> info = LowerSharedPointerDefinitionsInPhiInstr(
                  func, block, it, decomposed_shared_pointers);
              info.has_value()) {
            phi_instrs_lowering_info.push_back(*info);
          }
          break;
        case ir::InstrKind::kCall:
          LowerSharedPointersInCallInstr(func, static_cast<ir::CallInstr*>(old_instr),
                                         decomposed_shared_pointers);
          break;
        case ir::InstrKind::kReturn:
          LowerSharedPointersInReturnInstr(static_cast<ir::ReturnInstr*>(old_instr),
                                           decomposed_shared_pointers);
          break;
        default:
          continue;
      }
    }
  });
  for (PhiInstrLoweringInfo& info : phi_instrs_lowering_info) {
    LowerSharedPointerArgsForPhiInstr(info, decomposed_shared_pointers);
  }
}

}  // namespace

void LowerSharedPointersInProgram(ir::Program* program) {
  LoweringFuncs lowering_funcs{
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
