//
//  func.h
//  Katara
//
//  Created by Arne Philipeit on 12/23/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_func_h
#define ir_func_h

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "src/common/graph/graph.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"

namespace ir {

class Func : public Object {
 public:
  Func(func_num_t fnum) : number_(fnum) {}

  constexpr Object::Kind object_kind() const final { return Object::Kind::kFunc; }

  func_num_t number() const { return number_; }
  std::string name() const { return name_; }
  void set_name(std::string name) { name_ = name; }

  std::string ReferenceString() const;

  std::vector<std::shared_ptr<Computed>>& args() { return args_; }
  const std::vector<std::shared_ptr<Computed>>& args() const { return args_; }
  std::vector<const Type*>& result_types() { return result_types_; }
  const std::vector<const Type*>& result_types() const { return result_types_; }

  const std::vector<std::unique_ptr<Block>>& blocks() const { return blocks_; }

  Block* entry_block() const { return GetBlock(entry_block_num_); }
  block_num_t entry_block_num() const { return entry_block_num_; }
  void set_entry_block_num(block_num_t entry_block_num) { entry_block_num_ = entry_block_num; }

  bool HasBlock(block_num_t bnum) const { return GetBlock(bnum) != nullptr; }
  Block* GetBlock(block_num_t bnum) const;
  Block* AddBlock(block_num_t bnum = kNoBlockNum);
  void RemoveBlock(block_num_t bnum);

  void AddControlFlow(block_num_t parent, block_num_t child);
  void RemoveControlFlow(block_num_t parent, block_num_t child);

  block_num_t DominatorOf(block_num_t dominee_num) const;
  std::unordered_set<block_num_t> DomineesOf(block_num_t dominator_num) const;
  std::vector<block_num_t> GetBlocksInDominanceOrder() const;
  void ForBlocksInDominanceOrder(std::function<void(Block*)> f) const;

  int64_t computed_count() const { return computed_count_; }
  value_num_t next_computed_number() { return computed_count_++; }

  std::string ToString() const override;
  common::Graph ToControlFlowGraph() const;
  common::Graph ToDominatorTree() const;

 private:
  struct DomTreeContext {
    DomTreeContext(int64_t block_count);

    std::vector<block_num_t> tree_order_;   // tree_num_t -> block_num_t
    std::vector<block_num_t> tree_parent_;  // block_num_t -> block_num_t
    std::vector<tree_num_t> sdom_;          // block_num_t -> tree_num_t
    std::vector<block_num_t> idom_;         // block_num_t -> block_num_t
    std::vector<std::unordered_set<block_num_t>>
        bucket_;                         // block_num_t -> std::unordered_set<block_num_t>
    std::vector<block_num_t> ancestor_;  // block_num_t -> block_num_t
    std::vector<block_num_t> label_;     // block_num_t -> block_num_t
  };

  void UpdateDominatorTree() const;
  void FindDFSTree(DomTreeContext& ctx) const;
  void Link(DomTreeContext& ctx, block_num_t v, block_num_t w) const { ctx.ancestor_[w] = v; }
  void Compress(DomTreeContext& ctx, block_num_t v) const;
  block_num_t Eval(DomTreeContext& ctx, block_num_t v) const;
  void FindImplicitIDoms(DomTreeContext& ctx) const;
  void FindExplicitIDoms(DomTreeContext& ctx) const;

  func_num_t number_;
  std::string name_;

  std::vector<std::shared_ptr<Computed>> args_;
  std::vector<const Type*> result_types_;

  int64_t block_count_ = 0;
  std::vector<std::unique_ptr<Block>> blocks_;

  block_num_t entry_block_num_ = kNoBlockNum;

  mutable bool dominator_tree_ok_ = false;
  mutable std::unordered_map<block_num_t, block_num_t> dominators_;
  mutable std::unordered_map<block_num_t, std::unordered_set<block_num_t>> dominees_;

  int64_t computed_count_ = 0;
};

}  // namespace ir

#endif /* ir_func_h */
