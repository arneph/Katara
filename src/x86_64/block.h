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

#include "src/common/data_view/data_view.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/machine_code/linker.h"

namespace x86_64 {

class Program;
class Func;
typedef int64_t block_num_t;

class Block {
 public:
  Program* program() const;
  Func* func() const { return func_; }

  block_num_t block_num() const { return block_id_; }
  BlockRef GetBlockRef() const { return BlockRef(block_id_); }

  const std::vector<std::unique_ptr<Instr>>& instrs() const { return instrs_; }

  template <class T, class... Args>
  void AddInstr(Args&&... args) {
    instrs_.push_back(std::make_unique<T>(args...));
  }

  template <class T, class... Args>
  std::vector<std::unique_ptr<Instr>>::const_iterator InsertInstr(
      std::vector<std::unique_ptr<Instr>>::const_iterator it, Args&&... args) {
    return instrs_.insert(it, std::make_unique<T>(args...));
  }

  int64_t Encode(Linker& linker, common::DataView code) const;
  std::string ToString() const;

 private:
  Block(Func* func, block_num_t block_id) : func_(func), block_id_(block_id) {}

  Func* func_;
  block_num_t block_id_;
  std::vector<std::unique_ptr<Instr>> instrs_;

  friend Func;
};

}  // namespace x86_64

#endif /* x86_64_block_h */
