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
#include <unordered_set>
#include <vector>

#include "src/common/graph.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"

namespace ir {

class Block {
 public:
  Block(block_num_t bnum) : number_(bnum) {}

  block_num_t number() const { return number_; }
  std::string name() const { return name_; }
  void set_name(std::string name) { name_ = name; }

  std::string ReferenceString() const;

  const std::vector<std::unique_ptr<Instr>>& instrs() const { return instrs_; }
  std::vector<std::unique_ptr<Instr>>& instrs() { return instrs_; }

  void for_each_phi_instr(std::function<void(PhiInstr*)> f);
  void for_each_phi_instr_reverse(std::function<void(PhiInstr*)> f);
  void for_each_non_phi_instr(std::function<void(Instr*)> f);
  void for_each_non_phi_instr_reverse(std::function<void(Instr*)> f);

  const std::unordered_set<block_num_t>& parents() const { return parents_; }
  const std::unordered_set<block_num_t>& children() const { return children_; }

  std::string ToString() const;
  common::Node ToVCGNode() const;

  friend class Func;

 private:
  block_num_t number_;
  std::string name_;

  std::vector<std::unique_ptr<Instr>> instrs_;

  std::unordered_set<block_num_t> parents_;
  std::unordered_set<block_num_t> children_;
};

}  // namespace ir

#endif /* ir_block_h */
