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

std::string Block::ReferenceString() const {
  std::string title = "{" + std::to_string(number_) + "}";
  if (!name_.empty()) title += " " + name_;
  return title;
}

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

std::string Block::ToString() const {
  std::stringstream ss;
  ss << ReferenceString();
  for (auto& instr : instrs_) {
    ss << "\n";
    ss << instr->ToString();
  }
  return ss.str();
}

common::Node Block::ToNode() const {
  std::stringstream ss;
  for (size_t i = 0; i < instrs_.size(); i++) {
    if (i > 0) ss << "\n";
    ss << instrs_.at(i)->ToString();
  }

  return common::NodeBuilder(number_, ReferenceString()).SetText(ss.str()).Build();
}

}  // namespace ir
