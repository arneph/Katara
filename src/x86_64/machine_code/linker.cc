//
//  linker.cc
//  Katara
//
//  Created by Arne Philipeit on 11/27/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "linker.h"

namespace x86_64 {

void Linker::AddFuncAddr(int64_t func_id, uint8_t* func_addr) { func_addrs_[func_id] = func_addr; }

void Linker::AddBlockAddr(int64_t block_id, uint8_t* block_addr) {
  block_addrs_[block_id] = block_addr;
}

void Linker::AddFuncRef(const FuncRef& func_ref, common::DataView patch_data_view) {
  func_patches_.push_back(FuncPatch{func_ref, patch_data_view});
}

void Linker::AddBlockRef(const BlockRef& block_ref, common::DataView patch_data_view) {
  block_patches_.push_back(BlockPatch{block_ref, patch_data_view});
}

void Linker::ApplyPatches() const {
  for (auto func_patch : func_patches_) {
    FuncRef func_ref = func_patch.func_ref;
    common::DataView patch_data_view = func_patch.patch_data_view;
    uint8_t* dest_func_addr = func_addrs_.at(func_ref.func_id());
    int64_t offset = dest_func_addr - (patch_data_view.base() + 0x04);

    patch_data_view[0x00] = (offset >> 0) & 0x000000FF;
    patch_data_view[0x01] = (offset >> 8) & 0x000000FF;
    patch_data_view[0x02] = (offset >> 16) & 0x000000FF;
    patch_data_view[0x03] = (offset >> 24) & 0x000000FF;
  }

  for (auto block_patch : block_patches_) {
    BlockRef block_ref = block_patch.block_ref;
    common::DataView patch_data_view = block_patch.patch_data_view;
    uint8_t* dest_block_addr = block_addrs_.at(block_ref.block_id());
    int64_t offset = dest_block_addr - (patch_data_view.base() + 0x04);

    patch_data_view[0x00] = (offset >> 0) & 0x000000FF;
    patch_data_view[0x01] = (offset >> 8) & 0x000000FF;
    patch_data_view[0x02] = (offset >> 16) & 0x000000FF;
    patch_data_view[0x03] = (offset >> 24) & 0x000000FF;
  }
}

}  // namespace x86_64
