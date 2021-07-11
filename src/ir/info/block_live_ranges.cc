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
  return value_ranges_.at(value).start_index_ >= 0;
}

void BlockLiveRanges::AddValueDefinition(ir::value_num_t value, int64_t index) {
  if (auto it = value_ranges_.find(value); it != value_ranges_.end()) {
    it->second.start_index_ = index;

  } else {
    value_ranges_.insert({value, ValueRange{.start_index_ = index, .end_index_ = index}});
  }
}

void BlockLiveRanges::AddValueUse(ir::value_num_t value, int64_t index) {
  if (auto it = value_ranges_.find(value); it != value_ranges_.end()) {
    it->second.start_index_ = std::min(it->second.start_index_, index);
    it->second.end_index_ = std::max(it->second.end_index_, index);
  } else {
    value_ranges_.insert({value, ValueRange{.start_index_ = -1, .end_index_ = index}});
  }
}

void BlockLiveRanges::PropagateBackwardsFromExitSet(ir::value_num_t value) {
  const int64_t exit_index = block_->instrs().size();

  if (auto it = value_ranges_.find(value); it != value_ranges_.end()) {
    it->second.end_index_ = exit_index;

  } else {
    value_ranges_.insert({value, ValueRange{.start_index_ = -1, .end_index_ = exit_index}});
  }
}

std::unordered_set<ir::value_num_t> BlockLiveRanges::GetEntrySet() const {
  std::unordered_set<ir::value_num_t> entry_set;
  for (auto& [value, range] : value_ranges_) {
    if (range.start_index_ < 0) {
      entry_set.insert(value);
    }
  }
  return entry_set;
}

std::unordered_set<ir::value_num_t> BlockLiveRanges::GetExitSet() const {
  std::unordered_set<ir::value_num_t> exit_set;
  for (auto& [value, range] : value_ranges_) {
    if (range.end_index_ >= int64_t(block_->instrs().size())) {
      exit_set.insert(value);
    }
  }
  return exit_set;
}

std::unordered_set<ir::value_num_t> BlockLiveRanges::GetLiveSet(int64_t index) const {
  std::unordered_set<ir::value_num_t> live_set;
  for (auto& [value, range] : value_ranges_) {
    if (range.start_index_ <= index && index <= range.end_index_) {
      live_set.insert(value);
    }
  }
  return live_set;
}

std::string BlockLiveRanges::ToString() const {
  std::stringstream ss;

  ss << std::setw(5) << std::setfill(' ') << block_->ReferenceString() << " - live ranges:\n";
  for (auto& [value, range] : value_ranges_) {
    if (range.start_index_ < 0) {
      ss << '<';
    } else {
      ss << ' ';
    }
    for (size_t i = 0; i < block_->instrs().size(); i++) {
      if (range.start_index_ < int64_t(i) && int64_t(i) < range.end_index_) {
        ss << '-';
      } else if (int64_t(i) == range.start_index_ || int64_t(i) == range.end_index_) {
        ss << '+';
      } else {
        ss << ' ';
      }
    }
    if (range.end_index_ == int64_t(block_->instrs().size())) {
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
