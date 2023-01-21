//
//  func_call_graph.h
//  Katara
//
//  Created by Arne Philipeit on 1/9/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_info_func_call_graph_h
#define ir_info_func_call_graph_h

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "src/common/graph/graph.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/values.h"

namespace ir_info {

class FuncCall {
 public:
  FuncCall(ir::CallInstr* instr, ir::func_num_t caller, ir::func_num_t callee)
      : FuncCall(instr, caller, std::unordered_set<ir::func_num_t>{callee}) {}
  FuncCall(ir::CallInstr* instr, ir::func_num_t caller, std::unordered_set<ir::func_num_t> callees)
      : instr_(instr), caller_(caller), callees_(callees) {}

  ir::CallInstr* instr() const { return instr_; }
  ir::func_num_t caller() const { return caller_; }
  const std::unordered_set<ir::func_num_t>& callees() const { return callees_; }

 private:
  ir::CallInstr* instr_;
  ir::func_num_t caller_;
  std::unordered_set<ir::func_num_t> callees_;
};

class Component {
 public:
  const std::unordered_set<ir::func_num_t>& members() const { return members_; }

  const std::unordered_set<Component*>& callers() const { return callers_; }
  const std::unordered_set<Component*>& callees() const { return callees_; }

 private:
  Component(int64_t index, std::unordered_set<ir::func_num_t> members)
      : index_(index), members_(members) {}

  int64_t index_;
  std::unordered_set<ir::func_num_t> members_;
  std::unordered_set<Component*> callers_;
  std::unordered_set<Component*> callees_;

  friend class FuncCallGraph;
};

class FuncCallGraph {
 public:
  const std::unordered_set<ir::func_num_t>& funcs() const { return funcs_; }

  std::unordered_set<ir::func_num_t> CalleesOfFunc(ir::func_num_t caller_num) const;
  std::unordered_set<ir::func_num_t> CallersOfFunc(ir::func_num_t callee_num) const;

  std::unordered_set<FuncCall*> FuncCallsWithCaller(ir::func_num_t caller_num) const;
  std::unordered_set<FuncCall*> FuncCallsWithCallee(ir::func_num_t callee_num) const;
  FuncCall* FuncCallAtInstr(ir::CallInstr* call_instr) const;

  void AddFunc(ir::func_num_t func);
  void AddFuncCall(std::unique_ptr<FuncCall> func_call);

  Component* ComponentOfFunc(ir::func_num_t func_num) const;
  std::unordered_set<Component*> ComponentsReachableFromComponent(Component* root_component) const;
  std::unordered_set<ir::func_num_t> FuncsReachableFromComponent(Component* root_component) const;

  common::graph::Graph ToGraph(ir::Program* program = nullptr) const;

 private:
  // State required for Tarjan's Strongly Connected Components Algorithm
  struct SCCAlgorithmFuncAnnotations {
    bool on_stack;
    int64_t index;
    int64_t low_link;
  };
  struct SCCAlgorithmState {
    int64_t index;
    std::vector<ir::func_num_t> stack;
    std::unordered_map<ir::func_num_t, SCCAlgorithmFuncAnnotations> func_annotations;
  };

  std::vector<std::unique_ptr<Component>>& Components() const;
  void GenerateComponents() const;
  void GenerateComponent(ir::func_num_t func, SCCAlgorithmState& state) const;

  std::unordered_set<ir::func_num_t> funcs_;
  std::vector<std::unique_ptr<FuncCall>> func_calls_;
  mutable std::optional<std::vector<std::unique_ptr<Component>>> component_cache_;
};

}  // namespace ir_info

#endif /* func_call_graph_hpp */
