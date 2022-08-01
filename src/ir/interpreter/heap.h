//
//  heap.h
//  Katara
//
//  Created by Arne Philipeit on 7/3/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_interpreter_heap_h
#define ir_interpreter_heap_h

#include <cstdint>
#include <memory>
#include <vector>

namespace ir_interpreter {

class Heap {
 public:
  Heap(bool sanitize) : sanitize_(sanitize) {}
  ~Heap();

  int64_t Malloc(int64_t size);
  void Free(int64_t address);

  template <typename T>
  T Load(int64_t address) {
    MemoryRange range{
        .address = address,
        .size = sizeof(T),
    };
    if (sanitize_) {
      Memory* memory = CheckExists(range);
      CheckWasInitialized(memory, range);
    }
    return *((T*)(address));
  }

  template <typename T>
  void Store(int64_t address, T value) {
    MemoryRange range{
        .address = address,
        .size = sizeof(T),
    };
    Memory* memory = nullptr;
    if (sanitize_) {
      memory = CheckExists(range);
    }
    *((T*)(address)) = value;
    if (sanitize_) {
      MarkAsInitialized(memory, range);
    }
  }

 private:
  struct MemoryRange {
    int64_t address;
    int64_t size;
  };
  struct Memory {
    Memory(MemoryRange r) : range(r), initialization(r.size, false) {}

    MemoryRange range;
    std::vector<bool> initialization;
  };

  static bool IsContained(int64_t address, MemoryRange container);
  static bool IsContained(MemoryRange contained, MemoryRange container);
  static bool Overlap(MemoryRange range_a, MemoryRange range_b);

  Memory* CheckExists(MemoryRange range);
  void CheckWasInitialized(Memory* memory, MemoryRange range);
  void CheckCanBeFreed(int64_t address);

  void MarkAsInitialized(Memory* memory, MemoryRange range);

  bool sanitize_;
  std::vector<std::unique_ptr<Memory>> allocated_;
  std::vector<MemoryRange> freed_;
};

}  // namespace ir_interpreter

#endif /* ir_interpreter_heap_h */
