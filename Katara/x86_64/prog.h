//
//  prog.h
//  Katara
//
//  Created by Arne Philipeit on 12/7/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_prog_h
#define x86_64_prog_h

#include <memory>
#include <string>
#include <vector>

#include "common/data.h"
#include "x86_64/mc/linker.h"
#include "x86_64/func.h"

namespace x86_64 {

class Prog {
public:
    ~Prog();
    
    const std::vector<std::shared_ptr<Func>> funcs() const;
    
    int64_t Encode(Linker *linker,
                   common::data code) const;
    std::string ToString() const;
    
private:
    Prog();
    
    std::vector<std::shared_ptr<Func>> funcs_;
    
    friend class ProgBuilder;
};

class ProgBuilder {
public:
    ProgBuilder();
    ~ProgBuilder();
    
    FuncBuilder AddFunc(std::string func_name);
    
    std::shared_ptr<Prog> prog() const;
    
private:
    std::shared_ptr<Prog> prog_;
    int64_t func_count_;
    int64_t block_count_;
};

}

#endif /* x86_prog_h */
