//
//  prog.cc
//  Katara
//
//  Created by Arne Philipeit on 12/25/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "prog.h"

#include <sstream>

namespace ir {

Prog::Prog() {
    func_count_ = 0;
    
    entry_func_ = nullptr;
}

Prog::~Prog() {
    for (Func *func : funcs_) {
        delete func;
    }
}

const std::unordered_set<Func *>& Prog::funcs() const {
    return funcs_;
}

Func * Prog::entry_func() const {
    return entry_func_;
}

void Prog::set_entry_func(Func *func) {
    if (func == entry_func_) return;
    if (func != nullptr && func->prog_ != this)
        throw "tried to set entry func to func not owned by program";
    
    entry_func_ = func;
}

bool Prog::HasFunc(int64_t num) const {
    return func_lookup_.count(num) > 0;
}

Func * Prog::GetFunc(int64_t fnum) const {
    auto it = func_lookup_.find(fnum);
    if (it == func_lookup_.end()) {
        return nullptr;
    }
    return it->second;
}

Func * Prog::AddFunc(int64_t fnum) {
    if (fnum < 0) {
        fnum = func_count_++;
    } else {
        if (func_lookup_.count(fnum) != 0)
            throw "tried to add function with used function number";
        
        func_count_ = std::max(func_count_, fnum + 1);
    }
    Func *func = new Func(fnum, this);
    
    funcs_.insert(func);
    func_lookup_.insert({func->number_, func});
    
    return func;
}

void Prog::RemoveFunc(int64_t fnum) {
    auto it = func_lookup_.find(fnum);
    if (it == func_lookup_.end())
        throw "tried to remove func not owned by program";
    
    RemoveFunc(it->second);
}

void Prog::RemoveFunc(Func *func) {
    if (func == nullptr)
        throw "tried to remove nullptr func";
    if (func->prog_ != this)
        throw "tried to remove func not owned by program";
    if (entry_func_ == func)
        entry_func_ = nullptr;
    
    funcs_.erase(func);
    func_lookup_.erase(func->number_);
    
    delete func;
}

std::string Prog::ToString() const {
    std::vector<int64_t> fnums;
    fnums.reserve(func_lookup_.size());
    
    for (auto it : func_lookup_)
        fnums.push_back(it.first);
    
    std::sort(fnums.begin(), fnums.end());
    
    std::stringstream ss;
    bool first = true;
    
    for (int64_t fnum : fnums) {
        if (!first) ss << "\n\n";
        ss << func_lookup_.at(fnum)->ToString();
        first = false;
    }
    
    return ss.str();
}

}
