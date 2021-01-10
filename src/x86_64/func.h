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

#include "common/data.h"
#include "x86_64/block.h"
#include "x86_64/mc/linker.h"

namespace x86_64 {

class Prog;
class ProgBuilder;

class Func {
 public:
  ~Func();

  Prog* prog() const;
  int64_t func_id() const;
  std::string name() const;
  const std::vector<Block*> blocks() const;

  FuncRef GetFuncRef() const;

  int64_t Encode(Linker* linker, common::data code) const;
  std::string ToString() const;

 private:
  Func();

  Prog* prog_;
  int64_t func_id_;
  std::string name_;
  std::vector<Block*> blocks_;

  friend class FuncBuilder;
};

class FuncBuilder {
 public:
  ~FuncBuilder();

  BlockBuilder AddBlock();

  Func* func() const;

 private:
  FuncBuilder(Prog* prog, int64_t func_id, std::string func_name, int64_t& block_count);

  Func* func_;
  int64_t& block_count_;

  friend class ProgBuilder;
};

}  // namespace x86_64

#endif /* x86_func_h */
