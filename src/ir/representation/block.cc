//
//  block.cc
//  Katara
//
//  Created by Arne Philipeit on 12/23/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "block.h"

#include <iomanip>
#include <sstream>

namespace ir {

using ::common::positions::pos_t;

Instr* Block::ControlFlowInstr() const {
  if (instrs_.empty()) {
    return nullptr;
  } else if (instrs_.back()->IsControlFlowInstr()) {
    return instrs_.back().get();
  } else {
    return nullptr;
  }
}

void Block::ForEachPhiInstr(std::function<void(PhiInstr*)> f) const {
  for (size_t i = 0; i < instrs_.size(); i++) {
    Instr* instr = instrs_.at(i).get();
    if (instr->instr_kind() != InstrKind::kPhi) {
      return;
    }
    f(static_cast<PhiInstr*>(instr));
  }
}

void Block::ForEachPhiInstrReverse(std::function<void(PhiInstr*)> f) const {
  size_t phi_count = 0;
  for (phi_count = 0; phi_count < instrs_.size(); phi_count++) {
    Instr* instr = instrs_.at(phi_count).get();
    if (instr->instr_kind() != InstrKind::kPhi) {
      break;
    }
  }

  for (int64_t i = phi_count - 1; i >= 0; i--) {
    Instr* instr = instrs_.at(i).get();
    f(static_cast<PhiInstr*>(instr));
  }
}

void Block::ForEachNonPhiInstr(std::function<void(Instr*)> f) const {
  for (size_t i = 0; i < instrs_.size(); i++) {
    Instr* instr = instrs_.at(i).get();
    if (instr->instr_kind() == InstrKind::kPhi) {
      continue;
    }
    f(instr);
  }
}

void Block::ForEachNonPhiInstrReverse(std::function<void(Instr*)> f) const {
  for (int64_t i = instrs_.size() - 1; i >= 0; i--) {
    Instr* instr = instrs_.at(i).get();
    if (instr->instr_kind() == InstrKind::kPhi) {
      return;
    }
    f(instr);
  }
}

void Block::SetPositions(pos_t start, pos_t end) {
  start_ = start;
  end_ = end;
}

void Block::WriteRefString(std::ostream& os) const {
  os << "{" << number_ << "}";
  if (!name_.empty()) {
    os << " " << name_;
  }
}

common::graph::Node Block::ToNode() const {
  std::stringstream ss;
  for (size_t i = 0; i < instrs_.size(); i++) {
    if (i > 0) ss << "\n";
    instrs_.at(i)->WriteRefString(ss);
  }

  return common::graph::NodeBuilder(number_, RefString()).SetText(ss.str()).Build();
}

bool Block::operator==(const Block& that) const {
  if (number() != that.number()) return false;
  if (name() != that.name()) return false;
  if (instrs().size() != that.instrs().size()) return false;
  for (std::size_t i = 0; i < instrs().size(); i++) {
    const ir::Instr* instr_a = instrs().at(i).get();
    const ir::Instr* instr_b = that.instrs().at(i).get();
    if (!IsEqual(instr_a, instr_b)) return false;
  }
  if (parents() != that.parents()) return false;
  if (children() != that.children()) return false;
  return true;
}

}  // namespace ir
