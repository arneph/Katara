//
//  unlinker.h
//  Katara
//
//  Created by Arne Philipeit on 12/20/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_unlinker_h
#define x86_64_unlinker_h

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/common/data.h"
#include "src/x86_64/ops.h"

namespace x86_64 {

class Unlinker {
  Unlinker();
  ~Unlinker();

  const std::unordered_map<uint8_t*, FuncRef> func_refs() const;
  const std::unordered_map<uint8_t*, BlockRef> block_refs() const;

  FuncRef GetFuncRef(uint8_t* func_addr);
  BlockRef GetBlockRef(uint8_t* block_addr);

 private:
  std::unordered_map<uint8_t*, FuncRef> func_refs_;
  std::unordered_map<uint8_t*, BlockRef> block_refs_;
};

}  // namespace x86_64

#endif /* x86_64_unlinker_h */
