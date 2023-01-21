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

#include "src/common/graph/graph.h"
#include "src/common/positions/positions.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"

namespace ir {

class Block : public Object {
 public:
  Block(block_num_t bnum) : number_(bnum) {}

  constexpr Object::Kind object_kind() const final { return Object::Kind::kBlock; }

  block_num_t number() const { return number_; }
  std::string name() const { return name_; }
  void set_name(std::string name) { name_ = name; }

  const std::vector<std::unique_ptr<Instr>>& instrs() const { return instrs_; }
  std::vector<std::unique_ptr<Instr>>& instrs() { return instrs_; }

  bool HasControlFlowInstr() const { return ControlFlowInstr() != nullptr; }
  Instr* ControlFlowInstr() const;

  void ForEachPhiInstr(std::function<void(PhiInstr*)> f) const;
  void ForEachPhiInstrReverse(std::function<void(PhiInstr*)> f) const;
  void ForEachNonPhiInstr(std::function<void(Instr*)> f) const;
  void ForEachNonPhiInstrReverse(std::function<void(Instr*)> f) const;

  const std::unordered_set<block_num_t>& parents() const { return parents_; }
  const std::unordered_set<block_num_t>& children() const { return children_; }

  common::positions::pos_t start() const { return start_; }
  common::positions::pos_t end() const { return end_; }
  void SetPositions(common::positions::pos_t start, common::positions::pos_t end);
  void ClearPositions() { SetPositions(common::positions::kNoPos, common::positions::kNoPos); }

  void WriteRefString(std::ostream& os) const override;

  common::graph::Node ToNode() const;

  bool operator==(const Block& that) const;

  friend class Func;

 private:
  block_num_t number_;
  std::string name_;

  std::vector<std::unique_ptr<Instr>> instrs_;

  std::unordered_set<block_num_t> parents_;
  std::unordered_set<block_num_t> children_;

  common::positions::pos_t start_ = common::positions::kNoPos;
  common::positions::pos_t end_ = common::positions::kNoPos;
};

constexpr bool IsEqual(const Block* block_a, const Block* block_b) {
  if (block_a == block_b) return true;
  if (block_a == nullptr || block_b == nullptr) return false;
  return *block_a == *block_b;
}

}  // namespace ir

#endif /* ir_block_h */
