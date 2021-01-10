//
//  block.h
//  Katara
//
//  Created by Arne Philipeit on 12/23/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_block_h
#define ir_block_h

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ir/instr.h"
#include "ir/value.h"
#include "vcg/node.h"

namespace ir {

class Func;

class Block {
 public:
  int64_t number() const;
  std::string name() const;
  void set_name(std::string name);
  Func* func() const;

  std::string ReferenceString() const;

  BlockValue block_value() const;

  const std::vector<Instr*>& instrs() const;

  void for_each_phi_instr(std::function<void(PhiInstr*)> f);
  void for_each_phi_instr_reverse(std::function<void(PhiInstr*)> f);
  void for_each_non_phi_instr(std::function<void(Instr*)> f);
  void for_each_non_phi_instr_reverse(std::function<void(Instr*)> f);

  void AddInstr(Instr* instr);
  void InsertInstr(size_t index, Instr* instr);
  void RemoveInstr(int64_t inum);
  void RemoveInstr(Instr* instr);

  const std::unordered_set<Block*>& parents() const;
  const std::unordered_set<Block*>& children() const;

  bool HasBranchingParent() const;
  Block* BranchingParent() const;
  bool HasMergingChild() const;
  Block* MergingChild() const;
  // only a merging child can have phi instructions

  Block* dominator() const;
  const std::unordered_set<Block*>& dominees() const;

  std::string ToString() const;
  vcg::Node ToVCGNode() const;

  friend Func;

 private:
  Block(int64_t number, Func* func);
  ~Block();

  int64_t number_;
  std::string name_;
  Func* func_;

  std::vector<Instr*> instrs_;

  std::unordered_set<Block*> parents_;
  std::unordered_set<Block*> children_;

  Block* dominator_;
  std::unordered_set<Block*> dominees_;
};

}  // namespace ir

#endif /* ir_block_h */
