//
//  positions.cc
//  Katara
//
//  Created by Arne Philipeit on 10/6/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#include "positions.h"

#include "src/common/logging/logging.h"

namespace ir_serialization {

using common::logging::fail;
using common::positions::kNoRange;
using common::positions::range_t;

const FuncPositions& ProgramPositions::GetFuncPositions(const ir::Func* func) const {
  if (func == nullptr) {
    fail("Attempted to get FuncPositions for nullptr function.");
  } else if (auto it = func_positions_.find(func); it != func_positions_.end()) {
    return it->second;
  } else {
    fail("Attempted to get FuncPositions for unknown function: " + func->RefString());
  }
}

void ProgramPositions::AddFuncPositions(const ir::Func* func, FuncPositions func_positions) {
  func_positions_.insert_or_assign(func, func_positions);
}

const BlockPositions& ProgramPositions::GetBlockPositions(const ir::Block* block) const {
  if (block == nullptr) {
    fail("Attempted to get BlockPositions for nullptr block.");
  } else if (auto it = block_positions_.find(block); it != block_positions_.end()) {
    return it->second;
  } else {
    fail("Attempted to get BlockPositions for unknown block: " + block->RefString());
  }
}

void ProgramPositions::AddBlockPositions(const ir::Block* block, BlockPositions block_positions) {
  block_positions_.insert_or_assign(block, block_positions);
}

const InstrPositions& ProgramPositions::GetInstrPositions(const ir::Instr* instr) const {
  if (instr == nullptr) {
    fail("Attempted to get InstrPositions for nullptr instruction.");
  } else if (auto it = instr_positions_.find(instr); it != instr_positions_.end()) {
    return it->second;
  } else {
    fail("Attempted to get InstrPositions for unknown instruction: " + instr->RefString());
  }
}

void ProgramPositions::AddInstrPositions(const ir::Instr* instr, InstrPositions instr_positions) {
  instr_positions_.insert_or_assign(instr, instr_positions);
}

range_t FuncPositions::entire_func() const {
  return range_t{
      .start = number_.start,
      .end = body_.end,
  };
}

range_t FuncPositions::header() const {
  return range_t{
      .start = number_.start,
      .end = results_range_.end,
  };
}

range_t BlockPositions::entire_block() const {
  return range_t{
      .start = number_.start,
      .end = body_.end,
  };
}

range_t BlockPositions::header() const {
  if (name_ == kNoRange) {
    return number_;
  } else {
    return range_t{
        .start = number_.start,
        .end = name_.end,
    };
  }
}

range_t InstrPositions::entire_instr() const {
  return range_t{
      .start = (defined_value_ranges_.empty()) ? name_.start : defined_value_ranges_.front().start,
      .end = (used_value_ranges_.empty()) ? name_.end : used_value_ranges_.back().end,
  };
}

range_t InstrPositions::defined_values_range() const {
  if (defined_value_ranges_.empty()) {
    return kNoRange;
  } else {
    return range_t{
        .start = defined_value_ranges_.front().start,
        .end = defined_value_ranges_.back().end,
    };
  }
}

range_t InstrPositions::used_values_range() const {
  if (used_value_ranges_.empty()) {
    return kNoRange;
  } else {
    return range_t{
        .start = used_value_ranges_.front().start,
        .end = used_value_ranges_.back().end,
    };
  }
}

}  // namespace ir_serialization
