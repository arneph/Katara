//
//  func.cc
//  Katara
//
//  Created by Arne Philipeit on 12/23/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "func.h"

#include <sstream>

#include "src/common/logging.h"

namespace ir {

std::string Func::ReferenceString() const {
  std::string title = "@" + std::to_string(number_);
  if (!name_.empty()) title += " " + name_;
  return title;
}

Block* Func::GetBlock(block_num_t bnum) const {
  auto it = std::find_if(blocks_.begin(), blocks_.end(),
                         [=](auto& block) { return block->number() == bnum; });
  return (it != blocks_.end()) ? it->get() : nullptr;
}

Block* Func::AddBlock(block_num_t bnum) {
  if (bnum == kNoBlockNum) {
    bnum = block_count_++;
  } else if (bnum < block_count_ && HasBlock(bnum)) {
    common::fail("tried to add block with used block number");
  } else {
    block_count_ = std::max(block_count_, bnum + 1);
  }
  auto& block = blocks_.emplace_back(new Block(bnum));
  dominator_tree_ok_ = false;
  return block.get();
}

void Func::RemoveBlock(block_num_t bnum) {
  auto it = std::find_if(blocks_.begin(), blocks_.end(),
                         [=](auto& block) { return block->number() == bnum; });
  if (it == blocks_.end()) common::fail("tried to remove block not owned by function");
  if (entry_block_num_ == bnum) entry_block_num_ = kNoBlockNum;
  Block* block = it->get();
  for (block_num_t parent_num : block->parents()) {
    Block* parent = GetBlock(parent_num);
    parent->children_.erase(bnum);
  }
  for (block_num_t child_num : block->children()) {
    Block* child = GetBlock(child_num);
    child->parents_.erase(bnum);
  }
  blocks_.erase(it);
  dominator_tree_ok_ = false;
}

void Func::AddControlFlow(block_num_t parent_num, block_num_t child_num) {
  Block* parent = GetBlock(parent_num);
  Block* child = GetBlock(child_num);
  if (parent == nullptr) common::fail("tried to add control flow to unknown block");
  if (child == nullptr) common::fail("tried to add control flow to unknown block");
  parent->children_.insert(child_num);
  child->parents_.insert(parent_num);
  dominator_tree_ok_ = false;
}

void Func::RemoveControlFlow(block_num_t parent_num, block_num_t child_num) {
  Block* parent = GetBlock(parent_num);
  Block* child = GetBlock(child_num);
  if (parent == nullptr) common::fail("tried to remove control flow from nullptr block");
  if (child == nullptr) common::fail("tried to remove control flow from nullptr block");
  parent->children_.erase(child_num);
  child->parents_.erase(parent_num);
  dominator_tree_ok_ = false;
}

block_num_t Func::DominatorOf(block_num_t dominee_num) const {
  if (!dominator_tree_ok_) {
    UpdateDominatorTree();
  }
  return dominators_.at(dominee_num);
}

std::unordered_set<block_num_t> Func::DomineesOf(block_num_t dominator_num) const {
  if (!dominator_tree_ok_) {
    UpdateDominatorTree();
  }
  return dominees_.at(dominator_num);
}

std::vector<block_num_t> Func::GetBlocksInDominanceOrder() const {
  if (!dominator_tree_ok_) {
    UpdateDominatorTree();
  }
  std::vector<block_num_t> ordered_blocks;
  ordered_blocks.reserve(blocks_.size());
  ordered_blocks.push_back(entry_block_num_);
  for (std::size_t i = 0; i < ordered_blocks.size(); i++) {
    block_num_t current = ordered_blocks.at(i);
    for (block_num_t dominee : dominees_.at(current)) {
      ordered_blocks.push_back(dominee);
    }
  }
  return ordered_blocks;
}

void Func::ForBlocksInDominanceOrder(std::function<void(Block*)> f) const {
  std::vector<block_num_t> ordered_block_nums = GetBlocksInDominanceOrder();
  for (block_num_t current_num : ordered_block_nums) {
    Block* current_block = GetBlock(current_num);
    f(current_block);
  }
}

std::string Func::ToString() const {
  std::stringstream ss;
  ss << ReferenceString() << " ";
  ss << "(";
  for (size_t i = 0; i < args_.size(); i++) {
    if (i > 0) ss << ", ";
    ss << args_.at(i)->ToStringWithType();
  }
  ss << ") => (";
  for (size_t i = 0; i < result_types_.size(); i++) {
    if (i > 0) ss << ", ";
    ss << result_types_.at(i)->ToString();
  }
  ss << ") {";
  std::vector<block_num_t> bnums;
  bnums.reserve(blocks_.size());
  for (auto& block : blocks_) {
    bnums.push_back(block->number());
  }
  std::sort(bnums.begin(), bnums.end());
  for (block_num_t bnum : bnums) {
    ss << "\n" << GetBlock(bnum)->ToString();
  }
  ss << "\n}";
  return ss.str();
}

common::Graph Func::ToControlFlowGraph() const {
  common::Graph graph(/*is_directed=*/true);

  for (auto& block : blocks_) {
    graph.nodes().push_back(block->ToVCGNode());

    for (block_num_t child_num : block->children()) {
      graph.edges().push_back(common::Edge(block->number(), child_num));
    }
  }

  return graph;
}

common::Graph Func::ToDominatorTree() const {
  common::Graph graph(/*is_directed=*/true);

  for (auto& block : blocks_) {
    graph.nodes().push_back(block->ToVCGNode());

    for (block_num_t dominee_num : DomineesOf(block->number())) {
      graph.edges().push_back(common::Edge(block->number(), dominee_num));
    }
  }

  return graph;
}

Func::DomTreeContext::DomTreeContext(int64_t block_count) {
  tree_order_ = std::vector<block_num_t>();
  tree_order_.reserve(block_count);
  tree_parent_ = std::vector<block_num_t>(block_count, -1);
  sdom_ = std::vector<tree_num_t>(block_count, -1);
  idom_ = std::vector<block_num_t>(block_count, -1);
  bucket_ =
      std::vector<std::unordered_set<block_num_t>>(block_count, std::unordered_set<block_num_t>());
  ancestor_ = std::vector<block_num_t>(block_count, -1);
  label_ = std::vector<block_num_t>(block_count, -1);
}

void Func::UpdateDominatorTree() const {
  if (dominator_tree_ok_) return;
  if (entry_block_num_ == kNoBlockNum) {
    common::fail("can not determine dominator tree without entry block");
  }

  DomTreeContext ctx(block_count_);
  FindDFSTree(ctx);
  FindImplicitIDoms(ctx);
  FindExplicitIDoms(ctx);

  dominators_.clear();
  dominees_.clear();
  dominators_.reserve(blocks_.size());
  dominees_.reserve(blocks_.size());
  for (auto& block : blocks_) {
    dominators_.insert({block->number(), kNoBlockNum});
    dominees_.insert({block->number(), std::unordered_set<block_num_t>{}});
  }
  for (size_t i = 1; i < ctx.tree_order_.size(); i++) {
    block_num_t dominee_num = ctx.tree_order_.at(i);
    block_num_t dominator_num = ctx.idom_.at(dominee_num);

    dominators_.insert({dominee_num, dominator_num});
    dominees_.at(dominator_num).insert(dominee_num);
  }

  dominator_tree_ok_ = true;
}

void Func::FindDFSTree(DomTreeContext& ctx) const {
  std::vector<block_num_t> stack;
  std::unordered_set<block_num_t> seen;

  stack.push_back(entry_block_num_);
  seen.insert(entry_block_num_);

  while (!stack.empty()) {
    int64_t v = stack.back();
    stack.pop_back();

    ctx.tree_order_.push_back(v);
    ctx.sdom_[v] = ctx.tree_order_.size() - 1;
    ctx.label_[v] = v;

    Block* block = GetBlock(v);

    for (block_num_t w : block->children_) {
      if (seen.count(w) > 0) continue;

      stack.push_back(w);
      seen.insert(w);

      ctx.tree_parent_[w] = v;
    }
  }
}

void Func::Compress(DomTreeContext& ctx, block_num_t v) const {
  if (ctx.ancestor_.at(ctx.ancestor_.at(v)) == -1) {
    return;
  }

  Compress(ctx, ctx.ancestor_[v]);

  if (ctx.sdom_.at(ctx.label_.at(ctx.ancestor_.at(v))) < ctx.sdom_.at(ctx.label_.at(v))) {
    ctx.label_[v] = ctx.label_.at(ctx.ancestor_.at(v));
  }

  ctx.ancestor_[v] = ctx.ancestor_.at(ctx.ancestor_.at(v));
}

block_num_t Func::Eval(DomTreeContext& ctx, block_num_t v) const {
  if (ctx.ancestor_.at(v) == -1) {
    return v;
  }

  Compress(ctx, v);

  return ctx.label_.at(v);
}

void Func::FindImplicitIDoms(DomTreeContext& ctx) const {
  for (tree_num_t i = (tree_num_t)ctx.tree_order_.size() - 1; i > 0; i--) {
    block_num_t w = ctx.tree_order_.at(i);
    Block* block = GetBlock(w);

    // Step 2:
    for (block_num_t v : block->parents_) {
      int64_t u = Eval(ctx, v);

      if (ctx.sdom_.at(w) > ctx.sdom_.at(u)) {
        ctx.sdom_[w] = ctx.sdom_.at(u);
      }
    }

    ctx.bucket_.at(ctx.tree_order_.at(ctx.sdom_.at(w))).insert(w);

    Link(ctx, ctx.tree_parent_.at(w), w);

    // Step 3:
    for (block_num_t v : ctx.bucket_.at(ctx.tree_parent_.at(w))) {
      block_num_t u = Eval(ctx, v);

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
  for (tree_num_t i = 1; i < (tree_num_t)ctx.tree_order_.size(); i++) {
    block_num_t w = ctx.tree_order_.at(i);

    if (ctx.idom_.at(w) != ctx.tree_order_.at(ctx.sdom_.at(w))) {
      ctx.idom_[w] = ctx.idom_.at(ctx.idom_.at(w));
    }
  }
}

}  // namespace ir
