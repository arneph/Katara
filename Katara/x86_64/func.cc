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

std::string Func::name() const {
    return name_;
}

const std::vector<std::shared_ptr<Block>> Func::blocks() const {
    return blocks_;
}

std::unique_ptr<FuncRef> Func::GetFuncRef() const {
    return std::make_unique<FuncRef>(name_);
}

int64_t Func::Encode(Linker *linker,
                     common::data code) const {
    linker->AddFuncAddr(name_, code.base());
    
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

FuncBuilder::FuncBuilder(std::shared_ptr<Prog> prog,
                         std::string func_name,
                         int64_t& block_count)
    : block_count_(block_count) {
    func_ = std::shared_ptr<Func>(new Func());
    func_->prog_ = prog;
    func_->name_ = func_name;
}

FuncBuilder::~FuncBuilder() {}

BlockBuilder FuncBuilder::AddBlock() {
    std::string block_name =
        "BB" + std::to_string(func_->blocks_.size());
    BlockBuilder block_builder(func_, block_count_++);
    
    func_->blocks_.push_back(block_builder.block());
    return block_builder;
}

std::shared_ptr<Func> FuncBuilder::func() const {
    return func_;
}

}
