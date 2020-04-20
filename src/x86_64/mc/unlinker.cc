//
//  unlinker.cc
//  Katara
//
//  Created by Arne Philipeit on 12/20/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "unlinker.h"

namespace x86_64 {

Unlinker::Unlinker() {}
Unlinker::~Unlinker() {}

const std::unordered_map<uint8_t *, FuncRef>
    Unlinker::func_refs() const {
    return func_refs_;
}

const std::unordered_map<uint8_t *, BlockRef>
    Unlinker::block_refs() const {
    return block_refs_;
}

FuncRef Unlinker::GetFuncRef(uint8_t *func_addr) {
    if (func_refs_.count(func_addr) == 0) {
        int64_t func_id = func_refs_.size();
        FuncRef ref = FuncRef(func_id);
        func_refs_.insert({func_addr, ref});
        
        return ref;
    } else {
        return func_refs_.at(func_addr);
    }
}

BlockRef Unlinker::GetBlockRef(uint8_t *block_addr) {
    if (block_refs_.count(block_addr) == 0) {
        int64_t block_id = block_refs_.size();
        BlockRef ref = BlockRef(block_id);
        block_refs_.insert({block_addr, ref});
        
        return ref;
    } else {
        return block_refs_.at(block_addr);
    }
}

}
