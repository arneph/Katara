//
//  instr_cond.h
//  Katara
//
//  Created by Arne Philipeit on 2/23/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_instr_cond_h
#define x86_64_instr_cond_h

#include <string>

namespace x86_64 {

typedef enum : int8_t {
  kOverflow = 0x00,
  kNoOverflow = 0x01,
  kCarry = 0x02,
  kNoCarry = 0x03,
  kZero = 0x04,
  kNoZero = 0x05,
  kCarryZero = 0x06,
  kNoCarryZero = 0x07,
  kSign = 0x08,
  kNoSign = 0x09,
  kParity = 0x0a,
  kNoParity = 0x0b,
  kParityEven = kParity,
  kParityOdd = kNoParity,

  // all integers:
  kEqual = kZero,
  kNotEqual = kNoZero,

  // unsigned integers:
  kAbove = kNoCarryZero,
  kAboveOrEqual = kNoCarry,
  kBelowOrEqual = kCarryZero,
  kBelow = kCarry,

  // signed integers:
  kGreater = 0x0f,
  kGreaterOrEqual = 0x0d,
  kLessOrEqual = 0x0e,
  kLess = 0x0c
} InstrCond;

extern std::string to_suffix_string(InstrCond cond);

}  // namespace x86_64

#endif /* x86_64_instr_cond_h */
