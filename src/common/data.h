//
//  data.h
//  Katara
//
//  Created by Arne Philipeit on 12/11/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef common_data_h
#define common_data_h

#include <memory>

namespace common {

class data {
 public:
  data(uint8_t* base, int64_t size);
  ~data();

  uint8_t* base() const;
  int64_t size() const;

  virtual uint8_t& operator[](int64_t index);
  virtual const uint8_t& operator[](int64_t index) const;

  virtual data view(int64_t start_index) const;
  virtual data view(int64_t start_index, int64_t end_index) const;

 private:
  uint8_t* const base_;
  int64_t const size_;
};

class dummy_data final : public data {
 public:
  dummy_data();
  ~dummy_data();

  uint8_t& operator[](int64_t index) override;
  const uint8_t& operator[](int64_t index) const override;

  data view(int64_t start_index) const override;
  data view(int64_t start_index, int64_t end_index = 0) const override;

 private:
  uint8_t dummy_;
};

}  // namespace common

#endif /* common_data_h */
