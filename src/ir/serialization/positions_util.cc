//
//  positions_util.cc
//  Katara
//
//  Created by Arne Philipeit on 10/7/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#include "positions_util.h"

#include "src/ir/representation/instrs.h"

namespace ir_serialization {

using ::common::positions::range_t;

range_t GetJumpInstrDestinationRange(const InstrPositions& jump_instr_positions) {
  return jump_instr_positions.used_value_ranges().at(ir::JumpInstr::kDestinationIndex);
}

range_t GetJumpCondInstrDestinationTrueRange(const InstrPositions& jump_cond_instr_positions) {
  return jump_cond_instr_positions.used_value_ranges().at(ir::JumpCondInstr::kDestinationTrueIndex);
}

range_t GetJumpCondInstrDestinationFalseRange(const InstrPositions& jump_cond_instr_positions) {
  return jump_cond_instr_positions.used_value_ranges().at(
      ir::JumpCondInstr::kDestinationFalseIndex);
}

}  // namespace ir_serialization
