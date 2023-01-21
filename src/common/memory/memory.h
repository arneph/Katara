//
//  memory.h
//  Katara
//
//  Created by Arne Philipeit on 1/22/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef common_memory_hpp
#define common_memory_hpp

#include <sys/mman.h>

#include <cstdint>

#include "src/common/data/data_view.h"
#include "src/common/logging/logging.h"

namespace common::memory {

enum Permissions : int8_t {
  kNone = PROT_NONE,
  kRead = PROT_READ,
  kWrite = PROT_WRITE,
  kExecute = PROT_EXEC,
};

constexpr int64_t kPageSize = 1 << 12;

class Memory {
 public:
  Memory() : base_(nullptr), size_(0), permissions_(kNone) {}
  Memory(int64_t size, Permissions permissions);
  Memory(Memory&&);
  Memory(Memory&) = delete;
  Memory& operator=(Memory&&);
  Memory& operator=(Memory&) = delete;
  ~Memory();

  data::DataView data() const { return data::DataView(base_, size_); }
  Permissions permissions() const { return permissions_; }
  void ChangePermissions(Permissions new_permissions);
  void Free();

 private:
  uint8_t* base_;
  int64_t size_;
  Permissions permissions_;
};

}  // namespace common::memory

#endif /* common_memory_hpp */
