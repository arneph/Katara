//
//  positions_util.h
//  Katara
//
//  Created by Arne Philipeit on 10/7/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_positions_util_h
#define ir_serialization_positions_util_h

#include "src/common/positions/positions.h"
#include "src/ir/serialization/positions.h"

namespace ir_serialization {

common::positions::range_t GetJumpInstrDestinationRange(const InstrPositions& jump_instr_positions);

common::positions::range_t GetJumpCondInstrDestinationTrueRange(
    const InstrPositions& jump_cond_instr_positions);
common::positions::range_t GetJumpCondInstrDestinationFalseRange(
    const InstrPositions& jump_cond_instr_positions);

}  // namespace ir_serialization

#endif /* ir_serialization_positions_util_h */
