//
//  data_view.cc
//  Katara
//
//  Created by Arne Philipeit on 12/11/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "data_view.h"

#include <string>

#include "src/common/logging/logging.h"

namespace common::data {

using ::common::logging::fail;

DataView::DataView(uint8_t* base, int64_t size) : base_(base), size_(size) {
  if (size < 0) {
    fail("size is negative");
  }
}

void DataView::CheckIndex(int64_t index) const {
  if (index < 0 || index >= size_) {
    fail("index is out of bounds, size: " + std::to_string(size_) +
         ", index: " + std::to_string(index));
  }
}

void DataView::CheckSubViewIndices(int64_t start_index, int64_t end_index) const {
  CheckIndex(start_index);
  CheckIndex(end_index);
  if (start_index > end_index) {
    fail("subview start index is greater than end index");
  }
}

const uint8_t& DataView::operator[](int64_t index) const {
  CheckIndex(index);
  return base_[index];
}

uint8_t& DataView::operator[](int64_t index) {
  CheckIndex(index);
  return base_[index];
}

const DataView DataView::SubView(int64_t start_index) const {
  CheckIndex(start_index);
  return DataView(base_ + start_index, size_ - start_index);
}

const DataView DataView::SubView(int64_t start_index, int64_t end_index) const {
  CheckSubViewIndices(start_index, end_index);
  return DataView(base_ + start_index, end_index - start_index);
}

DataView DataView::SubView(int64_t start_index) {
  CheckIndex(start_index);
  return DataView(base_ + start_index, size_ - start_index);
}

DataView DataView::SubView(int64_t start_index, int64_t end_index) {
  CheckSubViewIndices(start_index, end_index);
  return DataView(base_ + start_index, end_index - start_index);
}

}  // namespace common::data
