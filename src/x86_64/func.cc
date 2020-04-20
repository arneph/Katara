//
//  func.cc
//  Katara
//
//  Created by Arne Philipeit on 11/27/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "func.h"

#include <sstream>

namespace x86_64 {

Func::Func() {}
Func::~Func() {}

Prog * Func::prog() const {
    return prog_;
}

int64_t Func::func_id() const {
    return func_id_;
}

std::string Func::name() const {
    return name_;
}

const std::vector<Block *> Func::blocks() const {
    return blocks_;
}

FuncRef Func::GetFuncRef() const {
    return FuncRef(func_id_);
}

int64_t Func::Encode(Linker *linker,
                     common::data code) const {
    linker->AddFuncAddr(func_id_, code.base());
    
    int64_t c = 0;
    for (auto& block : blocks_) {
        int64_t r = block->Encode(linker, code.view(c));
        if (r == -1) return -1;
        c += r;
    }
    return c;
}

std::string Func::ToString() const {
    std::stringstream ss;
    ss << name_ << ":\n";
    for (size_t i = 0; i < blocks_.size(); i++) {
        ss << blocks_[i]->ToString();
        if (i < blocks_.size() - 1) ss << "\n";
    }
    return ss.str();
}

FuncBuilder::FuncBuilder(Prog *prog,
                         int64_t func_id,
                         std::string func_name,
                         int64_t& block_count)
    : block_count_(block_count) {
    func_ = new Func();
    func_->prog_ = prog;
    func_->name_ = func_name;
    func_->func_id_ = func_id;
}

FuncBuilder::~FuncBuilder() {}

BlockBuilder FuncBuilder::AddBlock() {
    std::string block_name =
        "BB" + std::to_string(func_->blocks_.size());
    BlockBuilder block_builder(func_, block_count_++);
    
    func_->blocks_.push_back(block_builder.block());
    return block_builder;
}

Func * FuncBuilder::func() const {
    return func_;
}

}
