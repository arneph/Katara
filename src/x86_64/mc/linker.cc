//
//  linker.cc
//  Katara
//
//  Created by Arne Philipeit on 11/27/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "linker.h"

namespace x86_64 {

Linker::Linker() {}
Linker::~Linker() {}

void Linker::AddFuncAddr(int64_t func_id,
                         uint8_t *func_addr) {
    func_addrs_[func_id] = func_addr;
}

void Linker::AddBlockAddr(int64_t block_id,
                          uint8_t *block_addr) {
    block_addrs_[block_id] = block_addr;
}

void Linker::AddFuncRef(const FuncRef &func_ref,
                        common::data patch_data) {
    func_patches_.push_back(FuncPatch{func_ref, patch_data});
}

void Linker::AddBlockRef(const BlockRef &block_ref,
                         common::data patch_data) {
    block_patches_.push_back(BlockPatch{block_ref, patch_data});
}

void Linker::ApplyPatches() const {
    for (auto func_patch : func_patches_) {
        auto func_ref = func_patch.func_ref;
        auto patch_data = func_patch.patch_data;
        uint8_t *dest_func_addr = func_addrs_.at(func_ref.func_id());
        int64_t offset = dest_func_addr - (patch_data.base() + 0x04);
        
        patch_data[0x00] = (offset >>  0) & 0x000000FF;
        patch_data[0x01] = (offset >>  8) & 0x000000FF;
        patch_data[0x02] = (offset >> 16) & 0x000000FF;
        patch_data[0x03] = (offset >> 24) & 0x000000FF;
    }
    
    for (auto block_patch : block_patches_) {
        auto block_ref = block_patch.block_ref;
        auto patch_data = block_patch.patch_data;
        uint8_t *dest_block_addr = block_addrs_.at(block_ref.block_id());
        int64_t offset = dest_block_addr - (patch_data.base() + 0x04);
        
        patch_data[0x00] = (offset >>  0) & 0x000000FF;
        patch_data[0x01] = (offset >>  8) & 0x000000FF;
        patch_data[0x02] = (offset >> 16) & 0x000000FF;
        patch_data[0x03] = (offset >> 24) & 0x000000FF;
    }
}

}
