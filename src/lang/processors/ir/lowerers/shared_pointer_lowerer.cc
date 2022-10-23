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

#include "src/lang/processors/ir/lowerers/shared_pointer_impl.h"

namespace lang {
namespace ir_lowerers {
namespace {

struct DecomposedShared {
  std::shared_ptr<ir::Computed> control_block_pointer;
  std::shared_ptr<ir::Computed> underlying_pointer;
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

std::shared_ptr<ir::Value> DestructorForType(ir::Program* program, const ir::Type* type,
                                             const SharedPointerLoweringFuncs& lowering_funcs) {
  switch (type->type_kind()) {
    case ir::TypeKind::kLangSharedPointer:
      return static_cast<const ir_ext::SharedPointer*>(type)->is_strong()
                 ? ir::ToFuncConstant(lowering_funcs.delete_ptr_to_strong_shared_func_num)
                 : ir::ToFuncConstant(lowering_funcs.delete_ptr_to_weak_shared_func_num);
    case ir::TypeKind::kLangUniquePointer:
    default:
      return ir::NilFunc();
  }
}

void LowerMakeSharedPointerInstr(
    ir::Program* program, ir::Func* func, ir::Block* block,
    std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers,
    const SharedPointerLoweringFuncs& lowering_funcs) {
  auto make_shared_instr = static_cast<ir_ext::MakeSharedPointerInstr*>(it->get());
  const ir_ext::SharedPointer* shared_pointer_type = make_shared_instr->pointer_type();
  ir::value_num_t shared_pointer_num = make_shared_instr->result()->number();
  DecomposedShared decomposed{
      .control_block_pointer =
          std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
      .underlying_pointer =
          std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
  };
  std::shared_ptr<ir::Value> destructor =
      DestructorForType(program, shared_pointer_type->element(), lowering_funcs);
  auto call_instr = std::make_unique<ir::CallInstr>(
      ir::ToFuncConstant(lowering_funcs.make_shared_func_num),
      std::vector<std::shared_ptr<ir::Computed>>{decomposed.control_block_pointer,
                                                 decomposed.underlying_pointer},
      std::vector<std::shared_ptr<ir::Value>>{
          ir::ToIntConstant(common::Int(shared_pointer_type->element()->size())),
          make_shared_instr->size(), destructor});

  it = block->instrs().erase(it);
  it = block->instrs().insert(it, std::move(call_instr));
  decomposed_shared_pointers.emplace(shared_pointer_num, decomposed);
}

void LowerCopySharedPointerInstr(
    ir::Func* func, ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers,
    const SharedPointerLoweringFuncs& lowering_funcs) {
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

  auto call_instr = std::make_unique<ir::CallInstr>(
      ir::ToFuncConstant(func_num),
      std::vector<std::shared_ptr<ir::Computed>>{decomposed_result.underlying_pointer},
      std::vector<std::shared_ptr<ir::Value>>{decomposed_copied.control_block_pointer,
                                              decomposed_copied.underlying_pointer, offset});

  it = block->instrs().erase(it);
  it = block->instrs().insert(it, std::move(call_instr));

  decomposed_shared_pointers.emplace(result_shared_pointer_num, decomposed_result);
}

void LowerDeleteSharedPointerInstr(
    ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers,
    const SharedPointerLoweringFuncs& lowering_funcs) {
  auto delete_shared_instr = static_cast<ir_ext::DeleteSharedPointerInstr*>(it->get());
  ir::value_num_t deleted_shared_pointer_num =
      delete_shared_instr->deleted_shared_pointer()->number();
  DecomposedShared& decomposed_deleted = decomposed_shared_pointers.at(deleted_shared_pointer_num);

  ir::func_num_t func_num = static_cast<const ir_ext::SharedPointer*>(
                                delete_shared_instr->deleted_shared_pointer()->type())
                                    ->is_strong()
                                ? lowering_funcs.delete_strong_shared_func_num
                                : lowering_funcs.delete_weak_shared_func_num;

  auto call_instr = std::make_unique<ir::CallInstr>(
      ir::ToFuncConstant(func_num), std::vector<std::shared_ptr<ir::Computed>>{},
      std::vector<std::shared_ptr<ir::Value>>{decomposed_deleted.control_block_pointer});

  it = block->instrs().erase(it);
  it = block->instrs().insert(it, std::move(call_instr));
}

void LowerLoadValueFromSharedPointerInstr(
    ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers,
    const SharedPointerLoweringFuncs& lowering_funcs) {
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
    auto call_instr = std::make_unique<ir::CallInstr>(
        ir::ToFuncConstant(lowering_funcs.validate_weak_shared_func_num),
        std::vector<std::shared_ptr<ir::Computed>>{},
        std::vector<std::shared_ptr<ir::Value>>{decomposed_accessed.control_block_pointer});
    it = block->instrs().insert(it, std::move(call_instr));
  }
  it = block->instrs().insert(
      it, std::make_unique<ir::LoadInstr>(result, decomposed_accessed.underlying_pointer));
}

void LowerStoreValueInSharedPointerInstr(
    ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers,
    const SharedPointerLoweringFuncs& lowering_funcs) {
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
    auto call_instr = std::make_unique<ir::CallInstr>(
        ir::ToFuncConstant(lowering_funcs.validate_weak_shared_func_num),
        std::vector<std::shared_ptr<ir::Computed>>{},
        std::vector<std::shared_ptr<ir::Value>>{decomposed_accessed.control_block_pointer});
    it = block->instrs().insert(it, std::move(call_instr));
  }
  it = block->instrs().insert(
      it, std::make_unique<ir::StoreInstr>(decomposed_accessed.underlying_pointer, value));
}

void LowerLoadOfSharedPointerAsValueInstr(
    ir::Func* func, ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers) {
  auto load_shared_instr = static_cast<ir::LoadInstr*>(it->get());
  if (load_shared_instr->result()->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    return;
  }
  ir::value_num_t shared_pointer_num = load_shared_instr->result()->number();
  DecomposedShared decomposed{
      .control_block_pointer =
          std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
      .underlying_pointer =
          std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number()),
  };
  std::shared_ptr<ir::Computed> address_of_control_block_pointer =
      std::static_pointer_cast<ir::Computed>(load_shared_instr->address());
  std::shared_ptr<ir::Computed> address_of_underlying_pointer =
      std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number());

