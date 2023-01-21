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

#include "src/common/data/data_view.h"
#include "src/x86_64/block.h"
#include "src/x86_64/machine_code/linker.h"

namespace x86_64 {

class Program;
typedef int64_t func_num_t;

class Func {
 public:
  Program* program() const { return program_; }

  func_num_t func_num() const { return func_num_; }
  std::string name() const { return name_; }
  FuncRef GetFuncRef() const { return FuncRef(func_num_); }

  const std::vector<std::unique_ptr<Block>>& blocks() const { return blocks_; }

  Block* AddBlock();

  int64_t Encode(Linker& linker, common::data::DataView code) const;
  std::string ToString() const;

 private:
  Func(Program* program, func_num_t func_num, std::string name)
      : program_(program), func_num_(func_num), name_(name) {}

  Program* program_;
  func_num_t func_num_;
  std::string name_;
  std::vector<std::unique_ptr<Block>> blocks_;

  friend Program;
};

}  // namespace x86_64

#endif /* x86_func_h */
