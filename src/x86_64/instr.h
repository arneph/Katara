//
//  instr.h
//  Katara
//
//  Created by Arne Philipeit on 11/27/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_instr_h
#define x86_64_instr_h

#include <memory>
#include <string>

#include "common/data.h"
#include "x86_64/mc/linker.h"
#include "x86_64/ops.h"

namespace x86_64 {

// TODO:
// SAL/SAR/SHL/SHR      (shift)
// RCL/RCR/ROL/ROR      (rotate)

class Instr {
 public:
  virtual ~Instr() {}

  virtual int8_t Encode(Linker* linker, common::data code) const = 0;
  virtual std::string ToString() const = 0;
};

}  // namespace x86_64

#endif /* x86_64_instr_h */
