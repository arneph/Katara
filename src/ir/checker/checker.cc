//
//  checker.cc
//  Katara
//
//  Created by Arne Philipeit on 2/20/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "checker.h"

#include <sstream>

#include "src/common/logging/logging.h"

namespace ir_checker {

std::vector<Issue> CheckProgram(const ir::Program* program) {
  Checker checker(program);
  checker.CheckProgram();
  return checker.issues();
}

void AssertProgramIsOkay(const ir::Program* program) {
  std::vector<Issue> issues = CheckProgram(program);
  if (issues.empty()) {
    return;
  }
  std::stringstream buf;
  buf << "IR checker found issues:\n";
  for (const Issue& issue : issues) {
    buf << "[" << int64_t(issue.kind()) << "] " << issue.message() << "\n";
    buf << "\tScope: " << issue.scope_object()->RefString() << "\n";
    if (!issue.involved_objects().empty()) {
      buf << "\tInvolved Objects:\n";
      for (const ir::Object* object : issue.involved_objects()) {
        buf << "\t\t" << object->RefString() << "\n";
      }
    }
  }
  common::fail(buf.str());
}

void Checker::CheckProgram() {
  for (const std::unique_ptr<ir::Func>& func : program_->funcs()) {
    CheckFunc(func.get());
  }
}

void Checker::CheckFunc(const ir::Func* func) {
  CheckValuesInFunc(func);
  if (func->entry_block_num() == ir::kNoBlockNum) {
    AddIssue(Issue(func, Issue::Kind::kFuncHasNoEntryBlock, "ir::Func has no set entry block"));
  }
  for (const std::unique_ptr<ir::Block>& block : func->blocks()) {
    CheckBlock(block.get(), func);
  }
  for (const ir::Type* type : func->result_types()) {
    if (type == nullptr) {
      AddIssue(
          Issue(func, Issue::Kind::kFuncHasNullptrResultType, "ir::Func has nullptr result type"));
    }
  }
}

void Checker::AddValueInFunc(const ir::Computed* value, const ir::Func* func,
                             FuncValues& func_values) {
  CheckValue(value);

  // Check and update computed association with func:
  if (values_to_funcs_.contains(value) && values_to_funcs_.at(value) != func) {
    AddIssue(Issue(program_, {value, func, values_to_funcs_.at(value)},
                   Issue::Kind::kComputedValueUsedInMultipleFunctions,
                   "ir::Computed instance gets used in multiple functions"));
  } else {
    values_to_funcs_.insert({value, func});
  }

  // Check and update value number association with ir::Computed instance:
  if (func_values.pointers.contains(value->number()) &&
      func_values.pointers.at(value->number()) != value) {
    AddIssue(Issue(func, {value, func_values.pointers.at(value->number())},
                   Issue::Kind::kComputedValueNumberUsedMultipleTimes,
                   "Multiple ir::Computed instances use the same value number"));
  } else {
    func_values.pointers.insert({value->number(), value});
  }
}

void Checker::AddArgsInFunc(const ir::Func* func, FuncValues& func_values) {
  for (const std::shared_ptr<ir::Computed>& arg : func->args()) {
    if (arg == nullptr) {
      AddIssue(Issue(func, Issue::Kind::kFuncDefinesNullptrArg, "ir::Func defines nullptr arg"));
      continue;
    }
    AddValueInFunc(arg.get(), func, func_values);
    if (func_values.args.contains(arg->number())) {
      AddIssue(Issue(func, {arg.get()}, Issue::Kind::kComputedValueHasMultipleDefinitions,
                     "ir::Computed is a repeated function argument"));
    } else {
      func_values.args.insert(arg->number());
    }
  }
}

void Checker::AddDefinitionsInFunc(const ir::Func* func, FuncValues& func_values) {
  for (const std::unique_ptr<ir::Block>& block : func->blocks()) {
    for (std::size_t instr_index = 0; instr_index < block->instrs().size(); instr_index++) {
      const ir::Instr* instr = block->instrs().at(instr_index).get();
      for (const std::shared_ptr<ir::Computed>& defined_value : instr->DefinedValues()) {
        if (defined_value == nullptr) {
          AddIssue(Issue(instr, Issue::Kind::kInstrDefinesNullptrValue,
                         "ir::Instr defines nullptr value"));
          continue;
        }
        AddValueInFunc(defined_value.get(), func, func_values);
        if (func_values.args.contains(defined_value->number())) {
          AddIssue(Issue(func, {defined_value.get(), instr},
                         Issue::Kind::kComputedValueHasMultipleDefinitions,
                         "ir::Computed is a function argument and the result of a computation"));
        } else if (func_values.definitions.contains(defined_value->number())) {
          AddIssue(Issue(func,
                         {defined_value.get(),
                          func_values.definitions.at(defined_value->number()).instr, instr},
                         Issue::Kind::kComputedValueHasMultipleDefinitions,
                         "ir::Computed is the result of multiple computations"));
        } else {
          func_values.definitions.insert({defined_value->number(), FuncValueReference{
                                                                       .block = block.get(),
                                                                       .instr = instr,
                                                                       .instr_index = instr_index,
                                                                   }});
        }
      }
    }
  }
}

void Checker::CheckDefinitionDominatesUse(const FuncValueReference& definition,
                                          const FuncValueReference& use, const ir::Func* func) {
  auto add_issue = [&] {
    AddIssue(Issue(func, {definition.instr, use.instr},
                   Issue::Kind::kComputedValueDefinitionDoesNotDominateUse,
                   "ir::Computed use is not dominated by definition"));
  };
  if (definition.block == use.block) {
    if (definition.instr_index >= use.instr_index) {
      add_issue();
    }
  } else {
    ir::block_num_t current = use.block->number();
    while (current != ir::kNoBlockNum) {
      if (current == definition.block->number()) {
        return;
      }
      current = func->DominatorOf(current);
    }
    add_issue();
  }
}

void Checker::CheckDefinitionDominatesUseInPhi(const FuncValueReference& definition,
                                               const FuncValueReference& use,
                                               const ir::InheritedValue* inherited_value,
                                               const ir::Func* func) {
  const ir::Block* origin_block = func->GetBlock(inherited_value->origin());
  FuncValueReference phi_replacement_use{
      .block = origin_block,
      .instr = use.instr,
      .instr_index = origin_block->instrs().size(),
  };
  CheckDefinitionDominatesUse(definition, phi_replacement_use, func);
}

void Checker::CheckValuesInFunc(const ir::Func* func) {
  FuncValues func_values;
  AddArgsInFunc(func, func_values);
  AddDefinitionsInFunc(func, func_values);

  for (const std::unique_ptr<ir::Block>& block : func->blocks()) {
    for (std::size_t instr_index = 0; instr_index < block->instrs().size(); instr_index++) {
      const ir::Instr* instr = block->instrs().at(instr_index).get();
      for (std::size_t used_value_index = 0; used_value_index < instr->UsedValues().size();
           used_value_index++) {
        const ir::Value* used_value = instr->UsedValues().at(used_value_index).get();
        if (used_value == nullptr) {
          AddIssue(
              Issue(instr, Issue::Kind::kInstrUsesNullptrValue, "ir::Instr uses nullptr value"));
          continue;
        }
        const ir::InheritedValue* inherited_value = nullptr;
        if (instr->instr_kind() == ir::InstrKind::kPhi) {
          inherited_value =
              static_cast<const ir::PhiInstr*>(instr)->args().at(used_value_index).get();
        }
        if (used_value->kind() != ir::Value::Kind::kComputed) {
          CheckValue(used_value);
          continue;
        }
        auto value = static_cast<const ir::Computed*>(used_value);
        if (!func_values.pointers.contains(value->number())) {
          AddIssue(Issue(instr, {value}, Issue::Kind::kComputedValueHasNoDefinition,
                         "ir::Instr uses value without definition"));
        } else if (func_values.pointers.at(value->number()) != value) {
          AddIssue(Issue(func, {value, func_values.pointers.at(value->number())},
                         Issue::Kind::kComputedValueNumberUsedMultipleTimes,
                         "Multiple ir::Computed instances use the same value number"));
        }
        if (auto it = func_values.definitions.find(value->number());
            it != func_values.definitions.end()) {
          FuncValueReference& definition = it->second;
          FuncValueReference use{
              .block = block.get(),
              .instr = instr,
              .instr_index = instr_index,
          };
          if (inherited_value != nullptr) {
            CheckDefinitionDominatesUseInPhi(definition, use, inherited_value, func);
          } else {
            CheckDefinitionDominatesUse(definition, use, func);
          }
        }
      }
    }
  }
}

void Checker::CheckBlock(const ir::Block* block, const ir::Func* func) {
  if (func->entry_block() == block && !block->parents().empty()) {
    AddIssue(Issue(func, {block}, Issue::Kind::kEntryBlockHasParents,
                   "ir::Func has entry block with parents"));
  } else if (func->entry_block() != block && block->parents().empty()) {
    AddIssue(Issue(func, {block}, Issue::Kind::kNonEntryBlockHasNoParents,
                   "ir::Func has non-entry block without parents"));
  }
  if (block->instrs().empty()) {
    AddIssue(Issue(block, Issue::Kind::kBlockContainsNoInstrs,
                   "ir::Block does not contain instructions"));
    return;
  }

  ir::Instr* first_regular_instr = nullptr;
  ir::Instr* last_instr = block->instrs().back().get();
  if (!last_instr->IsControlFlowInstr()) {
    AddIssue(Issue(block, {last_instr}, Issue::Kind::kControlFlowInstrMissingAtEndOfBlock,
                   "ir::Block contains no control flow instruction at the end"));
  }

  for (const std::unique_ptr<ir::Instr>& instr : block->instrs()) {
    if (instr->instr_kind() == ir::InstrKind::kPhi) {
      if (block->parents().size() < 2) {
        AddIssue(Issue(block, {instr.get()}, Issue::Kind::kPhiInBlockWithoutMultipleParents,
                       "ir::Block without multiple parents contains ir::PhiInstr"));
      }
      if (first_regular_instr != nullptr) {
        AddIssue(Issue(block, {first_regular_instr, instr.get()},
                       Issue::Kind::kPhiAfterRegularInstrInBlock,
                       "ir::Block contains ir::PhiInstr after other instruction"));
      }
    } else if (first_regular_instr == nullptr) {
      first_regular_instr = instr.get();
    }
    if (instr->IsControlFlowInstr() && instr.get() != last_instr) {
      AddIssue(Issue(block, {instr.get()}, Issue::Kind::kControlFlowInstrBeforeEndOfBlock,
                     "ir::Block contains control flow instruction before the end"));
    }

    CheckInstr(instr.get(), block, func);
  }
}

void Checker::CheckInstr(const ir::Instr* instr, const ir::Block* block, const ir::Func* func) {
  for (const std::shared_ptr<ir::Value>& used_value : instr->UsedValues()) {
    if (used_value == nullptr) {
      return;
    }
    if (instr->instr_kind() != ir::InstrKind::kPhi &&
        used_value->kind() == ir::Value::Kind::kInherited) {
      AddIssue(Issue(instr, {used_value.get()}, Issue::Kind::kNonPhiInstrUsesInheritedValue,
                     "non-phi ir::Inst uses inherited value"));
    }
  }

  switch (instr->instr_kind()) {
    case ir::InstrKind::kMov:
      CheckMovInstr(static_cast<const ir::MovInstr*>(instr));
      break;
    case ir::InstrKind::kPhi:
      CheckPhiInstr(static_cast<const ir::PhiInstr*>(instr), block, func);
      break;
    case ir::InstrKind::kConversion:
      CheckConversion(static_cast<const ir::Conversion*>(instr));
      break;
    case ir::InstrKind::kBoolNot:
      CheckBoolNotInstr(static_cast<const ir::BoolNotInstr*>(instr));
      break;
    case ir::InstrKind::kBoolBinary:
      CheckBoolBinaryInstr(static_cast<const ir::BoolBinaryInstr*>(instr));
      break;
    case ir::InstrKind::kIntUnary:
      CheckIntUnaryInstr(static_cast<const ir::IntUnaryInstr*>(instr));
      break;
    case ir::InstrKind::kIntCompare:
      CheckIntCompareInstr(static_cast<const ir::IntCompareInstr*>(instr));
      break;
    case ir::InstrKind::kIntBinary:
      CheckIntBinaryInstr(static_cast<const ir::IntBinaryInstr*>(instr));
      break;
    case ir::InstrKind::kIntShift:
      CheckIntShiftInstr(static_cast<const ir::IntShiftInstr*>(instr));
      break;
    case ir::InstrKind::kPointerOffset:
      CheckPointerOffsetInstr(static_cast<const ir::PointerOffsetInstr*>(instr));
      break;
    case ir::InstrKind::kNilTest:
      CheckNilTestInstr(static_cast<const ir::NilTestInstr*>(instr));
      break;
    case ir::InstrKind::kMalloc:
      CheckMallocInstr(static_cast<const ir::MallocInstr*>(instr));
      break;
    case ir::InstrKind::kLoad:
      CheckLoadInstr(static_cast<const ir::LoadInstr*>(instr));
      break;
    case ir::InstrKind::kStore:
      CheckStoreInstr(static_cast<const ir::StoreInstr*>(instr));
      break;
    case ir::InstrKind::kFree:
      CheckFreeInstr(static_cast<const ir::FreeInstr*>(instr));
      break;
    case ir::InstrKind::kJump:
      CheckJumpInstr(static_cast<const ir::JumpInstr*>(instr), block);
      break;
    case ir::InstrKind::kJumpCond:
      CheckJumpCondInstr(static_cast<const ir::JumpCondInstr*>(instr), block);
      break;
    case ir::InstrKind::kCall:
      CheckCallInstr(static_cast<const ir::CallInstr*>(instr));
      break;
    case ir::InstrKind::kReturn:
      CheckReturnInstr(static_cast<const ir::ReturnInstr*>(instr), block, func);
      break;
    default:
      AddIssue(Issue(instr, Issue::Kind::kUnknownInstrKind, "ir::InstrKind is unknown"));
  }
}

void Checker::CheckMovInstr(const ir::MovInstr* mov_instr) {
  if (mov_instr->origin()->type() != mov_instr->result()->type()) {
    AddIssue(Issue(mov_instr, {mov_instr->origin().get(), mov_instr->result().get()},
                   Issue::Kind::kMovInstrOriginAndResultHaveMismatchedTypes,
                   "ir::MovInstr has with mismatched origin and result type"));
  }
}

void Checker::CheckPhiInstr(const ir::PhiInstr* phi_instr, const ir::Block* block,
                            const ir::Func* func) {
  std::unordered_map<ir::block_num_t, int64_t> parent_arg_indices;
  parent_arg_indices.reserve(block->parents().size());
  for (ir::block_num_t parent : block->parents()) {
    parent_arg_indices.insert({parent, -1});
  }

  for (std::size_t i = 0; i < phi_instr->args().size(); i++) {
    const ir::InheritedValue* arg = phi_instr->args().at(i).get();

    if (arg->origin() == ir::kNoBlockNum || !block->parents().contains(arg->origin())) {
      AddIssue(Issue(phi_instr, {arg}, Issue::Kind::kPhiInstrHasArgumentForNonParentBlock,
                     "ir::PhiInstr has arg for non-parent block"));
    } else if (parent_arg_indices.at(arg->origin()) != -1) {
      int64_t prior_arg_index = parent_arg_indices.at(arg->origin());
      AddIssue(Issue(phi_instr, {phi_instr->args().at(prior_arg_index).get(), arg},
                     Issue::Kind::kPhiInstrHasMultipleArgumentsForParentBlock,
                     "ir::PhiInstr has multiple args for the same parent block"));
    } else {
      parent_arg_indices.at(arg->origin()) = int64_t(i);
    }

    if (arg->type() != phi_instr->result()->type()) {
      AddIssue(Issue(phi_instr, {arg, phi_instr->result().get()},
                     Issue::Kind::kPhiInstrArgAndResultHaveMismatchedTypes,
                     "ir::PhiInstr has mismatched arg and result type"));
    }
  }

  for (ir::block_num_t parent : block->parents()) {
    if (parent_arg_indices.at(parent) == -1) {
      AddIssue(Issue(phi_instr, {func->GetBlock(parent)},
                     Issue::Kind::kPhiInstrHasNoArgumentForParentBlock,
                     "ir::PhiInstr has no argument for parent block"));
    }
  }
}

void Checker::CheckConversion(const ir::Conversion* conversion) {
  switch (conversion->operand()->type()->type_kind()) {
    case ir::TypeKind::kBool:
    case ir::TypeKind::kInt:
    case ir::TypeKind::kPointer:
    case ir::TypeKind::kFunc:
      break;
    default:
      AddIssue(Issue(conversion, {conversion->operand().get()},
                     Issue::Kind::kConversionOperandHasUnsupportedType,
                     "ir::Conversion has operand with unsupported type"));
  }
  switch (conversion->result()->type()->type_kind()) {
    case ir::TypeKind::kBool:
    case ir::TypeKind::kInt:
    case ir::TypeKind::kPointer:
    case ir::TypeKind::kFunc:
      break;
    default:
      AddIssue(Issue(conversion, {conversion->result().get()},
                     Issue::Kind::kConversionResultHasUnsupportedType,
                     "ir::Conversion has result with unsupported type"));
  }
}

void Checker::CheckBoolNotInstr(const ir::BoolNotInstr* bool_not_instr) {
  if (bool_not_instr->operand()->type() != ir::bool_type()) {
    AddIssue(Issue(bool_not_instr, {bool_not_instr->operand().get()},
                   Issue::Kind::kBoolNotInstrOperandDoesNotHaveBoolType,
                   "ir::BoolNotInstr operand does not have bool type"));
  }
  if (bool_not_instr->result()->type() != ir::bool_type()) {
    AddIssue(Issue(bool_not_instr, {bool_not_instr->result().get()},
                   Issue::Kind::kBoolNotInstrResultDoesNotHaveBoolType,
                   "ir::BoolNotInstr result does not have bool type"));
  }
}

void Checker::CheckBoolBinaryInstr(const ir::BoolBinaryInstr* bool_binary_instr) {
  auto check_operand = [this, bool_binary_instr](ir::Value* operand) {
    if (operand->type() != ir::bool_type()) {
      AddIssue(Issue(bool_binary_instr, {operand},
                     Issue::Kind::kBoolBinaryInstrOperandDoesNotHaveBoolType,
                     "ir::BoolBinaryInstr operand does not have bool type"));
    }
  };
  check_operand(bool_binary_instr->operand_a().get());
  check_operand(bool_binary_instr->operand_b().get());
  if (bool_binary_instr->result()->type() != ir::bool_type()) {
    AddIssue(Issue(bool_binary_instr, {bool_binary_instr->result().get()},
                   Issue::Kind::kBoolBinaryInstrResultDoesNotHaveBoolType,
                   "ir::BoolBinaryInstr result does not have bool type"));
  }
}

void Checker::CheckIntUnaryInstr(const ir::IntUnaryInstr* int_unary_instr) {
  if (int_unary_instr->operand()->type()->type_kind() != ir::TypeKind::kInt) {
    AddIssue(Issue(int_unary_instr, {int_unary_instr->operand().get()},
                   Issue::Kind::kIntUnaryInstrOperandDoesNotHaveIntType,
                   "ir::IntUnaryInstr operand does not have int type"));
  }
  if (int_unary_instr->result()->type()->type_kind() != ir::TypeKind::kInt) {
    AddIssue(Issue(int_unary_instr, {int_unary_instr->result().get()},
                   Issue::Kind::kIntUnaryInstrResultDoesNotHaveIntType,
                   "ir::IntUnaryInstr result does not have int type"));
  }
  if (int_unary_instr->result()->type() != int_unary_instr->operand()->type()) {
    AddIssue(Issue(int_unary_instr,
                   {int_unary_instr->result().get(), int_unary_instr->operand().get()},
                   Issue::Kind::kIntUnaryInstrResultAndOperandHaveDifferentTypes,
                   "ir::IntUnaryInstr result and operand have different types"));
  }
}

void Checker::CheckIntCompareInstr(const ir::IntCompareInstr* int_compare_instr) {
  auto check_operand = [this, int_compare_instr](ir::Value* operand) {
    if (operand->type()->type_kind() != ir::TypeKind::kInt) {
      AddIssue(Issue(int_compare_instr, {operand},
                     Issue::Kind::kIntCompareInstrOperandDoesNotHaveIntType,
                     "ir::IntCompareInstr operand does not have int type"));
    }
  };
  check_operand(int_compare_instr->operand_a().get());
  check_operand(int_compare_instr->operand_b().get());
  if (int_compare_instr->operand_a()->type() != int_compare_instr->operand_b()->type()) {
    AddIssue(Issue(int_compare_instr,
                   {int_compare_instr->operand_a().get(), int_compare_instr->operand_b().get()},
                   Issue::Kind::kIntCompareInstrOperandsHaveDifferentTypes,
                   "ir::IntCompareInstr operands have different types"));
  }
  if (int_compare_instr->result()->type() != ir::bool_type()) {
    AddIssue(Issue(int_compare_instr, {int_compare_instr->result().get()},
                   Issue::Kind::kIntCompareInstrResultDoesNotHaveBoolType,
                   "ir::IntCompareInstr result does not have bool type"));
  }
}

void Checker::CheckIntBinaryInstr(const ir::IntBinaryInstr* int_binary_instr) {
  auto check_operand = [this, int_binary_instr](ir::Value* operand) {
    if (operand->type()->type_kind() != ir::TypeKind::kInt) {
      AddIssue(Issue(int_binary_instr, {operand},
                     Issue::Kind::kIntBinaryInstrOperandDoesNotHaveIntType,
                     "ir::IntBinaryInstr operand does not have int type"));
    }
  };
  check_operand(int_binary_instr->operand_a().get());
  check_operand(int_binary_instr->operand_b().get());
  if (int_binary_instr->result()->type()->type_kind() != ir::TypeKind::kInt) {
    AddIssue(Issue(int_binary_instr, {int_binary_instr->result().get()},
                   Issue::Kind::kIntBinaryInstrResultDoesNotHaveIntType,
                   "ir::IntBinaryInstr result does not have int type"));
  }
  if (int_binary_instr->result()->type() != int_binary_instr->operand_a()->type() ||
      int_binary_instr->result()->type() != int_binary_instr->operand_b()->type()) {
    AddIssue(Issue(int_binary_instr,
                   {int_binary_instr->result().get(), int_binary_instr->operand_a().get(),
                    int_binary_instr->operand_b().get()},
                   Issue::Kind::kIntBinaryInstrOperandsAndResultHaveDifferentTypes,
                   "ir::IntBinaryInstr operands and result have different types"));
  }
}

void Checker::CheckIntShiftInstr(const ir::IntShiftInstr* int_shift_instr) {
  auto check_operand = [this, int_shift_instr](ir::Value* operand) {
    if (operand->type()->type_kind() != ir::TypeKind::kInt) {
      AddIssue(Issue(int_shift_instr, {operand},
                     Issue::Kind::kIntShiftInstrOperandDoesNotHaveIntType,
                     "ir::IntShiftInstr operand does not have int type"));
    }
  };
  check_operand(int_shift_instr->shifted().get());
  check_operand(int_shift_instr->offset().get());
  if (int_shift_instr->result()->type()->type_kind() != ir::TypeKind::kInt) {
    AddIssue(Issue(int_shift_instr, {int_shift_instr->result().get()},
                   Issue::Kind::kIntShiftInstrResultDoesNotHaveIntType,
                   "ir::IntShiftInstr result does not have int type"));
  }
  if (int_shift_instr->result()->type() != int_shift_instr->shifted()->type()) {
    AddIssue(Issue(int_shift_instr,
                   {int_shift_instr->result().get(), int_shift_instr->shifted().get()},
                   Issue::Kind::kIntShiftInstrShiftedAndResultHaveDifferentTypes,
                   "ir::IntShiftInstr shifted and result have different types"));
  }
}

void Checker::CheckPointerOffsetInstr(const ir::PointerOffsetInstr* pointer_offset_instr) {
  if (pointer_offset_instr->pointer()->type() != ir::pointer_type()) {
    AddIssue(Issue(pointer_offset_instr, {pointer_offset_instr->pointer().get()},
                   Issue::Kind::kPointerOffsetInstrPointerDoesNotHavePointerType,
                   "ir::PointerOffsetInstr pointer does not have pointer type"));
  }
  if (pointer_offset_instr->offset()->type() != ir::i64()) {
    AddIssue(Issue(pointer_offset_instr, {pointer_offset_instr->offset().get()},
                   Issue::Kind::kPointerOffsetInstrOffsetDoesNotHaveI64Type,
                   "ir::PointerOffsetInstr offset does not have I64 type"));
  }
  if (pointer_offset_instr->result()->type() != ir::pointer_type()) {
    AddIssue(Issue(pointer_offset_instr, {pointer_offset_instr->result().get()},
                   Issue::Kind::kPointerOffsetInstrResultDoesNotHavePointerType,
                   "ir::PointerOffsetInstr result does not have pointer type"));
  }
}

void Checker::CheckNilTestInstr(const ir::NilTestInstr* nil_test_instr) {
  if (nil_test_instr->tested()->type() != ir::pointer_type() &&
      nil_test_instr->tested()->type() != ir::func_type()) {
    AddIssue(Issue(nil_test_instr, {nil_test_instr->tested().get()},
                   Issue::Kind::kNilTestInstrTestedDoesNotHavePointerOrFuncType,
                   "ir::NilTestInstr tested does not have pointer or func type"));
  }
  if (nil_test_instr->result()->type() != ir::bool_type()) {
    AddIssue(Issue(nil_test_instr, {nil_test_instr->result().get()},
                   Issue::Kind::kNilTestInstrResultDoesNotHaveBoolType,
                   "ir::NilTestInstr result does not have bool type"));
  }
}

void Checker::CheckMallocInstr(const ir::MallocInstr* malloc_instr) {
  if (malloc_instr->size()->type() != ir::i64()) {
    AddIssue(Issue(malloc_instr, {malloc_instr->size().get()},
                   Issue::Kind::kMallocInstrSizeDoesNotHaveI64Type,
                   "ir::MallocInstr size does not have I64 type"));
  }
  if (malloc_instr->result()->type() != ir::pointer_type()) {
    AddIssue(Issue(malloc_instr, {malloc_instr->result().get()},
                   Issue::Kind::kMallocInstrResultDoesNotHavePointerType,
                   "ir::MallocInstr result does not have pointer type"));
  }
}

void Checker::CheckLoadInstr(const ir::LoadInstr* load_instr) {
  if (load_instr->address()->type() != ir::pointer_type()) {
    AddIssue(Issue(load_instr, {load_instr->address().get()},
                   Issue::Kind::kLoadInstrAddressDoesNotHavePointerType,
                   "ir::LoadInstr address does not have pointer type"));
  }
}

void Checker::CheckStoreInstr(const ir::StoreInstr* store_instr) {
  if (store_instr->address()->type() != ir::pointer_type()) {
    AddIssue(Issue(store_instr, {store_instr->address().get()},
                   Issue::Kind::kStoreInstrAddressDoesNotHavePointerType,
                   "ir::StoreInstr address does not have pointer type"));
  }
}

void Checker::CheckFreeInstr(const ir::FreeInstr* free_instr) {
  if (free_instr->address()->type() != ir::pointer_type()) {
    AddIssue(Issue(free_instr, {free_instr->address().get()},
                   Issue::Kind::kFreeInstrAddressDoesNotHavePointerType,
                   "ir::FreeInstr address does not have pointer type"));
  }
}

void Checker::CheckJumpInstr(const ir::JumpInstr* jump_instr, const ir::Block* block) {
  if (block->children().size() != 1) {
    AddIssue(Issue(block, {jump_instr}, Issue::Kind::kControlFlowInstrMismatchedWithBlockGraph,
                   "ir::Block ends with ir::JumpInstr but does not have one child block"));
    return;
  }
  if (*block->children().begin() != jump_instr->destination()) {
    AddIssue(Issue(block, {jump_instr}, Issue::Kind::kJumpInstrDestinationIsNotChildBlock,
                   "ir::JumpInstr destination is not a child block"));
  }
}

void Checker::CheckJumpCondInstr(const ir::JumpCondInstr* jump_cond_instr, const ir::Block* block) {
  if (jump_cond_instr->condition()->type() != ir::bool_type()) {
    AddIssue(Issue(jump_cond_instr, {jump_cond_instr->condition().get()},
                   Issue::Kind::kJumpCondInstrConditionDoesNotHaveBoolType,
                   "ir::JumpCondInstr condition does not have bool type"));
  }
  if (block->children().size() != 2) {
    AddIssue(Issue(block, {jump_cond_instr}, Issue::Kind::kControlFlowInstrMismatchedWithBlockGraph,
                   "ir::Block ends with ir::JumpCondInstr but does not have two child blocks"));
    return;
  }
  if (jump_cond_instr->destination_true() == jump_cond_instr->destination_false()) {
    AddIssue(Issue(jump_cond_instr, Issue::Kind::kJumpCondInstrHasDuplicateDestinations,
                   "ir::JumpCondInstr has the same destination for true and false"));
    return;
  }
  bool child_matches_destination_true = false;
  bool child_matches_destination_false = false;
  for (ir::block_num_t child : block->children()) {
    if (child == jump_cond_instr->destination_true()) {
      child_matches_destination_true = true;
    } else if (child == jump_cond_instr->destination_false()) {
      child_matches_destination_false = true;
    }
  }
  if (!child_matches_destination_true) {
    AddIssue(Issue(block, {jump_cond_instr}, Issue::Kind::kJumpCondInstrDestinationIsNotChildBlock,
                   "ir::JumpCondInstr destination_true is not a child block"));
  }
  if (!child_matches_destination_false) {
    AddIssue(Issue(block, {jump_cond_instr}, Issue::Kind::kJumpCondInstrDestinationIsNotChildBlock,
                   "ir::JumpCondInstr destination_false is not a child block"));
  }
}

void Checker::CheckCallInstr(const ir::CallInstr* call_instr) {
  if (call_instr->func()->type() != ir::func_type()) {
    AddIssue(Issue(call_instr, {call_instr->func().get()},
                   Issue::Kind::kCallInstrCalleeDoesNotHaveFuncType,
                   "ir::CallInstr callee does not have func type"));
  }
  if (call_instr->func()->kind() != ir::Value::Kind::kConstant) {
    return;
  }
  ir::func_num_t callee_num = static_cast<ir::FuncConstant*>(call_instr->func().get())->value();
  if (!program_->HasFunc(callee_num)) {
    AddIssue(Issue(call_instr, {call_instr->func().get()},
                   Issue::Kind::kCallInstrStaticCalleeDoesNotExist,
                   "ir::CallInstr static callee func does not exist"));
    return;
  }

  ir::Func* callee = program_->GetFunc(callee_num);
  if (call_instr->args().size() != callee->args().size()) {
    AddIssue(Issue(call_instr, {callee}, Issue::Kind::kCallInstrDoesNotMatchStaticCalleeSignature,
                   "ir::CallInstr static callee has different number of arguments than provided"));
  } else {
    for (std::size_t i = 0; i < call_instr->args().size(); i++) {
      const ir::Type* actual_arg_type = call_instr->args().at(i)->type();
      const ir::Type* expected_arg_type = callee->args().at(i)->type();
      if (actual_arg_type != expected_arg_type) {
        AddIssue(Issue(call_instr,
                       {callee, call_instr->args().at(i).get(), callee->args().at(i).get()},
                       Issue::Kind::kCallInstrDoesNotMatchStaticCalleeSignature,
                       "ir::CallInstr and static callee argument type are mismatched"));
      }
    }
  }
  if (call_instr->results().size() != callee->result_types().size()) {
    AddIssue(Issue(call_instr, {callee}, Issue::Kind::kCallInstrDoesNotMatchStaticCalleeSignature,
                   "ir::CallInstr static callee has different number of results than provided"));
  } else {
    for (std::size_t i = 0; i < call_instr->results().size(); i++) {
      const ir::Type* actual_result_type = call_instr->results().at(i)->type();
      const ir::Type* expected_result_type = callee->result_types().at(i);
      if (actual_result_type != expected_result_type) {
        AddIssue(Issue(call_instr,
                       {callee, call_instr->results().at(i).get(), callee->result_types().at(i)},
                       Issue::Kind::kCallInstrDoesNotMatchStaticCalleeSignature,
                       "ir::CallInstr and static callee result type are mismatched"));
      }
    }
  }
}

void Checker::CheckReturnInstr(const ir::ReturnInstr* return_instr, const ir::Block* block,
                               const ir::Func* func) {
  if (!block->children().empty()) {
    AddIssue(Issue(block, {return_instr}, Issue::Kind::kControlFlowInstrMismatchedWithBlockGraph,
                   "ir::Block ends with ir::ReturnInstr but has child blocks"));
  }
  if (func->result_types().size() != return_instr->args().size()) {
    AddIssue(Issue(func, {return_instr}, Issue::Kind::kReturnInstrDoesNotMatchFuncSignature,
                   "ir::ReturnInstr and containing ir::Func have different numbers of results"));
    return;
  }
  for (std::size_t i = 0; i < return_instr->args().size(); i++) {
    const ir::Value* actual_return_value = return_instr->args().at(i).get();
    if (actual_return_value == nullptr) {
      return;
    }
    const ir::Type* actual_return_type = actual_return_value->type();
    const ir::Type* expected_return_type = func->result_types().at(i);
    if (actual_return_type != expected_return_type) {
      AddIssue(Issue(func,
                     {return_instr, return_instr->args().at(i).get(), func->result_types().at(i)},
                     Issue::Kind::kReturnInstrDoesNotMatchFuncSignature,
                     "ir::ReturnInstr arg and ir::Func result type are mismatched"));
    }
  }
}

void Checker::CheckValue(const ir::Value* value) {
  if (value->type() == nullptr) {
    AddIssue(Issue(value, Issue::Kind::kValueHasNullptrType, "ir::Value has nullptr type"));
  }
}

void Checker::AddIssue(Issue issue) { issues_.push_back(issue); }

std::ostream& operator<<(std::ostream& os, const Issue& issue) {
  return os << "[" << int64_t(issue.kind()) << "] " << issue.message();
}

std::ostream& operator<<(std::ostream& os, const Issue::Kind& kind) {
  return os << "[" << int64_t(kind) << "]";
}

}  // namespace ir_checker
