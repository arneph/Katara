//
//  memory.cc
//  Katara
//
//  Created by Arne Philipeit on 1/22/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "memory.h"

#include <sys/mman.h>

namespace common {
namespace {

bool ValidatePermissions(Memory::Permissions permissions) {
  if (permissions == Memory::kNone || permissions == Memory::kExecute) {
    return true;
  }
  if (permissions & Memory::kExecute) {
    // Execute can not be mixed with other permissions.
    return false;
  }
  if (permissions & ~(Memory::kRead | Memory::kWrite)) {
    // Unknown permissions, expected read and/or write.
    return false;
  }
  return true;
}

}  // namespace

Memory::Memory(int64_t size, Permissions permissions) {
  if (size < 0) {
    fail("Memory constructed with negative size");
  } else if (!ValidatePermissions(permissions)) {
    fail("Invalid permissions");
  } else if (size == 0) {
    base_ = nullptr;
    size_ = 0;
    permissions_ = permissions;
  } else {
    void* base = mmap(/*addr=*/NULL, size, permissions, /*flags=*/MAP_PRIVATE | MAP_ANONYMOUS,
                      /*fildes=*/0, /*offset=*/0);
    if (base == (void*)-1) {
      fail("mmap failed");
    }
    base_ = (uint8_t*)base;
    size_ = size;
    permissions_ = permissions;
  }
}

Memory::Memory(Memory&& other)
    : base_(std::exchange(other.base_, nullptr)),
      size_(std::exchange(other.size_, 0)),
      permissions_(std::exchange(other.permissions_, kNone)) {}

Memory& Memory::operator=(Memory&& other) {
  std::swap(base_, other.base_);
  std::swap(size_, other.size_);
  std::swap(permissions_, other.permissions_);
  other.Free();
  return *this;
}

Memory::~Memory() { Free(); }

void Memory::ChangePermissions(Permissions new_permissions) {
  if (!ValidatePermissions(new_permissions)) {
    fail("Invalid permissions");
  }
  if (base_ == nullptr || size_ == 0) {
    permissions_ = new_permissions;
    return;
  }
  if (mprotect(base_, size_, new_permissions) != 0) {
    fail("mprotect failed");
  }
  permissions_ = new_permissions;
}

void Memory::Free() {
  if (base_ == nullptr || size_ == 0) {
    base_ = nullptr;
    size_ = 0;
    permissions_ = kNone;
    return;
  }
  if (munmap(base_, size_) != 0) {
    fail("munmap failed");
  }
  base_ = nullptr;
  size_ = 0;
  permissions_ = kNone;
}

}  // namespace common
