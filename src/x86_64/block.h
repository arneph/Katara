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

#include "src/common/data_view.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/machine_code/linker.h"

namespace x86_64 {

class Func;

class Block {
 public:
  Func* func() const { return func_; }
  int64_t block_id() const { return block_id_; }
  const std::vector<std::unique_ptr<Instr>>& instrs() const { return instrs_; }

  BlockRef GetBlockRef() const { return BlockRef(block_id_); }

  int64_t Encode(Linker& linker, common::DataView code) const;
  std::string ToString() const;

 private:
  Block(Func* func, int64_t block_id) : func_(func), block_id_(block_id) {}

  Func* func_;
  int64_t block_id_;
  std::vector<std::unique_ptr<Instr>> instrs_;

  friend class FuncBuilder;
  friend class BlockBuilder;
};

class BlockBuilder {
 public:
  template <class T, class... Args>
  void AddInstr(Args&&... args) {
    block_->instrs_.push_back(std::make_unique<T>(args...));
  }

  Block* block() const { return block_; }

 private:
  BlockBuilder(Block* block) : block_(block) {}

  Block* block_;

  friend class FuncBuilder;
};

}  // namespace x86_64

#endif /* x86_64_block_h */
