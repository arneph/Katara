//
//  instr_cond.cc
//  Katara
//
//  Created by Arne Philipeit on 2/23/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "instr_cond.h"

namespace x86_64 {

std::string to_suffix_string(InstrCond cond) {
  switch (cond) {
    case InstrCond::kOverflow:
      return "o";
    case InstrCond::kNoOverflow:
      return "no";
    case InstrCond::kSign:
      return "s";
    case InstrCond::kNoSign:
      return "ns";
    case InstrCond::kParityEven:
      return "pe";
    case InstrCond::kParityOdd:
      return "po";
    case InstrCond::kEqual:
      return "e";
    case InstrCond::kNotEqual:
      return "ne";
    case InstrCond::kAbove:
      return "a";
    case InstrCond::kAboveOrEqual:
      return "ae";
    case InstrCond::kBelowOrEqual:
      return "be";
    case InstrCond::kBelow:
      return "b";
    case InstrCond::kGreater:
      return "g";
    case InstrCond::kGreaterOrEqual:
      return "ge";
    case InstrCond::kLessOrEqual:
      return "le";
    case InstrCond::kLess:
      return "l";
  }
}

}  // namespace x86_64
