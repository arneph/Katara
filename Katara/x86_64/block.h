//
//  block.h
//  Katara
//
//  Created by Arne Philipeit on 11/27/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_block_h
#define x86_64_block_h

#include <memory>
#include <string>
#include <vector>

#include "common/data.h"
#include "x86_64/mc/linker.h"
#include "x86_64/instr.h"

namespace x86_64 {

class Func;
class FuncBuilder;

class Block {
public:
    ~Block();
    
    Func * func() const;
    int64_t block_id() const;
    const std::vector<Instr *>& instrs() const;
    
    BlockRef GetBlockRef() const;
    
    int64_t Encode(Linker *linker,
                   common::data code) const;
    std::string ToString() const;
    
private:
    Block();
    
    Func *func_;
    int64_t block_id_;
    std::vector<Instr *> instrs_;
    
    friend class BlockBuilder;
};

class BlockBuilder {
public:
    ~BlockBuilder();
    
    void AddInstr(Instr *instr);
    
    Block * block() const;
    
private:
    BlockBuilder(Func *func,
                 int64_t block_id);
    
    Block *block_;
    
    friend class FuncBuilder;
};

}

#endif /* x86_64_block_h */
