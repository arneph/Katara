//
//  heap.cc
//  Katara
//
//  Created by Arne Philipeit on 7/3/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "heap.h"

#include "src/common/logging/logging.h"

namespace ir_interpreter {

Heap::~Heap() {
  if (sanitize_) {
    if (!allocated_.empty()) {
      common::fail("not all memory was freed");
    }
    for (MemoryRange range : freed_) {
      free((void*)(range.address));
    }
  }
}

int64_t Heap::Malloc(int64_t size) {
  if (sanitize_) {
    if (size <= 0) {
      common::fail("attempted malloc with non-positive size");
    }
  }
  int64_t address = int64_t(malloc(size));
  if (sanitize_) {
    allocated_.push_back(std::make_unique<Memory>(MemoryRange{.address = address, .size = size}));
  }
  return address;
}

void Heap::Free(int64_t address) {
  if (sanitize_) {
    CheckCanBeFreed(address);
  }
  if (!sanitize_) {
    free((void*)(address));
  }
  if (sanitize_) {
    auto it = std::find_if(
        allocated_.begin(), allocated_.end(),
        [address](std::unique_ptr<Memory>& memory) { return memory->range.address == address; });
    MemoryRange range = (*it)->range;
    allocated_.erase(it);
    freed_.push_back(range);
  }
}

bool Heap::IsContained(int64_t address, MemoryRange container) {
  int64_t container_begin = container.address;
  int64_t container_end = container.address + container.size;
  return container_begin <= address && address < container_end;
}

bool Heap::IsContained(MemoryRange contained, MemoryRange container) {
  int64_t container_begin = container.address;
  int64_t container_end = container.address + container.size;
  int64_t contained_begin = contained.address;
  int64_t contained_end = contained.address + contained.size;
  return container_begin <= contained_begin && contained_end <= container_end;
}

bool Heap::Overlap(MemoryRange range_a, MemoryRange range_b) {
  int64_t range_a_begin = range_a.address;
  int64_t range_a_end = range_a.address + range_a.size;
  int64_t range_b_begin = range_b.address;
  int64_t range_b_end = range_b.address + range_b.size;
  return (range_a_begin <= range_b_begin && range_a_end > range_b_begin) ||
         (range_b_begin <= range_a_begin && range_b_end > range_a_begin);
}

Heap::Memory* Heap::CheckExists(MemoryRange range) {
  for (auto& memory : allocated_) {
    if (IsContained(/*contained=*/range, /*container=*/memory->range)) {
      return memory.get();
    } else if (Overlap(range, memory->range)) {
      common::fail(
          "attempted to access memory range that only partially overlaps allocated memory");
    }
  }
  for (MemoryRange& freed_range : freed_) {
    if (Overlap(range, freed_range)) {
      common::fail("attempted to access memory range that was freed");
    }
  }
  common::fail("attempted to access memory range that doesn't exist");
}

void Heap::CheckWasInitialized(Memory* memory, MemoryRange range) {
  int64_t range_index_begin = range.address - memory->range.address;
  int64_t range_index_end = range_index_begin + range.size;
  for (int64_t i = range_index_begin; i < range_index_end; i++) {
    if (memory->initialization.at(i) == false) {
      common::fail("attempted to read uninitialized memory");
    }
  }
}

void Heap::CheckCanBeFreed(int64_t address) {
  for (auto& memory : allocated_) {
    if (memory->range.address == address) {
      return;
    } else if (IsContained(address, memory->range)) {
      common::fail("address to be freed does not point to start of allocated block");
    }
  }
  for (MemoryRange& range : freed_) {
    if (range.address == address) {
      common::fail("memory was already freed");
    }
  }
  common::fail("memory was never allocated");
}

void Heap::MarkAsInitialized(Memory* memory, MemoryRange range) {
  int64_t range_index_begin = range.address - memory->range.address;
  int64_t range_index_end = range_index_begin + range.size;
  for (int64_t i = range_index_begin; i < range_index_end; i++) {
    memory->initialization.at(i) = true;
  }
}

}  // namespace ir_interpreter
