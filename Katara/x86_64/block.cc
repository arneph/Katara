//
//  block.cc
//  Katara
//
//  Created by Arne Philipeit on 11/27/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "block.h"

#include <sstream>

#include "func.h"

namespace x86_64 {

Block::Block() {}
Block::~Block() {}

std::weak_ptr<Func> Block::func() const {
    return func_;
}

int64_t Block::block_id() const {
    return block_id_;
}

const std::vector<std::unique_ptr<Instr>>& Block::instrs() const {
    return instrs_;
}

BlockRef Block::GetBlockRef() const {
    return BlockRef(block_id_);
}

int64_t Block::Encode(Linker *linker,
                      common::data code) const {
    linker->AddBlockAddr(block_id_, code.base());
    
    int64_t c = 0;
    for (auto& instr : instrs_) {
        int8_t r = instr->Encode(linker, code.view(c));
        if (r == -1) return -1;
        c += r;
    }
    return c;
}

std::string Block::ToString() const {
    std::stringstream ss;
    ss << "BB" << std::to_string(block_id_) << ":\n";
    for (size_t i = 0; i < instrs_.size(); i++) {
        ss << "\t" << instrs_[i]->ToString();
        if (i < instrs_.size() - 1) ss << "\n";
    }
    return ss.str();
}

BlockBuilder::BlockBuilder(std::shared_ptr<Func> func,
                           int64_t block_id) {
    block_ = std::shared_ptr<Block>(new Block());
    block_->func_ = func;
    block_->block_id_ = block_id;
}

BlockBuilder::~BlockBuilder() {}

void BlockBuilder::AddInstr(std::unique_ptr<Instr> instr) {
    block_->instrs_.push_back(std::move(instr));
}

std::shared_ptr<Block> BlockBuilder::block() const {
    return block_;
}

}
