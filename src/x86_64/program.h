//
//  program.h
//  Katara
//
//  Created by Arne Philipeit on 12/7/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_program_h
#define x86_64_program_h

#include <memory>
#include <string>
#include <vector>

#include "src/common/data.h"
#include "src/x86_64/func.h"
#include "src/x86_64/machine_code/linker.h"

namespace x86_64 {

class Program {
 public:
  const std::vector<std::unique_ptr<Func>>& funcs() const { return funcs_; }

  int64_t Encode(Linker& linker, common::data code) const;
  std::string ToString() const;

 private:
  Program() {}

  std::vector<std::unique_ptr<Func>> funcs_;

  friend class ProgramBuilder;
};

class ProgramBuilder {
 public:
  ProgramBuilder() : program_(new Program()), func_count_(0), block_count_(0) {}

  FuncBuilder AddFunc(std::string func_name);

  Program* program() const { return program_.get(); }
  std::unique_ptr<Program> Build() { return std::move(program_); }

 private:
  std::unique_ptr<Program> program_;
  int64_t func_count_;
  int64_t block_count_;
};

}  // namespace x86_64

#endif /* x86_64_program_h */