  it = block->instrs().erase(it);
  it =
      block->instrs().insert(it, std::make_unique<ir::LoadInstr>(decomposed.control_block_pointer,
                                                                 address_of_control_block_pointer));
  ++it;
  it = block->instrs().insert(
      it, std::make_unique<ir::PointerOffsetInstr>(
              address_of_underlying_pointer, address_of_control_block_pointer, ir::I64Eight()));
  ++it;
  it = block->instrs().insert(it, std::make_unique<ir::LoadInstr>(decomposed.underlying_pointer,
                                                                  address_of_underlying_pointer));
  decomposed_shared_pointers.emplace(shared_pointer_num, decomposed);
}

void LowerStoreOfSharedPointerAsValueInstr(
    ir::Func* func, ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers) {
  auto store_shared_instr = static_cast<ir::StoreInstr*>(it->get());
  std::shared_ptr<ir::Value> control_block_pointer;
  std::shared_ptr<ir::Value> underlying_pointer;
  if (ir::IsEqual(store_shared_instr->value().get(), ir::NilPointer().get())) {
    control_block_pointer = ir::NilPointer();
    underlying_pointer = ir::NilPointer();
  } else if (store_shared_instr->value()->type()->type_kind() == ir::TypeKind::kLangSharedPointer) {
    ir::value_num_t shared_pointer_num =
        static_cast<ir::Computed*>(store_shared_instr->value().get())->number();
    DecomposedShared& decomposed = decomposed_shared_pointers.at(shared_pointer_num);
    control_block_pointer = decomposed.control_block_pointer;
    underlying_pointer = decomposed.underlying_pointer;
  } else {
    return;
  }
  std::shared_ptr<ir::Computed> address_of_control_block_pointer =
      std::static_pointer_cast<ir::Computed>(store_shared_instr->address());
  std::shared_ptr<ir::Computed> address_of_underlying_pointer =
      std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number());

  it = block->instrs().erase(it);
  it = block->instrs().insert(it, std::make_unique<ir::StoreInstr>(address_of_control_block_pointer,
                                                                   control_block_pointer));
  ++it;
  it = block->instrs().insert(
      it, std::make_unique<ir::PointerOffsetInstr>(
              address_of_underlying_pointer, address_of_control_block_pointer, ir::I64Eight()));
  ++it;
  it = block->instrs().insert(
      it, std::make_unique<ir::StoreInstr>(address_of_underlying_pointer, underlying_pointer));
}

