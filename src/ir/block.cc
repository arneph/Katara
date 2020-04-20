//
//  block.cc
//  Katara
//
//  Created by Arne Philipeit on 12/23/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "block.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <sstream>

#include "ir/func.h"

namespace ir {

Block::Block(int64_t number, Func *func)
    : number_(number), func_(func) {}

Block::~Block() {
    for (ir::Instr *instr : instrs_) {
        delete instr;
    }
}

int64_t Block::number() const {
    return number_;
}

std::string Block::name() const {
    return name_;
}

void Block::set_name(std::string name) {
    name_ = name;
}

Func * Block::func() const {
    return func_;
}

std::string Block::ReferenceString() const {
    std::string title = "{" + std::to_string(number_) + "}";
    if (!name_.empty()) title += " " + name_;
    return title;
}

BlockValue Block::block_value() const {
    return BlockValue(number_);
}

const std::vector<Instr *>& Block::instrs() const {
    return instrs_;
}

void Block::for_each_phi_instr(
    std::function<void (PhiInstr *)> f) {
    for (size_t i = 0; i < instrs_.size(); i++) {
        Instr *instr = instrs_.at(i);
        PhiInstr *phi_instr = dynamic_cast<PhiInstr *>(instr);
        
        if (phi_instr == nullptr) return;
        
        f(phi_instr);
    }
}

void Block::for_each_phi_instr_reverse(
    std::function<void (PhiInstr *)> f) {
    size_t phi_count = 0;
    for (phi_count = 0; phi_count < instrs_.size(); phi_count++) {
        Instr *instr = instrs_.at(phi_count);
        PhiInstr *phi_instr = dynamic_cast<PhiInstr *>(instr);
        if (phi_instr == nullptr) break;
    }
    
    for (int64_t i = phi_count - 1; i >= 0; i--) {
        Instr *instr = instrs_.at(i);
        PhiInstr *phi_instr = static_cast<PhiInstr *>(instr);
        
        f(phi_instr);
    }
}

void Block::for_each_non_phi_instr(
    std::function<void (Instr *)> f) {
    for (size_t i = 0; i < instrs_.size(); i++) {
        Instr *instr = instrs_.at(i);
        PhiInstr *phi_instr = dynamic_cast<PhiInstr *>(instr);
        
        if (phi_instr != nullptr) continue;
        
        f(instr);
    }
}

void Block::for_each_non_phi_instr_reverse(
    std::function<void (Instr *)> f) {
    for (int64_t i = instrs_.size() - 1; i >= 0; i--) {
        Instr *instr = instrs_.at(i);
        PhiInstr *phi_instr = dynamic_cast<PhiInstr *>(instr);
        
        if (phi_instr != nullptr) return;
        
        f(instr);
    }
}

void Block::AddInstr(Instr *instr) {
    InsertInstr(instrs_.size(), instr);
}

void Block::InsertInstr(size_t index, Instr *instr) {
    if (index < 0 || index > instrs_.size())
        throw "insertion index out of bounds";
    if (instr == nullptr)
        throw "tried to add nullptr instruction to block";
    if (instr->number_ != -1)
        throw "tried to add instruction to block that is "
              "already used elsewhere";
    
    instr->number_ = func_->instr_count_++;
    instr->block_ = this;
    instrs_.insert(instrs_.begin() + index, instr);
    func_->instr_lookup_.insert({instr->number(), instr});
}

void Block::RemoveInstr(int64_t inum) {
    auto it = func_->instr_lookup_.find(inum);
    if (it == func_->instr_lookup_.end())
        throw "tried to remove instruction not owned by function";
    
    RemoveInstr(it->second);
}

void Block::RemoveInstr(Instr *instr) {
    if (instr == nullptr)
        throw "tried to remove nullptr instruction";
    if (instr->block_ != this)
        throw "tried to remove instruction not owned by block";
    
    auto it = std::find(instrs_.begin(), instrs_.end(), instr);
    if (it == instrs_.end())
        throw "tried to remove instruction not owned by block";
    
    instrs_.erase(it);
    func_->instr_lookup_.erase(instr->number());
    instr->number_ = -1;
    instr->block_ = nullptr;
}

const std::unordered_set<Block *>& Block::parents() const {
    return parents_;
}

const std::unordered_set<Block *>& Block::children() const {
    return children_;
}

bool Block::HasBranchingParent() const {
    if (parents_.size() != 1) return false;
    Block *parent = *parents_.begin();
    
    return parent->children().size() > 1;
}

Block * Block::BranchingParent() const {
    if (!HasBranchingParent())
        throw "block has no branching parent";
    
    return *parents_.begin();
}

bool Block::HasMergingChild() const {
    if (children_.size() != 1) return false;
    Block *child = *children_.begin();
    
    return child->parents().size() > 1;
}

Block * Block::MergingChild() const {
    if (!HasMergingChild())
        throw "block has no merging child";
    
    return *children_.begin();
}

Block * Block::dominator() const {
    if (!func_->dom_tree_ok_) {
        func_->UpdateDominatorTree();
    }
    
    return dominator_;
}

const std::unordered_set<Block *>& Block::dominees() const {
    if (!func_->dom_tree_ok_) {
        func_->UpdateDominatorTree();
    }
    
    return dominees_;
}

std::string Block::ToString() const {
    std::stringstream ss;
    ss << ReferenceString();
    const int instr_num_w = std::max(2.0, log10(func_->block_count_-1));
    for (Instr *instr : instrs_) {
        ss << "\n";
        ss << std::setfill('0') << std::setw(instr_num_w);
        ss << instr->number();
        ss << " ";
        ss << instr->ToString();
    }
    return ss.str();
}

vcg::Node Block::ToVCGNode() const {
    std::stringstream ss;
    const int instr_num_w = std::max(2.0, log10(func_->block_count_-1));
    for (size_t i = 0; i < instrs_.size(); i++) {
        if (i > 0) ss << "\n";
        ss << std::setfill('0') << std::setw(instr_num_w);
        ss << instrs_.at(i)->number();
        ss << " ";
        ss << instrs_.at(i)->ToString();
    }
    
    return vcg::Node(number_, ReferenceString(), ss.str());
}

}
