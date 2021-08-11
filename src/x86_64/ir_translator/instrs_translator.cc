//
//  instrs_translator.cc
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "instrs_translator.h"

#include "src/common/logging.h"
#include "src/x86_64/ir_translator/instr_translators/arithmetic_logic_instrs_translator.h"
#include "src/x86_64/ir_translator/instr_translators/control_flow_instrs_translator.h"
#include "src/x86_64/ir_translator/instr_translators/data_instrs_translator.h"

namespace ir_to_x86_64_translator {

void TranslateInstr(ir::Instr* ir_instr, BlockContext& ctx) {
  switch (ir_instr->instr_kind()) {
    case ir::InstrKind::kMov:
      TranslateMovInstr(static_cast<ir::MovInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kBoolNot:
      TranslateBoolNotInstr(static_cast<ir::BoolNotInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kBoolBinary:
      TranslateBoolBinaryInstr(static_cast<ir::BoolBinaryInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kIntUnary:
      TranslateIntUnaryInstr(static_cast<ir::IntUnaryInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kIntCompare:
      TranslateIntCompareInstr(static_cast<ir::IntCompareInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kIntBinary:
      TranslateIntBinaryInstr(static_cast<ir::IntBinaryInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kIntShift:
      TranslateIntShiftInstr(static_cast<ir::IntShiftInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kPointerOffset:
      TranslatePointerOffsetInstr(static_cast<ir::PointerOffsetInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kNilTest:
      TranslateNilTestInstr(static_cast<ir::NilTestInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kMalloc:
      TranslateMallocInstr(static_cast<ir::MallocInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kLoad:
      TranslateLoadInstr(static_cast<ir::LoadInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kStore:
      TranslateStoreInstr(static_cast<ir::StoreInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kFree:
      TranslateFreeInstr(static_cast<ir::FreeInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kJump:
      TranslateJumpInstr(static_cast<ir::JumpInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kJumpCond:
      TranslateJumpCondInstr(static_cast<ir::JumpCondInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kCall:
      TranslateCallInstr(static_cast<ir::CallInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kReturn:
      TranslateReturnInstr(static_cast<ir::ReturnInstr*>(ir_instr), ctx);
      break;
    case ir::InstrKind::kLangPanic:
      // TODO: add lowering pass
      break;
    default:
      common::fail("unexpected instr: " + ir_instr->ToString());
  }
}

}  // namespace ir_to_x86_64_translator
