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
#include "src/common/positions/positions.h"
#include "src/ir/serialization/positions.h"
#include "src/ir/serialization/positions_util.h"

namespace ir_check {

using ::common::positions::range_t;
using ::ir_serialization::BlockPositions;
using ::ir_serialization::FuncPositions;
using ::ir_serialization::InstrPositions;

void Checker::CheckProgram() {
  for (const std::unique_ptr<ir::Func>& func : program_->funcs()) {
    const FuncPositions& func_positions = program_positions_.GetFuncPositions(func.get());
    CheckFunc(func.get(), func_positions);
  }
}

void Checker::CheckFunc(const ir::Func* func,
                        const ir_serialization::FuncPositions& func_positions) {
  CheckValuesInFunc(func, func_positions);
  if (func->entry_block_num() == ir::kNoBlockNum) {
    issue_tracker().Add(ir_issues::IssueKind::kFuncHasNoEntryBlock, func_positions.header(),
                        "ir::Func has no set entry block");
  }
  for (const std::unique_ptr<ir::Block>& block : func->blocks()) {
    const BlockPositions& block_positions = program_positions_.GetBlockPositions(block.get());
    CheckBlock(block.get(), block_positions, func, func_positions);
  }
  for (std::size_t i = 0; i < func->result_types().size(); i++) {
    const ir::Type* type = func->result_types().at(i);
    if (type == nullptr) {
      issue_tracker().Add(ir_issues::IssueKind::kFuncHasNullptrResultType,
                          func_positions.result_ranges().at(i), "ir::Func has nullptr result type");
    }
  }
}

void Checker::AddDefinitionInFunc(const ir::Computed* value, range_t value_range,
                                  const ir::Func* func, FuncValues& func_values) {
  CheckValue(value, value_range);

  // Check and update computed association with func:
  if (auto it = values_to_funcs_.find(value); it != values_to_funcs_.end() && it->second != func) {
    issue_tracker().Add(ir_issues::IssueKind::kComputedValueUsedInMultipleFunctions,
                        {value_range, values_to_ranges_.at(value)},
                        "ir::Computed instance gets used in multiple functions");
  } else {
    values_to_funcs_.insert({value, func});
    values_to_ranges_.insert({value, value_range});
  }

  // Check and update value number association with ir::Computed instance:
  if (auto it = func_values.pointers.find(value->number());
      it != func_values.pointers.end() && it->second != value) {
    issue_tracker().Add(ir_issues::IssueKind::kComputedValueNumberUsedMultipleTimes,
                        {value_range, values_to_ranges_.at(it->second)},
                        "Multiple ir::Computed instances use the same value number");
  } else {
    func_values.pointers.insert({value->number(), value});
  }
}

void Checker::AddArgsInFunc(const ir::Func* func,
                            const ir_serialization::FuncPositions& func_positions,
                            FuncValues& func_values) {
  for (std::size_t i = 0; i < func->args().size(); i++) {
    const std::shared_ptr<ir::Computed>& arg = func->args().at(i);
    range_t arg_range = func_positions.arg_ranges().at(i);
    if (arg == nullptr) {
      issue_tracker().Add(ir_issues::IssueKind::kFuncDefinesNullptrArg,
                          func_positions.arg_ranges().at(i), "ir::Func defines nullptr arg");
      continue;
    }
    AddDefinitionInFunc(arg.get(), arg_range, func, func_values);
    if (auto it = func_values.args.find(arg->number()); it != func_values.args.end()) {
      issue_tracker().Add(ir_issues::IssueKind::kComputedValueHasMultipleDefinitions,
                          {it->second, arg_range}, "ir::Computed is a repeated function argument");
    } else {
      func_values.args.insert({arg->number(), arg_range});
    }
  }
}

void Checker::AddDefinitionsInFunc(const ir::Func* func, FuncValues& func_values) {
  for (const std::unique_ptr<ir::Block>& block : func->blocks()) {
    for (std::size_t instr_index = 0; instr_index < block->instrs().size(); instr_index++) {
      const ir::Instr* instr = block->instrs().at(instr_index).get();
      const InstrPositions& instr_positions = program_positions_.GetInstrPositions(instr);
      for (std::size_t i = 0; i < instr->DefinedValues().size(); i++) {
        ir::Computed* defined_value = instr->DefinedValues().at(i).get();
        range_t defined_value_range = instr_positions.defined_value_ranges().at(i);
        if (defined_value == nullptr) {
          issue_tracker().Add(ir_issues::IssueKind::kInstrDefinesNullptrValue, defined_value_range,
                              "ir::Instr defines nullptr value");
          continue;
        }
        AddDefinitionInFunc(defined_value, defined_value_range, func, func_values);
        if (auto arg_it = func_values.args.find(defined_value->number());
            arg_it != func_values.args.end()) {
          issue_tracker().Add(
              ir_issues::IssueKind::kComputedValueHasMultipleDefinitions,
              {arg_it->second, defined_value_range},
              "ir::Computed is a function argument and the result of a computation");
        } else if (auto it = func_values.definitions.find(defined_value->number());
                   it != func_values.definitions.end()) {
          issue_tracker().Add(ir_issues::IssueKind::kComputedValueHasMultipleDefinitions,
                              {it->second.range, defined_value_range},
                              "ir::Computed is the result of multiple computations");
        } else {
          func_values.definitions.insert({defined_value->number(), FuncValueReference{
                                                                       .block = block.get(),
                                                                       .instr = instr,
                                                                       .instr_index = instr_index,
                                                                       .range = defined_value_range,
                                                                   }});
        }
      }
    }
  }
}

void Checker::CheckDefinitionDominatesUse(const FuncValueReference& definition,
                                          const FuncValueReference& use, const ir::Func* func) {
  auto add_issue = [&] {
    issue_tracker().Add(ir_issues::IssueKind::kComputedValueDefinitionDoesNotDominateUse,
                        {definition.range, use.range},
                        "ir::Computed use is not dominated by definition");
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

void Checker::CheckValuesInFunc(const ir::Func* func,
                                const ir_serialization::FuncPositions& func_positions) {
  FuncValues func_values;
  AddArgsInFunc(func, func_positions, func_values);
  AddDefinitionsInFunc(func, func_values);

  for (const std::unique_ptr<ir::Block>& block : func->blocks()) {
    for (std::size_t instr_index = 0; instr_index < block->instrs().size(); instr_index++) {
      const ir::Instr* instr = block->instrs().at(instr_index).get();
      const InstrPositions& instr_positions = program_positions_.GetInstrPositions(instr);
      for (std::size_t used_value_index = 0; used_value_index < instr->UsedValues().size();
           used_value_index++) {
        const ir::Value* used_value = instr->UsedValues().at(used_value_index).get();
        if (used_value == nullptr) {
          issue_tracker().Add(ir_issues::IssueKind::kInstrUsesNullptrValue,
                              instr_positions.used_value_ranges().at(used_value_index),
                              "ir::Instr uses nullptr value");
          continue;
        }
        const ir::InheritedValue* inherited_value = nullptr;
        if (instr->instr_kind() == ir::InstrKind::kPhi) {
          inherited_value =
              static_cast<const ir::PhiInstr*>(instr)->args().at(used_value_index).get();
        }
        if (used_value->kind() != ir::Value::Kind::kComputed) {
          continue;
        }
        auto value = static_cast<const ir::Computed*>(used_value);
        if (!func_values.pointers.contains(value->number())) {
          issue_tracker().Add(ir_issues::IssueKind::kComputedValueHasNoDefinition,
                              instr_positions.used_value_ranges().at(used_value_index),
                              "ir::Instr uses value without definition");
        } else if (func_values.pointers.at(value->number()) != value) {
          issue_tracker().Add(ir_issues::IssueKind::kComputedValueNumberUsedMultipleTimes,
                              {instr_positions.used_value_ranges().at(used_value_index),
                               values_to_ranges_.at(func_values.pointers.at(value->number()))},
                              "Multiple ir::Computed instances use the same value number");
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

void Checker::CheckBlock(const ir::Block* block, const BlockPositions& block_positions,
                         const ir::Func* func, const FuncPositions& func_positions) {
  if (func->entry_block() == block && !block->parents().empty()) {
    issue_tracker().Add(ir_issues::IssueKind::kEntryBlockHasParents, block_positions.header(),
                        "ir::Func has entry block with parents");
  } else if (func->entry_block() != block && block->parents().empty()) {
    issue_tracker().Add(ir_issues::IssueKind::kNonEntryBlockHasNoParents, block_positions.header(),
                        "ir::Func has non-entry block without parents");
  }
  if (block->instrs().empty()) {
    issue_tracker().Add(ir_issues::IssueKind::kBlockContainsNoInstrs,
                        block_positions.entire_block(), "ir::Block does not contain instructions");
    return;
  }

  ir::Instr* first_regular_instr = nullptr;
  ir::Instr* last_instr = block->instrs().back().get();
  const InstrPositions& last_instr_positions = program_positions_.GetInstrPositions(last_instr);
  if (!last_instr->IsControlFlowInstr()) {
    issue_tracker().Add(ir_issues::IssueKind::kControlFlowInstrMissingAtEndOfBlock,
                        last_instr_positions.entire_instr(),
                        "ir::Block contains no control flow instruction at the end");
  }

  for (const std::unique_ptr<ir::Instr>& instr : block->instrs()) {
    const InstrPositions& instr_positions = program_positions_.GetInstrPositions(instr.get());
    if (instr->instr_kind() == ir::InstrKind::kPhi) {
      if (block->parents().size() < 2) {
        issue_tracker().Add(ir_issues::IssueKind::kPhiInBlockWithoutMultipleParents,
                            instr_positions.entire_instr(),
                            "ir::Block without multiple parents contains ir::PhiInstr");
      }
      if (first_regular_instr != nullptr) {
        const InstrPositions& first_regular_instr_positions =
            program_positions_.GetInstrPositions(first_regular_instr);
        issue_tracker().Add(
            ir_issues::IssueKind::kPhiAfterRegularInstrInBlock,
            {first_regular_instr_positions.entire_instr(), instr_positions.entire_instr()},
            "ir::Block contains ir::PhiInstr after other instruction");
      }
    } else if (first_regular_instr == nullptr) {
      first_regular_instr = instr.get();
    }
    if (instr->IsControlFlowInstr() && instr.get() != last_instr) {
      issue_tracker().Add(ir_issues::IssueKind::kControlFlowInstrBeforeEndOfBlock,
                          instr_positions.entire_instr(),
                          "ir::Block contains control flow instruction before the end");
    }

    CheckInstr(instr.get(), instr_positions, block, func, func_positions);
  }
}

void Checker::CheckInstr(const ir::Instr* instr,
                         const ir_serialization::InstrPositions& instr_positions,
                         const ir::Block* block, const ir::Func* func,
                         const FuncPositions& func_positions) {
  for (std::size_t i = 0; i < instr->UsedValues().size(); i++) {
    ir::Value* used_value = instr->UsedValues().at(i).get();
    if (used_value == nullptr) {
      return;
    }
    if (instr->instr_kind() != ir::InstrKind::kPhi &&
        used_value->kind() == ir::Value::Kind::kInherited) {
      issue_tracker().Add(ir_issues::IssueKind::kNonPhiInstrUsesInheritedValue,
                          instr_positions.used_value_ranges().at(i),
                          "non-phi ir::Inst uses inherited value");
    }
  }

  switch (instr->instr_kind()) {
    case ir::InstrKind::kMov:
      CheckMovInstr(static_cast<const ir::MovInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kPhi:
      CheckPhiInstr(static_cast<const ir::PhiInstr*>(instr), instr_positions, block, func);
      break;
    case ir::InstrKind::kConversion:
      CheckConversion(static_cast<const ir::Conversion*>(instr), instr_positions);
      break;
    case ir::InstrKind::kBoolNot:
      CheckBoolNotInstr(static_cast<const ir::BoolNotInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kBoolBinary:
      CheckBoolBinaryInstr(static_cast<const ir::BoolBinaryInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kIntUnary:
      CheckIntUnaryInstr(static_cast<const ir::IntUnaryInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kIntCompare:
      CheckIntCompareInstr(static_cast<const ir::IntCompareInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kIntBinary:
      CheckIntBinaryInstr(static_cast<const ir::IntBinaryInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kIntShift:
      CheckIntShiftInstr(static_cast<const ir::IntShiftInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kPointerOffset:
      CheckPointerOffsetInstr(static_cast<const ir::PointerOffsetInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kNilTest:
      CheckNilTestInstr(static_cast<const ir::NilTestInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kMalloc:
      CheckMallocInstr(static_cast<const ir::MallocInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kLoad:
      CheckLoadInstr(static_cast<const ir::LoadInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kStore:
      CheckStoreInstr(static_cast<const ir::StoreInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kFree:
      CheckFreeInstr(static_cast<const ir::FreeInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kJump:
      CheckJumpInstr(static_cast<const ir::JumpInstr*>(instr), instr_positions, block);
      break;
    case ir::InstrKind::kJumpCond:
      CheckJumpCondInstr(static_cast<const ir::JumpCondInstr*>(instr), instr_positions, block);
      break;
    case ir::InstrKind::kSyscall:
      CheckSyscallInstr(static_cast<const ir::SyscallInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kCall:
      CheckCallInstr(static_cast<const ir::CallInstr*>(instr), instr_positions);
      break;
    case ir::InstrKind::kReturn:
      CheckReturnInstr(static_cast<const ir::ReturnInstr*>(instr), instr_positions, block, func,
                       func_positions);
      break;
    default:
      issue_tracker().Add(ir_issues::IssueKind::kUnknownInstrKind, instr_positions.name(),
                          "ir::InstrKind is unknown");
  }
}

void Checker::CheckMovInstr(const ir::MovInstr* mov_instr,
                            const ir_serialization::InstrPositions& mov_instr_positions) {
  if (!ir::IsEqual(mov_instr->origin()->type(), mov_instr->result()->type())) {
    issue_tracker().Add(ir_issues::IssueKind::kMovInstrOriginAndResultHaveMismatchedTypes,
                        {mov_instr_positions.defined_value_ranges().front(),
                         mov_instr_positions.used_value_ranges().front()},
                        "ir::MovInstr has with mismatched origin and result type");
  }
}

void Checker::CheckPhiInstr(const ir::PhiInstr* phi_instr,
                            const ir_serialization::InstrPositions& phi_instr_positions,
                            const ir::Block* block, const ir::Func* func) {
  std::unordered_map<ir::block_num_t, int64_t> parent_arg_indices;
  parent_arg_indices.reserve(block->parents().size());
  for (ir::block_num_t parent : block->parents()) {
    parent_arg_indices.insert({parent, -1});
  }

  for (std::size_t i = 0; i < phi_instr->args().size(); i++) {
    const ir::InheritedValue* arg = phi_instr->args().at(i).get();

    if (arg->origin() == ir::kNoBlockNum || !block->parents().contains(arg->origin())) {
      issue_tracker().Add(ir_issues::IssueKind::kPhiInstrHasArgumentForNonParentBlock,
                          phi_instr_positions.used_value_ranges().at(i),
                          "ir::PhiInstr has arg for non-parent block");
    } else if (parent_arg_indices.at(arg->origin()) != -1) {
      issue_tracker().Add(ir_issues::IssueKind::kPhiInstrHasMultipleArgumentsForParentBlock,
                          phi_instr_positions.used_value_ranges(),
                          "ir::PhiInstr has multiple args for the same parent block");
    } else {
      parent_arg_indices.at(arg->origin()) = int64_t(i);
    }

    if (!ir::IsEqual(arg->type(), phi_instr->result()->type())) {
      issue_tracker().Add(ir_issues::IssueKind::kPhiInstrArgAndResultHaveMismatchedTypes,
                          {phi_instr_positions.defined_value_ranges().front(),
                           phi_instr_positions.used_value_ranges().at(i)},
                          "ir::PhiInstr has mismatched arg and result type");
    }
  }

  for (ir::block_num_t parent_num : block->parents()) {
    ir::Block* parent_block = func->GetBlock(parent_num);
    const BlockPositions parent_block_positions =
        program_positions_.GetBlockPositions(parent_block);
    if (parent_arg_indices.at(parent_num) == -1) {
      issue_tracker().Add(
          ir_issues::IssueKind::kPhiInstrHasNoArgumentForParentBlock,
          {phi_instr_positions.used_values_range(), parent_block_positions.entire_block()},
          "ir::PhiInstr has no argument for parent block");
    }
  }
}

void Checker::CheckConversion(const ir::Conversion* conversion,
                              const ir_serialization::InstrPositions& conversion_positions) {
  switch (conversion->operand()->type()->type_kind()) {
    case ir::TypeKind::kBool:
    case ir::TypeKind::kInt:
    case ir::TypeKind::kPointer:
    case ir::TypeKind::kFunc:
      break;
    default:
      issue_tracker().Add(ir_issues::IssueKind::kConversionOperandHasUnsupportedType,
                          conversion_positions.used_value_ranges().front(),
                          "ir::Conversion has operand with unsupported type");
  }
  switch (conversion->result()->type()->type_kind()) {
    case ir::TypeKind::kBool:
    case ir::TypeKind::kInt:
    case ir::TypeKind::kPointer:
    case ir::TypeKind::kFunc:
      break;
    default:
      issue_tracker().Add(ir_issues::IssueKind::kConversionResultHasUnsupportedType,
                          conversion_positions.defined_value_ranges().front(),
                          "ir::Conversion has result with unsupported type");
  }
}

void Checker::CheckBoolNotInstr(const ir::BoolNotInstr* bool_not_instr,
                                const ir_serialization::InstrPositions& bool_not_instr_positions) {
  if (bool_not_instr->operand()->type() != ir::bool_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kBoolNotInstrOperandDoesNotHaveBoolType,
                        bool_not_instr_positions.used_value_ranges().front(),
                        "ir::BoolNotInstr operand does not have bool type");
  }
  if (bool_not_instr->result()->type() != ir::bool_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kBoolNotInstrResultDoesNotHaveBoolType,
                        bool_not_instr_positions.defined_value_ranges().front(),
                        "ir::BoolNotInstr result does not have bool type");
  }
}

void Checker::CheckBoolBinaryInstr(
    const ir::BoolBinaryInstr* bool_binary_instr,
    const ir_serialization::InstrPositions& bool_binary_instr_positions) {
  auto check_operand = [this](ir::Value* operand, range_t operand_range) {
    if (operand->type() != ir::bool_type()) {
      issue_tracker().Add(ir_issues::IssueKind::kBoolBinaryInstrOperandDoesNotHaveBoolType,
                          operand_range, "ir::BoolBinaryInstr operand does not have bool type");
    }
  };
  check_operand(bool_binary_instr->operand_a().get(),
                bool_binary_instr_positions.used_value_ranges().at(0));
  check_operand(bool_binary_instr->operand_b().get(),
                bool_binary_instr_positions.used_value_ranges().at(1));
  if (bool_binary_instr->result()->type() != ir::bool_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kBoolBinaryInstrResultDoesNotHaveBoolType,
                        bool_binary_instr_positions.defined_value_ranges().front(),
                        "ir::BoolBinaryInstr result does not have bool type");
  }
}

void Checker::CheckIntUnaryInstr(
    const ir::IntUnaryInstr* int_unary_instr,
    const ir_serialization::InstrPositions& int_unary_instr_positions) {
  if (int_unary_instr->operand()->type()->type_kind() != ir::TypeKind::kInt) {
    issue_tracker().Add(ir_issues::IssueKind::kIntUnaryInstrOperandDoesNotHaveIntType,
                        int_unary_instr_positions.used_value_ranges().front(),
                        "ir::IntUnaryInstr operand does not have int type");
  }
  if (int_unary_instr->result()->type()->type_kind() != ir::TypeKind::kInt) {
    issue_tracker().Add(ir_issues::IssueKind::kIntUnaryInstrResultDoesNotHaveIntType,
                        int_unary_instr_positions.defined_value_ranges().front(),
                        "ir::IntUnaryInstr result does not have int type");
  }
  if (int_unary_instr->result()->type() != int_unary_instr->operand()->type()) {
    issue_tracker().Add(ir_issues::IssueKind::kIntUnaryInstrResultAndOperandHaveDifferentTypes,
                        {int_unary_instr_positions.defined_value_ranges().front(),
                         int_unary_instr_positions.used_value_ranges().front()},
                        "ir::IntUnaryInstr result and operand have different types");
  }
}

void Checker::CheckIntCompareInstr(
    const ir::IntCompareInstr* int_compare_instr,
    const ir_serialization::InstrPositions& int_compare_instr_positions) {
  auto check_operand = [this](ir::Value* operand, range_t operand_range) {
    if (operand->type()->type_kind() != ir::TypeKind::kInt) {
      issue_tracker().Add(ir_issues::IssueKind::kIntCompareInstrOperandDoesNotHaveIntType,
                          operand_range, "ir::IntCompareInstr operand does not have int type");
    }
  };
  check_operand(int_compare_instr->operand_a().get(),
                int_compare_instr_positions.used_value_ranges().at(0));
  check_operand(int_compare_instr->operand_b().get(),
                int_compare_instr_positions.used_value_ranges().at(1));
  if (int_compare_instr->operand_a()->type() != int_compare_instr->operand_b()->type()) {
    issue_tracker().Add(ir_issues::IssueKind::kIntCompareInstrOperandsHaveDifferentTypes,
                        {int_compare_instr_positions.used_value_ranges().at(0),
                         int_compare_instr_positions.used_value_ranges().at(1)},
                        "ir::IntCompareInstr operands have different types");
  }
  if (int_compare_instr->result()->type() != ir::bool_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kIntCompareInstrResultDoesNotHaveBoolType,
                        int_compare_instr_positions.defined_value_ranges().front(),
                        "ir::IntCompareInstr result does not have bool type");
  }
}

void Checker::CheckIntBinaryInstr(
    const ir::IntBinaryInstr* int_binary_instr,
    const ir_serialization::InstrPositions& int_binary_instr_positions) {
  auto check_operand = [this](ir::Value* operand, range_t operand_range) {
    if (operand->type()->type_kind() != ir::TypeKind::kInt) {
      issue_tracker().Add(ir_issues::IssueKind::kIntBinaryInstrOperandDoesNotHaveIntType,
                          operand_range, "ir::IntBinaryInstr operand does not have int type");
    }
  };
  check_operand(int_binary_instr->operand_a().get(),
                int_binary_instr_positions.used_value_ranges().at(0));
  check_operand(int_binary_instr->operand_b().get(),
                int_binary_instr_positions.used_value_ranges().at(1));
  if (int_binary_instr->result()->type()->type_kind() != ir::TypeKind::kInt) {
    issue_tracker().Add(ir_issues::IssueKind::kIntBinaryInstrResultDoesNotHaveIntType,
                        int_binary_instr_positions.defined_value_ranges().front(),
                        "ir::IntBinaryInstr result does not have int type");
  }
  if (int_binary_instr->result()->type() != int_binary_instr->operand_a()->type() ||
      int_binary_instr->result()->type() != int_binary_instr->operand_b()->type()) {
    issue_tracker().Add(ir_issues::IssueKind::kIntBinaryInstrOperandsAndResultHaveDifferentTypes,
                        {int_binary_instr_positions.defined_value_ranges().front(),
                         int_binary_instr_positions.used_values_range()},
                        "ir::IntBinaryInstr operands and result have different types");
  }
}

void Checker::CheckIntShiftInstr(
    const ir::IntShiftInstr* int_shift_instr,
    const ir_serialization::InstrPositions& int_shift_instr_positions) {
  auto check_operand = [this](ir::Value* operand, range_t operand_range) {
    if (operand->type()->type_kind() != ir::TypeKind::kInt) {
      issue_tracker().Add(ir_issues::IssueKind::kIntShiftInstrOperandDoesNotHaveIntType,
                          operand_range, "ir::IntShiftInstr operand does not have int type");
    }
  };
  check_operand(int_shift_instr->shifted().get(),
                int_shift_instr_positions.used_value_ranges().at(0));
  check_operand(int_shift_instr->offset().get(),
                int_shift_instr_positions.used_value_ranges().at(1));
  if (int_shift_instr->result()->type()->type_kind() != ir::TypeKind::kInt) {
    issue_tracker().Add(ir_issues::IssueKind::kIntShiftInstrResultDoesNotHaveIntType,
                        int_shift_instr_positions.defined_value_ranges().front(),
                        "ir::IntShiftInstr result does not have int type");
  }
  if (int_shift_instr->result()->type() != int_shift_instr->shifted()->type()) {
    issue_tracker().Add(ir_issues::IssueKind::kIntShiftInstrShiftedAndResultHaveDifferentTypes,
                        {int_shift_instr_positions.defined_value_ranges().front(),
                         int_shift_instr_positions.used_value_ranges().at(0)},
                        "ir::IntShiftInstr shifted and result have different types");
  }
}

void Checker::CheckPointerOffsetInstr(
    const ir::PointerOffsetInstr* pointer_offset_instr,
    const ir_serialization::InstrPositions& pointer_offset_instr_positions) {
  if (pointer_offset_instr->pointer()->type() != ir::pointer_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kPointerOffsetInstrPointerDoesNotHavePointerType,
                        pointer_offset_instr_positions.used_value_ranges().at(0),
                        "ir::PointerOffsetInstr pointer does not have pointer type");
  }
  if (pointer_offset_instr->offset()->type() != ir::i64()) {
    issue_tracker().Add(ir_issues::IssueKind::kPointerOffsetInstrOffsetDoesNotHaveI64Type,
                        pointer_offset_instr_positions.used_value_ranges().at(1),
                        "ir::PointerOffsetInstr offset does not have I64 type");
  }
  if (pointer_offset_instr->result()->type() != ir::pointer_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kPointerOffsetInstrResultDoesNotHavePointerType,
                        pointer_offset_instr_positions.defined_value_ranges().front(),
                        "ir::PointerOffsetInstr result does not have pointer type");
  }
}

void Checker::CheckNilTestInstr(const ir::NilTestInstr* nil_test_instr,
                                const ir_serialization::InstrPositions& nil_test_instr_positions) {
  if (nil_test_instr->tested()->type() != ir::pointer_type() &&
      nil_test_instr->tested()->type() != ir::func_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kNilTestInstrTestedDoesNotHavePointerOrFuncType,
                        nil_test_instr_positions.used_value_ranges().front(),
                        "ir::NilTestInstr tested does not have pointer or func type");
  }
  if (nil_test_instr->result()->type() != ir::bool_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kNilTestInstrResultDoesNotHaveBoolType,
                        nil_test_instr_positions.defined_value_ranges().front(),
                        "ir::NilTestInstr result does not have bool type");
  }
}

void Checker::CheckMallocInstr(const ir::MallocInstr* malloc_instr,
                               const ir_serialization::InstrPositions& malloc_instr_positions) {
  if (malloc_instr->size()->type() != ir::i64()) {
    issue_tracker().Add(ir_issues::IssueKind::kMallocInstrSizeDoesNotHaveI64Type,
                        malloc_instr_positions.used_value_ranges().front(),
                        "ir::MallocInstr size does not have I64 type");
  }
  if (malloc_instr->result()->type() != ir::pointer_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kMallocInstrResultDoesNotHavePointerType,
                        malloc_instr_positions.defined_value_ranges().front(),
                        "ir::MallocInstr result does not have pointer type");
  }
}

void Checker::CheckLoadInstr(const ir::LoadInstr* load_instr,
                             const ir_serialization::InstrPositions& load_instr_positions) {
  if (load_instr->address()->type() != ir::pointer_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kLoadInstrAddressDoesNotHavePointerType,
                        load_instr_positions.used_value_ranges().front(),
                        "ir::LoadInstr address does not have pointer type");
  }
}

void Checker::CheckStoreInstr(const ir::StoreInstr* store_instr,
                              const ir_serialization::InstrPositions& store_instr_positions) {
  if (store_instr->address()->type() != ir::pointer_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kStoreInstrAddressDoesNotHavePointerType,
                        store_instr_positions.used_value_ranges().front(),
                        "ir::StoreInstr address does not have pointer type");
  }
}

void Checker::CheckFreeInstr(const ir::FreeInstr* free_instr,
                             const ir_serialization::InstrPositions& free_instr_positions) {
  if (free_instr->address()->type() != ir::pointer_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kFreeInstrAddressDoesNotHavePointerType,
                        free_instr_positions.used_value_ranges().front(),
                        "ir::FreeInstr address does not have pointer type");
  }
}

void Checker::CheckJumpInstr(const ir::JumpInstr* jump_instr,
                             const ir_serialization::InstrPositions& jump_instr_positions,
                             const ir::Block* block) {
  if (block->children().size() != 1) {
    issue_tracker().Add(ir_issues::IssueKind::kControlFlowInstrMismatchedWithBlockGraph,
                        jump_instr_positions.entire_instr(),
                        "ir::Block ends with ir::JumpInstr but does not have one child block");
    return;
  }
  if (*block->children().begin() != jump_instr->destination()) {
    issue_tracker().Add(ir_issues::IssueKind::kJumpInstrDestinationIsNotChildBlock,
                        ir_serialization::GetJumpInstrDestinationRange(jump_instr_positions),
                        "ir::JumpInstr destination is not a child block");
  }
}

void Checker::CheckJumpCondInstr(const ir::JumpCondInstr* jump_cond_instr,
                                 const ir_serialization::InstrPositions& jump_cond_instr_positions,
                                 const ir::Block* block) {
  if (jump_cond_instr->condition()->type() != ir::bool_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kJumpCondInstrConditionDoesNotHaveBoolType,
                        jump_cond_instr_positions.used_value_ranges().front(),
                        "ir::JumpCondInstr condition does not have bool type");
  }
  if (block->children().size() != 2) {
    issue_tracker().Add(ir_issues::IssueKind::kControlFlowInstrMismatchedWithBlockGraph,
                        jump_cond_instr_positions.entire_instr(),
                        "ir::Block ends with ir::JumpCondInstr but does not have two child blocks");
    return;
  }
  if (jump_cond_instr->destination_true() == jump_cond_instr->destination_false()) {
    issue_tracker().Add(
        ir_issues::IssueKind::kJumpCondInstrHasDuplicateDestinations,
        {ir_serialization::GetJumpCondInstrDestinationTrueRange(jump_cond_instr_positions),
         ir_serialization::GetJumpCondInstrDestinationFalseRange(jump_cond_instr_positions)},
        "ir::JumpCondInstr has the same destination for true and false");
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
    issue_tracker().Add(ir_issues::IssueKind::kJumpCondInstrDestinationIsNotChildBlock,
                        jump_cond_instr_positions.used_value_ranges().at(1),
                        "ir::JumpCondInstr destination_true is not a child block");
  }
  if (!child_matches_destination_false) {
    issue_tracker().Add(ir_issues::IssueKind::kJumpCondInstrDestinationIsNotChildBlock,
                        jump_cond_instr_positions.used_value_ranges().at(2),
                        "ir::JumpCondInstr destination_false is not a child block");
  }
}

void Checker::CheckSyscallInstr(const ir::SyscallInstr* syscall_instr,
                                const ir_serialization::InstrPositions& syscall_instr_positions) {
  if (syscall_instr->result()->type() != ir::i64()) {
    issue_tracker().Add(ir_issues::IssueKind::kSyscallInstrResultDoesNotHaveI64Type,
                        syscall_instr_positions.defined_value_ranges().front(),
                        "ir::SyscallInstr result does not have I64 type");
  }
  if (syscall_instr->syscall_num()->type() != ir::i64()) {
    issue_tracker().Add(ir_issues::IssueKind::kSyscallInstrSyscallNumberDoesNotHaveI64Type,
                        syscall_instr_positions.used_value_ranges().front(),
                        "ir::SyscallInstr syscall number does not have I64 type");
  }
  for (std::size_t i = 0; i < syscall_instr->args().size(); i++) {
    ir::Value* arg = syscall_instr->args().at(i).get();
    if (arg->type() != ir::i64()) {
      issue_tracker().Add(ir_issues::IssueKind::kSyscallInstrArgDoesNotHaveI64Type,
                          syscall_instr_positions.used_value_ranges().at(i + 1),
                          "ir::SyscallInstr arg does not have I64 type");
    }
  }
}

void Checker::CheckCallInstr(const ir::CallInstr* call_instr,
                             const ir_serialization::InstrPositions& call_instr_positions) {
  if (call_instr->func()->type() != ir::func_type()) {
    issue_tracker().Add(ir_issues::IssueKind::kCallInstrCalleeDoesNotHaveFuncType,
                        call_instr_positions.used_value_ranges().front(),
                        "ir::CallInstr callee does not have func type");
  }
  if (call_instr->func()->kind() != ir::Value::Kind::kConstant) {
    return;
  }
  ir::func_num_t callee_num = static_cast<ir::FuncConstant*>(call_instr->func().get())->value();
  if (!program_->HasFunc(callee_num)) {
    issue_tracker().Add(ir_issues::IssueKind::kCallInstrStaticCalleeDoesNotExist,
                        call_instr_positions.used_value_ranges().front(),
                        "ir::CallInstr static callee func does not exist");
    return;
  }

  ir::Func* callee = program_->GetFunc(callee_num);
  const FuncPositions& callee_positions = program_positions_.GetFuncPositions(callee);
  if (call_instr->args().size() != callee->args().size()) {
    issue_tracker().Add(
        ir_issues::IssueKind::kCallInstrDoesNotMatchStaticCalleeSignature,
        {call_instr_positions.used_values_range(), callee_positions.args_range()},
        "ir::CallInstr static callee has different number of arguments than provided");
  } else {
    for (std::size_t i = 0; i < call_instr->args().size(); i++) {
      const ir::Type* actual_arg_type = call_instr->args().at(i)->type();
      const ir::Type* expected_arg_type = callee->args().at(i)->type();
      if (!ir::IsEqual(actual_arg_type, expected_arg_type)) {
        issue_tracker().Add(ir_issues::IssueKind::kCallInstrDoesNotMatchStaticCalleeSignature,
                            {call_instr_positions.used_value_ranges().at(i + 1),
                             callee_positions.arg_ranges().at(i)},
                            "ir::CallInstr and static callee argument type are mismatched");
      }
    }
  }
  if (call_instr->results().size() != callee->result_types().size()) {
    issue_tracker().Add(
        ir_issues::IssueKind::kCallInstrDoesNotMatchStaticCalleeSignature,
        {call_instr_positions.defined_values_range(), callee_positions.results_range()},
        "ir::CallInstr static callee has different number of results than provided");
  } else {
    for (std::size_t i = 0; i < call_instr->results().size(); i++) {
      const ir::Type* actual_result_type = call_instr->results().at(i)->type();
      const ir::Type* expected_result_type = callee->result_types().at(i);
      if (!ir::IsEqual(actual_result_type, expected_result_type)) {
        issue_tracker().Add(ir_issues::IssueKind::kCallInstrDoesNotMatchStaticCalleeSignature,
                            {call_instr_positions.defined_value_ranges().at(i),
                             callee_positions.result_ranges().at(i)},
                            "ir::CallInstr and static callee result type are mismatched");
      }
    }
  }
}

void Checker::CheckReturnInstr(const ir::ReturnInstr* return_instr,
                               const ir_serialization::InstrPositions& return_instr_positions,
                               const ir::Block* block, const ir::Func* func,
                               const FuncPositions& func_positions) {
  if (!block->children().empty()) {
    issue_tracker().Add(ir_issues::IssueKind::kControlFlowInstrMismatchedWithBlockGraph,
                        return_instr_positions.entire_instr(),
                        "ir::Block ends with ir::ReturnInstr but has child blocks");
  }
  if (func->result_types().size() != return_instr->args().size()) {
    issue_tracker().Add(
        ir_issues::IssueKind::kReturnInstrDoesNotMatchFuncSignature,
        {return_instr_positions.used_values_range(), func_positions.results_range()},
        "ir::ReturnInstr and containing ir::Func have different numbers of results");
    return;
  }
  for (std::size_t i = 0; i < return_instr->args().size(); i++) {
    const ir::Value* actual_return_value = return_instr->args().at(i).get();
    if (actual_return_value == nullptr) {
      return;
    }
    const ir::Type* actual_return_type = actual_return_value->type();
    const ir::Type* expected_return_type = func->result_types().at(i);
    if (!ir::IsEqual(actual_return_type, expected_return_type)) {
      issue_tracker().Add(
          ir_issues::IssueKind::kReturnInstrDoesNotMatchFuncSignature,
          {return_instr_positions.used_value_ranges().at(i), func_positions.result_ranges().at(i)},
          "ir::ReturnInstr arg and ir::Func result type are mismatched");
    }
  }
}

void Checker::CheckValue(const ir::Computed* value, range_t value_range) {
  if (value->type() == nullptr) {
    issue_tracker().Add(ir_issues::IssueKind::kValueHasNullptrType, value_range,
                        "ir::Value has nullptr type");
  }
}

}  // namespace ir_check
