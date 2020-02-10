//
//  unlinker.cc
//  Katara
//
//  Created by Arne Philipeit on 12/20/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "unlinker.h"

namespace x64 {

Unlinker::Unlinker() {}
Unlinker::~Unlinker() {}

const std::unordered_map<uint8_t *,
                         std::shared_ptr<FuncRef>>
    Unlinker::func_refs() const {
    return func_refs_;
}

const std::unordered_map<uint8_t *,
                         std::shared_ptr<BlockRef>>
    Unlinker::block_refs() const {
    return block_refs_;
}

std::shared_ptr<FuncRef>
    Unlinker::GetFuncRef(uint8_t *func_addr) {
    if (func_refs_.count(func_addr) == 0) {
        std::string name =
            "Func" + std::to_string(func_refs_.size());
        std::shared_ptr<FuncRef> ref
            = std::make_unique<FuncRef>(name);
        func_refs_[func_addr] = ref;
        
        return ref;
    } else {
        return func_refs_.at(func_addr);
    }
}

std::shared_ptr<BlockRef>
    Unlinker::GetBlockRef(uint8_t *block_addr) {
    if (block_refs_.count(block_addr) == 0) {
        int64_t block_id = block_refs_.size();
        std::shared_ptr<BlockRef> ref
            = std::make_unique<BlockRef>(block_id);
        block_refs_[block_addr] = ref;
        
        return ref;
    } else {
        return block_refs_.at(block_addr);
    }
}

}
