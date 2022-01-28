//
//  data_view.h
//  Katara
//
//  Created by Arne Philipeit on 12/11/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef common_data_view_h
#define common_data_view_h

#include <cstdint>

namespace common {

class DataView {
 public:
  DataView(uint8_t* base, int64_t size);

  uint8_t* base() const { return base_; }
  int64_t size() const { return size_; }

  const uint8_t& operator[](int64_t index) const;
  uint8_t& operator[](int64_t index);

  const DataView SubView(int64_t start_index) const;
  const DataView SubView(int64_t start_index, int64_t end_index) const;
  DataView SubView(int64_t start_index);
  DataView SubView(int64_t start_index, int64_t end_index);

 private:
  void CheckIndex(int64_t index) const;
  void CheckSubViewIndices(int64_t start_index, int64_t end_index) const;

  uint8_t* const base_;
  int64_t const size_;
};

}  // namespace common

#endif /* common_data_h */
