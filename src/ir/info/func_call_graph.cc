//
//  func_call_graph.cc
//  Katara
//
//  Created by Arne Philipeit on 1/9/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "func_call_graph.h"

#include "src/common/logging/logging.h"

namespace ir_info {

std::unordered_set<ir::func_num_t> FuncCallGraph::CalleesOfFunc(ir::func_num_t caller_num) const {
  std::unordered_set<ir::func_num_t> callees;
  for (const auto& func_call : func_calls_) {
    if (func_call->caller() == caller_num) {
    }
  }
  return callees;
}

std::unordered_set<ir::func_num_t> FuncCallGraph::CallersOfFunc(ir::func_num_t callee_num) const {
  std::unordered_set<ir::func_num_t> callers;
  for (const auto& func_call : func_calls_) {
    if (func_call->callees().contains(callee_num)) {
      callers.insert(func_call->caller());
    }
  }
  return callers;
}

std::unordered_set<FuncCall*> FuncCallGraph::FuncCallsWithCaller(ir::func_num_t caller_num) const {
  std::unordered_set<FuncCall*> func_calls_with_caller;
  for (const auto& func_call : func_calls_) {
    if (func_call->caller() == caller_num) {
      func_calls_with_caller.insert(func_call.get());
    }
  }
  return func_calls_with_caller;
}

std::unordered_set<FuncCall*> FuncCallGraph::FuncCallsWithCallee(ir::func_num_t callee_num) const {
  std::unordered_set<FuncCall*> func_calls_with_callee;
  for (const auto& func_call : func_calls_) {
    if (func_call->callees().contains(callee_num)) {
      func_calls_with_callee.insert(func_call.get());
    }
  }
  return func_calls_with_callee;
}

FuncCall* FuncCallGraph::FuncCallAtInstr(ir::CallInstr* call_instr) const {
  for (auto& func_call : func_calls_) {
    if (func_call->instr() == call_instr) {
      return func_call.get();
    }
  }
  return nullptr;
}

void FuncCallGraph::AddFunc(ir::func_num_t func) {
  funcs_.insert(func);
  component_cache_ = std::nullopt;
}

void FuncCallGraph::AddFuncCall(std::unique_ptr<FuncCall> func_call) {
  funcs_.insert(func_call->caller());
  funcs_.insert(func_call->callees().begin(), func_call->callees().end());
  func_calls_.push_back(std::move(func_call));
  component_cache_ = std::nullopt;
}

Component* FuncCallGraph::ComponentOfFunc(ir::func_num_t func_num) const {
  for (const auto& component : Components()) {
    if (component->members().contains(func_num)) {
      return component.get();
    }
  }
  return nullptr;
}

std::unordered_set<Component*> FuncCallGraph::ComponentsReachableFromComponent(
    Component* root_component) const {
  std::unordered_set<Component*> reachable_components{root_component};
  std::vector<Component*> frontier{root_component};
  while (!frontier.empty()) {
    Component* current = frontier.back();
    frontier.pop_back();
    for (Component* next : current->callees()) {
      if (reachable_components.contains(next)) {
        continue;
      }
      reachable_components.insert(next);
      frontier.push_back(next);
    }
  }
  return reachable_components;
}

std::unordered_set<ir::func_num_t> FuncCallGraph::FuncsReachableFromComponent(
    Component* root_component) const {
  std::unordered_set<ir::func_num_t> reachable_funcs;
  for (Component* component : ComponentsReachableFromComponent(root_component)) {
    reachable_funcs.insert(component->members().begin(), component->members().end());
  }
  return reachable_funcs;
}

std::vector<std::unique_ptr<Component>>& FuncCallGraph::Components() const {
  if (!component_cache_.has_value()) {
    GenerateComponents();
  }
  return component_cache_.value();
}

void FuncCallGraph::GenerateComponents() const {
  component_cache_ = std::vector<std::unique_ptr<Component>>();
  SCCAlgorithmState state{
      .index = 0,
  };
  for (ir::func_num_t func : funcs_) {
    GenerateComponent(func, state);
  }
  for (const auto& func_call : func_calls_) {
    Component* caller_component = ComponentOfFunc(func_call->caller());
    for (ir::func_num_t callee : func_call->callees()) {
      Component* callee_component = ComponentOfFunc(callee);
      if (caller_component == callee_component) {
        continue;
      }
      caller_component->callees_.insert(callee_component);
      callee_component->callers_.insert(caller_component);
    }
  }
}

void FuncCallGraph::GenerateComponent(ir::func_num_t caller, SCCAlgorithmState& state) const {
  state.func_annotations.insert({caller, SCCAlgorithmFuncAnnotations{
                                             .on_stack = true,
                                             .index = state.index,
                                             .low_link = state.index,
                                         }});
  state.index++;
  state.stack.push_back(caller);

  for (ir::func_num_t callee : CalleesOfFunc(caller)) {
    if (!state.func_annotations.contains(callee)) {
      GenerateComponent(callee, state);
      int64_t caller_low_link = state.func_annotations.at(caller).low_link;
      int64_t callee_low_link = state.func_annotations.at(callee).low_link;
      state.func_annotations.at(caller).low_link = std::min(caller_low_link, callee_low_link);

    } else if (state.func_annotations.at(callee).on_stack) {
      int64_t caller_low_link = state.func_annotations.at(caller).low_link;
      int64_t callee_index = state.func_annotations.at(callee).index;
      state.func_annotations.at(caller).low_link = std::min(caller_low_link, callee_index);
    }
  }

  int64_t caller_low_link = state.func_annotations.at(caller).low_link;
  int64_t caller_index = state.func_annotations.at(caller).index;
  if (caller_low_link == caller_index) {
    std::unordered_set<ir::func_num_t> members;
    while (true) {
      ir::func_num_t member = state.stack.back();
      state.stack.pop_back();
      state.func_annotations.at(member).on_stack = false;
      members.insert(member);
      if (member == caller) {
        break;
      }
    }
    component_cache_->push_back(
        std::unique_ptr<Component>(new Component(/* index= */ component_cache_->size(), members)));
  }
}

common::graph::Graph FuncCallGraph::ToGraph(ir::Program* program) const {
  common::graph::Graph graph(/*is_directed=*/true);

  for (ir::func_num_t func_num : funcs_) {
    std::string func_str = "@" + std::to_string(func_num);
    if (program != nullptr && program->HasFunc(func_num)) {
      ir::Func* func = program->GetFunc(func_num);
      func_str += "_" + func->name();
    }
    Component* component = ComponentOfFunc(func_num);
    graph.nodes().push_back(
        common::graph::NodeBuilder(func_num, func_str).SetSubgraph(component->index_).Build());
  }

  for (const auto& func_call : func_calls_) {
    ir::func_num_t caller = func_call->caller();
    for (ir::func_num_t callee : func_call->callees()) {
      graph.edges().push_back(common::graph::Edge(caller, callee));
    }
  }

  return graph;
}

}  // namespace ir_info
