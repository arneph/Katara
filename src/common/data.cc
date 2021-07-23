//
//  data.cpp
//  Katara
//
//  Created by Arne Philipeit on 12/11/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "data.h"

#include <string>

#include "src/common/logging.h"

namespace common {

data::data(uint8_t* base, int64_t size) : base_(base), size_(size) {
  if (size < 0) {
    common::fail("size is negative");
  }
}

uint8_t* data::base() const { return base_; }

int64_t data::size() const { return size_; }

uint8_t& data::operator[](int64_t index) {
  if (index < 0 || index >= size_)
    common::fail("index out of bounds exception, size: " + std::to_string(size_) +
                 ", index: " + std::to_string(index));
  return base_[index];
}

const uint8_t& data::operator[](int64_t index) const {
  if (index < 0 || index >= size_)
    common::fail("index out of bounds exception, size: " + std::to_string(size_) +
                 ", index: " + std::to_string(index));
  return base_[index];
}

data data::view(int64_t start_index) const {
  if (start_index < 0 || start_index > size_) {
    common::fail("index out of bounds exception, size: " + std::to_string(size_) +
                 ", index: " + std::to_string(start_index));
  }

  return data(base_ + start_index, size_ - start_index);
}

data data::view(int64_t start_index, int64_t end_index) const {
  if (start_index < 0 || start_index > size_) {
    common::fail("index out of bounds exception, size: " + std::to_string(size_) +
                 ", index: " + std::to_string(start_index));
  }
  if (end_index < 0 || end_index > size_) {
    common::fail("index out of bounds exception, size: " + std::to_string(size_) +
                 ", index: " + std::to_string(end_index));
  }
  if (start_index > end_index) {
    common::fail("start index is greater than end index");
  }

  return data(base_ + start_index, end_index - start_index);
}

dummy_data::dummy_data() : data(nullptr, 0) {}

uint8_t& dummy_data::operator[](int64_t) { return dummy_; }

const uint8_t& dummy_data::operator[](int64_t) const { return dummy_; }

data dummy_data::view(int64_t) const { return *this; }

data dummy_data::view(int64_t, int64_t) const { return *this; }

}  // namespace common
