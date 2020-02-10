//
//  unlinker.h
//  Katara
//
//  Created by Arne Philipeit on 12/20/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef unlinker_h
#define unlinker_h

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/data.h"
#include "x86_64/ops.h"

namespace x64 {

class Unlinker {
    Unlinker();
    ~Unlinker();
    
    const std::unordered_map<uint8_t *,
                             std::shared_ptr<FuncRef>>
        func_refs() const;
    const std::unordered_map<uint8_t *,
                             std::shared_ptr<BlockRef>>
        block_refs() const;
    
    std::shared_ptr<FuncRef> GetFuncRef(uint8_t *func_addr);
    std::shared_ptr<BlockRef> GetBlockRef(uint8_t *block_addr);
    
private:
    std::unordered_map<uint8_t *,
                       std::shared_ptr<FuncRef>> func_refs_;
    std::unordered_map<uint8_t *,
                       std::shared_ptr<BlockRef>> block_refs_;
};

}

#endif /* unlinker_h */
