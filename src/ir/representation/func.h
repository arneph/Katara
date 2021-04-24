//
//  func.h
//  Katara
//
//  Created by Arne Philipeit on 12/23/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_func_h
#define ir_func_h

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ir/representation/block.h"
#include "ir/representation/value.h"
#include "vcg/graph.h"

namespace ir {

class Program;

class Func {
 public:
  int64_t number() const;
  std::string name() const;
  void set_name(std::string name);

  std::string ReferenceString() const;

  Constant func_value() const;

  std::vector<Computed>& args();
  const std::vector<Computed>& args() const;
  std::vector<Type>& result_types();
  const std::vector<Type>& result_types() const;

  const std::unordered_set<Block*>& blocks() const;

  Block* entry_block() const;
  void set_entry_block(Block* block);

  bool HasBlock(int64_t bnum) const;
  Block* GetBlock(int64_t bnum) const;
  Block* AddBlock(int64_t bnum = -1);
  void RemoveBlock(int64_t bnum);
  void RemoveBlock(Block* block);

  void AddControlFlow(Block* parent, Block* child);
  void RemoveControlFlow(Block* parent, Block* child);

  std::string ToString() const;
  vcg::Graph ToControlFlowGraph() const;
  vcg::Graph ToDominatorTree() const;

  friend Program;
  friend Block;

 private:
  struct DomTreeContext {
    DomTreeContext(int64_t n);
    ~DomTreeContext();

    std::vector<int64_t> tree_order_;   // tree index   -> vertex index
    std::vector<int64_t> tree_parent_;  // vertex index -> vertex index
    std::vector<int64_t> sdom_;         // vertex index -> tree index
    std::vector<int64_t> idom_;         // vertex index -> vertex index
    std::vector<std::unordered_set<int64_t>> bucket_;
    // vertex index -> vertex indices
    std::vector<int64_t> ancestor_;  // vertex index -> vertex index
    std::vector<int64_t> label_;     // vertex index -> vertex index
  };

  int64_t number_;
  std::string name_;
  Program* prog_;

  std::vector<Computed> args_;
  std::vector<Type> result_types_;

  int64_t block_count_ = 0;
  std::unordered_set<Block*> blocks_;
  std::unordered_map<int64_t, Block*> block_lookup_;

  Block* entry_block_ = nullptr;

  bool dom_tree_ok_ = false;

  int64_t instr_count_ = 0;
  std::unordered_map<int64_t, Instr*> instr_lookup_;

  Func(int64_t number, Program* prog);
  ~Func();

  void UpdateDominatorTree();
  void FindDFSTree(DomTreeContext& ctx) const;
  void Link(DomTreeContext& ctx, int64_t v, int64_t w) const;
  void Compress(DomTreeContext& ctx, int64_t v) const;
  int64_t Eval(DomTreeContext& ctx, int64_t v) const;
  void FindImplicitIDoms(DomTreeContext& ctx) const;
  void FindExplicitIDoms(DomTreeContext& ctx) const;
};

}  // namespace ir

#endif /* ir_func_h */
