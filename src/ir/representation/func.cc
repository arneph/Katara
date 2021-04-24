//
//  func.cc
//  Katara
//
//  Created by Arne Philipeit on 12/23/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "func.h"

#include <sstream>

#include "vcg/edge.h"
#include "vcg/node.h"

namespace ir {

Func::Func(int64_t number, Program* prog) : number_(number), prog_(prog) {}

Func::~Func() {
  for (Block* block : blocks_) {
    delete block;
  }
}

int64_t Func::number() const { return number_; }

std::string Func::name() const { return name_; }

void Func::set_name(std::string name) { name_ = name; }

std::string Func::ReferenceString() const {
  std::string title = "@" + std::to_string(number_);
  if (!name_.empty()) title += " " + name_;
  return title;
}

Constant Func::func_value() const { return Constant(Type::kFunc, number_); }

std::vector<Computed>& Func::args() { return args_; }

const std::vector<Computed>& Func::args() const { return args_; }

std::vector<Type>& Func::result_types() { return result_types_; }

const std::vector<Type>& Func::result_types() const { return result_types_; }

const std::unordered_set<Block*>& Func::blocks() const { return blocks_; }

Block* Func::entry_block() const { return entry_block_; }

void Func::set_entry_block(Block* block) {
  if (block == entry_block_) return;
  if (block != nullptr && block->func() != this)
    throw "tried to set entry block to block not owned by function";

  entry_block_ = block;
  dom_tree_ok_ = false;
}

bool Func::HasBlock(int64_t bnum) const { return block_lookup_.count(bnum) > 0; }

Block* Func::GetBlock(int64_t bnum) const {
  auto it = block_lookup_.find(bnum);
  if (it == block_lookup_.end()) {
    return nullptr;
  }

  return it->second;
}

Block* Func::AddBlock(int64_t bnum) {
  if (bnum < 0) {
    bnum = block_count_++;
  } else {
    if (block_lookup_.count(bnum) != 0) throw "tried to add block with used block number";

    block_count_ = std::max(block_count_, bnum + 1);
  }
  Block* block = new Block(bnum, this);

  blocks_.insert(block);
  block_lookup_.insert({block->number(), block});

  dom_tree_ok_ = false;

  return block;
}

void Func::RemoveBlock(int64_t bnum) {
  auto it = block_lookup_.find(bnum);
  if (it == block_lookup_.end()) throw "tried to remove block not owned by function";

  RemoveBlock(it->second);
}

void Func::RemoveBlock(Block* block) {
  if (block == nullptr) throw "tried to remove nullptr block";
  if (block->func() != this) throw "tried to remove block not owned by function";
  if (entry_block_ == block) entry_block_ = nullptr;

  for (Block* parent : block->parents_) {
    parent->children_.erase(block);
  }
  for (Block* child : block->children_) {
    child->parents_.erase(block);
  }

  blocks_.erase(block);
  block_lookup_.erase(block->number());

  dom_tree_ok_ = false;

  delete block;
}

void Func::AddControlFlow(Block* parent, Block* child) {
  if (parent == nullptr) throw "tried to add control flow to nullptr block";
  if (parent->func() != this) throw "tried to add control flow to block not owned by function";
  if (child == nullptr) throw "tried to add control flow to nullptr block";
  if (child->func() != this) throw "tried to add control flow to block not owned by function";

  parent->children_.insert(child);
  child->parents_.insert(parent);

  dom_tree_ok_ = false;
}

void Func::RemoveControlFlow(Block* parent, Block* child) {
  if (parent == nullptr) throw "tried to remove control flow from nullptr block";
  if (parent->func() != this) throw "tried to remove control flow from block not owned by function";
  if (child == nullptr) throw "tried to remove control flow from nullptr block";
  if (child->func() != this) throw "tried to remove control flow from block not owned by function";

  parent->children_.erase(child);
  child->parents_.erase(parent);

  dom_tree_ok_ = false;
}

std::string Func::ToString() const {
  std::stringstream ss;
  ss << ReferenceString() << " ";
  ss << "(";
  for (size_t i = 0; i < args_.size(); i++) {
    if (i > 0) ss << ", ";
    ss << args_.at(i).ToStringWithType();
  }
  ss << ") => (";
  for (size_t i = 0; i < result_types_.size(); i++) {
    if (i > 0) ss << ", ";
    ss << to_string(result_types_.at(i));
  }
  ss << ") {";

  std::vector<int64_t> bnums;
  bnums.reserve(block_lookup_.size());

  for (auto it : block_lookup_) bnums.push_back(it.first);

  std::sort(bnums.begin(), bnums.end());

  for (int64_t bnum : bnums) {
    ss << "\n" << block_lookup_.at(bnum)->ToString();
  }

  ss << "\n}";
  return ss.str();
}

vcg::Graph Func::ToControlFlowGraph() const {
  vcg::Graph graph;

  for (Block* block : blocks_) {
    graph.nodes().push_back(block->ToVCGNode());

    for (Block* child : block->children()) {
      graph.edges().push_back(vcg::Edge(block->number(), child->number(),
                                        /*is_directed=*/true));
    }
  }

  return graph;
}

vcg::Graph Func::ToDominatorTree() const {
  vcg::Graph graph;

  for (Block* block : blocks_) {
    graph.nodes().push_back(block->ToVCGNode());

    for (Block* child : block->dominees()) {
      graph.edges().push_back(vcg::Edge(block->number(), child->number(),
                                        /*is_directed=*/true));
    }
  }

  return graph;
}

Func::DomTreeContext::DomTreeContext(int64_t n) {
  tree_order_ = std::vector<int64_t>();
  tree_order_.reserve(n);
  tree_parent_ = std::vector<int64_t>(n, -1);
  sdom_ = std::vector<int64_t>(n, -1);
  idom_ = std::vector<int64_t>(n, -1);
  bucket_ = std::vector<std::unordered_set<int64_t>>(n, std::unordered_set<int64_t>());
  ancestor_ = std::vector<int64_t>(n, -1);
  label_ = std::vector<int64_t>(n, -1);
}

Func::DomTreeContext::~DomTreeContext() {}

void Func::UpdateDominatorTree() {
  if (dom_tree_ok_) return;
  if (entry_block_ == nullptr) throw "can not determine dominator tree without entry block";

  for (Block* block : blocks_) {
    block->dominator_ = nullptr;
    block->dominees_.clear();
  }

  DomTreeContext ctx(block_count_);

  FindDFSTree(ctx);
  FindImplicitIDoms(ctx);
  FindExplicitIDoms(ctx);

  for (size_t i = 1; i < ctx.tree_order_.size(); i++) {
    int64_t v = ctx.tree_order_.at(i);
    int64_t w = ctx.idom_.at(v);

    Block* dominator = block_lookup_.at(w);
    Block* dominee = block_lookup_.at(v);

    dominator->dominees_.insert(dominee);
    dominee->dominator_ = nullptr;
  }

  dom_tree_ok_ = true;
}

void Func::FindDFSTree(DomTreeContext& ctx) const {
  std::vector<int64_t> stack;
  std::unordered_set<int64_t> seen;

  stack.push_back(entry_block_->number());
  seen.insert(entry_block_->number());

  while (!stack.empty()) {
    int64_t v = stack.back();
    stack.pop_back();

    ctx.tree_order_.push_back(v);
    ctx.sdom_[v] = ctx.tree_order_.size() - 1;
    ctx.label_[v] = v;

    Block* block = block_lookup_.at(v);

    for (Block* child : block->children_) {
      int64_t w = child->number();
      if (seen.count(w) > 0) continue;

      stack.push_back(w);
      seen.insert(w);

      ctx.tree_parent_[w] = v;
    }
  }
}

void Func::Link(DomTreeContext& ctx, int64_t v, int64_t w) const { ctx.ancestor_[w] = v; }

void Func::Compress(DomTreeContext& ctx, int64_t v) const {
  if (ctx.ancestor_.at(ctx.ancestor_.at(v)) == -1) {
    return;
  }

  Compress(ctx, ctx.ancestor_[v]);

  if (ctx.sdom_.at(ctx.label_.at(ctx.ancestor_.at(v))) < ctx.sdom_.at(ctx.label_.at(v))) {
    ctx.label_[v] = ctx.label_.at(ctx.ancestor_.at(v));
  }

  ctx.ancestor_[v] = ctx.ancestor_.at(ctx.ancestor_.at(v));
}

int64_t Func::Eval(DomTreeContext& ctx, int64_t v) const {
  if (ctx.ancestor_.at(v) == -1) {
    return v;
  }

  Compress(ctx, v);

  return ctx.label_.at(v);
}

void Func::FindImplicitIDoms(DomTreeContext& ctx) const {
  for (size_t i = (int)ctx.tree_order_.size() - 1; i > 0; i--) {
    int64_t w = ctx.tree_order_.at(i);
    Block* block = block_lookup_.at(w);

    // Step 2:
    for (Block* parent : block->parents_) {
      int64_t v = parent->number();
      int64_t u = Eval(ctx, v);

      if (ctx.sdom_.at(w) > ctx.sdom_.at(u)) {
        ctx.sdom_[w] = ctx.sdom_.at(u);
      }
    }

    ctx.bucket_.at(ctx.tree_order_.at(ctx.sdom_.at(w))).insert(w);

    Link(ctx, ctx.tree_parent_.at(w), w);

    // Step 3:
    for (int64_t v : ctx.bucket_.at(ctx.tree_parent_.at(w))) {
      int64_t u = Eval(ctx, v);

      if (ctx.sdom_.at(u) < ctx.sdom_.at(v)) {
        ctx.idom_[v] = u;
      } else {
        ctx.idom_[v] = ctx.tree_parent_.at(w);
      }
    }

    ctx.bucket_.at(ctx.tree_parent_.at(w)).clear();
  }
}

void Func::FindExplicitIDoms(DomTreeContext& ctx) const {
  for (size_t i = 1; i < ctx.tree_order_.size(); i++) {
    int64_t w = ctx.tree_order_.at(i);

    if (ctx.idom_.at(w) != ctx.tree_order_.at(ctx.sdom_.at(w))) {
      ctx.idom_[w] = ctx.idom_.at(ctx.idom_.at(w));
    }
  }
}

}  // namespace ir
