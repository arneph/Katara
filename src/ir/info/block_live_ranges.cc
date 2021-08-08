//
//  block_live_ranges.cc
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "block_live_ranges.h"

#include <iomanip>
#include <sstream>

namespace ir_info {

bool BlockLiveRanges::HasValue(ir::value_num_t value) const {
  return value_ranges_.count(value) != 0;
}

bool BlockLiveRanges::HasValueDefinition(ir::value_num_t value) const {
  if (value_ranges_.count(value) == 0) {
    return false;
  }
  return value_ranges_.at(value).start_instr_ != nullptr;
}

void BlockLiveRanges::AddValueDefinition(ir::value_num_t value, const ir::Instr* instr) {
  if (auto it = value_ranges_.find(value); it != value_ranges_.end()) {
    it->second.start_instr_ = instr;
  } else {
    value_ranges_.insert({value, ValueRange{.start_instr_ = instr, .end_instr_ = instr}});
  }
}

void BlockLiveRanges::AddValueUse(ir::value_num_t value, const ir::Instr* instr) {
  if (auto it = value_ranges_.find(value); it != value_ranges_.end()) {
    if (it->second.end_instr_ != nullptr && InstrsAreOrdered(it->second.end_instr_, instr)) {
      it->second.end_instr_ = instr;
    }
  } else {
    value_ranges_.insert({value, ValueRange{.start_instr_ = nullptr, .end_instr_ = instr}});
  }
}

void BlockLiveRanges::PropagateBackwardsFromExitSet(ir::value_num_t value) {
  if (auto it = value_ranges_.find(value); it != value_ranges_.end()) {
    it->second.end_instr_ = nullptr;

  } else {
    value_ranges_.insert({value, ValueRange{.start_instr_ = nullptr, .end_instr_ = nullptr}});
  }
}

std::unordered_set<ir::value_num_t> BlockLiveRanges::GetEntrySet() const {
  std::unordered_set<ir::value_num_t> entry_set;
  for (auto& [value, range] : value_ranges_) {
    if (range.start_instr_ == nullptr) {
      entry_set.insert(value);
    }
  }
  return entry_set;
}

std::unordered_set<ir::value_num_t> BlockLiveRanges::GetExitSet() const {
  std::unordered_set<ir::value_num_t> exit_set;
  for (auto& [value, range] : value_ranges_) {
    if (range.end_instr_ == nullptr) {
      exit_set.insert(value);
    }
  }
  return exit_set;
}

std::unordered_set<ir::value_num_t> BlockLiveRanges::GetLiveSet(const ir::Instr* instr) const {
  std::unordered_set<ir::value_num_t> live_set;
  for (auto& [value, range] : value_ranges_) {
    if (InstrIsInRange(instr, range)) {
      live_set.insert(value);
    }
  }
  return live_set;
}

bool BlockLiveRanges::InstrsAreOrdered(const ir::Instr* instr_a, const ir::Instr* instr_b) const {
  std::size_t index_a = 0;
  std::size_t index_b = 0;
  for (std::size_t i = 0; i < block_->instrs().size(); i++) {
    const ir::Instr* instr = block_->instrs().at(i).get();
    if (instr == instr_a) {
      index_a = i;
    }
    if (instr == instr_b) {
      index_b = i;
    }
  }
  return index_a <= index_b;
}

bool BlockLiveRanges::InstrIsInRange(const ir::Instr* needle_instr, const ValueRange& range) const {
  std::size_t needle_index = 0;
  std::size_t range_start = 0;
  std::size_t range_end = block_->instrs().size();
  for (std::size_t i = 0; i < block_->instrs().size(); i++) {
    const ir::Instr* instr = block_->instrs().at(i).get();
    if (instr == needle_instr) {
      needle_index = i;
    }
    if (instr == range.start_instr_) {
      range_start = i;
    }
    if (instr == range.end_instr_) {
      range_end = i;
    }
  }
  return range_start <= needle_index && needle_index <= range_end;
}

std::string BlockLiveRanges::ToString() const {
  std::stringstream ss;

  ss << std::setw(5) << std::setfill(' ') << block_->ReferenceString() << " - live ranges:\n";
  for (auto& [value, range] : value_ranges_) {
    if (range.start_instr_ == nullptr) {
      ss << '<';
    } else {
      ss << ' ';
    }
    for (size_t i = 0; i < block_->instrs().size(); i++) {
      ir::Instr* instr = block_->instrs().at(i).get();
      if (instr == range.start_instr_ || instr == range.end_instr_) {
        ss << '+';
      } else if (InstrIsInRange(instr, range)) {
        ss << '-';
      } else {
        ss << ' ';
      }
    }
    if (range.end_instr_ == nullptr) {
      ss << '>';
    } else {
      ss << ' ';
    }

    ss << " %" << value << '\n';
  }

  ss << "entry set: ";
  bool first = true;
  for (ir::value_num_t value : GetEntrySet()) {
    if (first) {
      first = false;
    } else {
      ss << ", ";
    }
    ss << "%" << value;
  }
  ss << '\n';

  ss << " exit set: ";
  first = true;
  for (ir::value_num_t value : GetExitSet()) {
    if (first) {
      first = false;
    } else {
      ss << ", ";
    }
    ss << "%" << value;
  }
  ss << '\n';

  return ss.str();
}

}  // namespace ir_info