void LowerMovSharedPointerInstr(
                                ir::Func* func, ir::Block* block, std::vector<std::unique_ptr<ir::Instr>>::iterator& it,
    std::unordered_map<ir::value_num_t, DecomposedShared>& decomposed_shared_pointers) {
  auto mov_instr = static_cast<ir::MovInstr*>(it->get());
  if (mov_instr->result()->type()->type_kind() != ir::TypeKind::kLangSharedPointer) {
    return;
  }
  
  ir::value_num_t result_shared_pointer_num = mov_instr->result()->number();
  if (ir::IsEqual(mov_instr->origin().get(), ir::NilPointer().get())) {
    std::shared_ptr<ir::Computed> control_block_pointer =
      std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number());
    std::shared_ptr<ir::Computed> underlying_pointer =
      std::make_shared<ir::Computed>(ir::pointer_type(), func->next_computed_number());
    DecomposedShared decomposed{
      .control_block_pointer = control_block_pointer,
      .underlying_pointer = underlying_pointer,
    };

    it = block->instrs().erase(it);
    --it;
    it = block->instrs().insert(it, std::make_unique<ir::MovInstr>(control_block_pointer, ir::NilPointer()));
    it = block->instrs().insert(it, std::make_unique<ir::MovInstr>(underlying_pointer, ir::NilPointer()));
    decomposed_shared_pointers.emplace(result_shared_pointer_num, decomposed);
    
  } else {
    ir::value_num_t origin_shared_pointer_num =
    static_cast<ir::Computed*>(mov_instr->origin().get())->number();
    DecomposedShared& decomposed_origin = decomposed_shared_pointers.at(origin_shared_pointer_num);
    it = block->instrs().erase(it);
    --it;
    decomposed_shared_pointers.emplace(result_shared_pointer_num, decomposed_origin);
  }
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

void LowerSharedPointersInFunc(ir::Program* program, ir::Func* func,
                               const SharedPointerLoweringFuncs& lowering_funcs) {
  std::unordered_map<ir::value_num_t, DecomposedShared> decomposed_shared_pointers;
  std::vector<PhiInstrLoweringInfo> phi_instrs_lowering_info;
  LowerSharedPointerArgsOfFunc(func, decomposed_shared_pointers);
  LowerSharedPointerResultsOfFunc(func);
  func->ForBlocksInDominanceOrder([&](ir::Block* block) {
    for (auto it = block->instrs().begin(); it != block->instrs().end(); ++it) {
      ir::Instr* old_instr = it->get();
      switch (old_instr->instr_kind()) {
        case ir::InstrKind::kLangMakeSharedPointer:
          LowerMakeSharedPointerInstr(program, func, block, it, decomposed_shared_pointers,
                                      lowering_funcs);
          break;
        case ir::InstrKind::kLangCopySharedPointer:
          LowerCopySharedPointerInstr(func, block, it, decomposed_shared_pointers, lowering_funcs);
          break;
        case ir::InstrKind::kLangDeleteSharedPointer:
          LowerDeleteSharedPointerInstr(block, it, decomposed_shared_pointers, lowering_funcs);
          break;
        case ir::InstrKind::kLoad:
          LowerLoadValueFromSharedPointerInstr(block, it, decomposed_shared_pointers,
                                               lowering_funcs);
          LowerLoadOfSharedPointerAsValueInstr(func, block, it, decomposed_shared_pointers);
          break;
        case ir::InstrKind::kStore:
          LowerStoreValueInSharedPointerInstr(block, it, decomposed_shared_pointers,
                                              lowering_funcs);
          LowerStoreOfSharedPointerAsValueInstr(func, block, it, decomposed_shared_pointers);
          break;
        case ir::InstrKind::kMov:
          LowerMovSharedPointerInstr(func, block, it, decomposed_shared_pointers);
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
  SharedPointerLoweringFuncs lowering_funcs = AddSharedPointerLoweringFuncsToProgram(program);

  for (auto& func : program->funcs()) {
    LowerSharedPointersInFunc(program, func.get(), lowering_funcs);
  }
}

}  // namespace ir_lowerers
}  // namespace lang
