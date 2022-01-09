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

#include "src/common/data_view.h"
#include "src/x86_64/block.h"
#include "src/x86_64/func.h"
#include "src/x86_64/machine_code/linker.h"

namespace x86_64 {

class Program {
 public:
  const std::vector<std::unique_ptr<Func>>& defined_funcs() const { return defined_funcs_; }
  const std::unordered_map<std::string, int64_t>& declared_funcs() const { return declared_funcs_; }

  func_num_t DeclareFunc(std::string func_name);
  Func* DefineFunc(std::string func_name);
  Func* DefinedFuncWithNumber(func_num_t number) const;
  Func* DefinedFuncWithName(std::string name) const;

  int64_t block_count() const { return block_count_; }

  int64_t Encode(Linker& linker, common::DataView code) const;
  std::string ToString() const;

 private:
  int64_t block_count_ = 0;
  std::vector<std::unique_ptr<Func>> defined_funcs_;
  std::unordered_map<std::string, int64_t> declared_funcs_;

  friend Block* Func::AddBlock();
};

}  // namespace x86_64

#endif /* x86_64_program_h */
