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

#include "src/common/data.h"
#include "src/x86_64/block.h"
#include "src/x86_64/machine_code/linker.h"

namespace x86_64 {

class Program;

class Func {
 public:
  Program* program() const { return program_; }
  int64_t func_id() const { return func_id_; }
  std::string name() const { return name_; }
  const std::vector<std::unique_ptr<Block>>& blocks() const { return blocks_; }

  FuncRef GetFuncRef() const { return FuncRef(func_id_); }

  int64_t Encode(Linker& linker, common::data code) const;
  std::string ToString() const;

 private:
  Func(Program* program, int64_t func_id, std::string name)
      : program_(program), func_id_(func_id), name_(name) {}

  Program* program_;
  int64_t func_id_;
  std::string name_;
  std::vector<std::unique_ptr<Block>> blocks_;

  friend class ProgramBuilder;
  friend class FuncBuilder;
};

class FuncBuilder {
 public:
  BlockBuilder AddBlock();

  Func* func() const { return func_; }

 private:
  FuncBuilder(Func* func, int64_t& block_count) : func_(func), block_count_(block_count) {}

  Func* func_;
  int64_t& block_count_;

  friend class ProgramBuilder;
};

}  // namespace x86_64

#endif /* x86_func_h */
