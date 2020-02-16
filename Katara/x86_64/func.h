//
//  func.h
//  Katara
//
//  Created by Arne Philipeit on 11/27/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_func_h
#define x86_64_func_h

#include <memory>
#include <string>
#include <vector>

#include "common/data.h"
#include "x86_64/mc/linker.h"
#include "x86_64/block.h"

namespace x86_64 {

class Prog;
class ProgBuilder;

class Func {
public:
    ~Func();
    
    std::weak_ptr<Prog> prog() const;
    int64_t func_id() const;
    std::string name() const;
    const std::vector<std::shared_ptr<Block>> blocks() const;
    
    FuncRef GetFuncRef() const;
    
    int64_t Encode(Linker *linker,
                   common::data code) const;
    std::string ToString() const;
    
private:
    Func();
    
    std::weak_ptr<Prog> prog_;
    int64_t func_id_;
    std::string name_;
    std::vector<std::shared_ptr<Block>> blocks_;
    
    friend class FuncBuilder;
};

class FuncBuilder {
public:
    ~FuncBuilder();
    
    BlockBuilder AddBlock();
    
    std::shared_ptr<Func> func() const;
    
private:
    FuncBuilder(std::shared_ptr<Prog> prog,
                int64_t func_id,
                std::string func_name,
                int64_t& block_count);
    
    std::shared_ptr<Func> func_;
    int64_t& block_count_;
    
    friend class ProgBuilder;
};

}

#endif /* x86_func_h */
