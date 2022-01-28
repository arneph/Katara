//
//  func_call_graph_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 1/9/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "func_call_graph_builder.h"

#include "src/common/logging/logging.h"

namespace ir_analyzers {
namespace {

void AddDynamicCalleesFromValues(const auto& values,
                                 std::unordered_set<ir::func_num_t>& dynamic_callees) {
  for (const auto& value : values) {
    if (value->kind() != ir::Value::Kind::kConstant || value->type() != ir::func_type()) {
      continue;
    }
    ir::FuncConstant* func_constant = static_cast<ir::FuncConstant*>(value.get());
    dynamic_callees.insert(func_constant->value());
  }
}

std::unordered_set<ir::func_num_t> FindDynamicCallees(const ir::Program* program) {
  // TODO: improve finding callees, e.g. group by func signatures, implement generic traversal
  // helpers
  std::unordered_set<ir::func_num_t> dynamic_callees;
  for (const auto& func : program->funcs()) {
    for (const auto& block : func->blocks()) {
      for (const auto& instr : block->instrs()) {
        if (instr->instr_kind() == ir::InstrKind::kCall) {
          ir::CallInstr* call_instr = static_cast<ir::CallInstr*>(instr.get());
          AddDynamicCalleesFromValues(call_instr->args(), dynamic_callees);
        } else {
          AddDynamicCalleesFromValues(instr->UsedValues(), dynamic_callees);
        }
      }
    }
  }
  return dynamic_callees;
}

}  // namespace

const ir_info::FuncCallGraph BuildFuncCallGraphForProgram(const ir::Program* program) {
  ir_info::FuncCallGraph fcg;
  for (const auto& func : program->funcs()) {
    fcg.AddFunc(func->number());
  }

  std::unordered_set<ir::func_num_t> dynamic_callees = FindDynamicCallees(program);
  for (const auto& caller_func : program->funcs()) {
    ir::func_num_t caller_func_num = caller_func->number();
    for (const auto& block : caller_func->blocks()) {
      for (const auto& instr : block->instrs()) {
        if (instr->instr_kind() != ir::InstrKind::kCall) {
          continue;
        }
        ir::CallInstr* call_instr = static_cast<ir::CallInstr*>(instr.get());
        ir::Value* callee_value = call_instr->func().get();
        if (callee_value->kind() == ir::Value::Kind::kConstant) {
          ir::func_num_t callee_func_num = static_cast<ir::FuncConstant*>(callee_value)->value();
          fcg.AddFuncCall(
              std::make_unique<ir_info::FuncCall>(call_instr, caller_func_num, callee_func_num));
        } else if (callee_value->kind() == ir::Value::Kind::kComputed) {
          fcg.AddFuncCall(
              std::make_unique<ir_info::FuncCall>(call_instr, caller_func_num, dynamic_callees));
        } else {
          common::fail("unexpected ir value kind");
        }
      }
    }
  }
  return fcg;
}

}  // namespace ir_analyzers
