//
//  instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "instrs.h"

#include <array>
#include <utility>

#include "src/common/logging/logging.h"

namespace ir {

bool Instr::IsControlFlowInstr() const {
  switch (instr_kind()) {
    case InstrKind::kJump:
    case InstrKind::kJumpCond:
    case InstrKind::kReturn:
    case InstrKind::kLangPanic:
      return true;
    default:
      return false;
  }
}

void Instr::SetPositions(common::pos_t start, common::pos_t end) {
  start_ = start;
  end_ = end;
}

void Instr::WriteRefString(std::ostream& os) const {
  bool wrote_first_defined_value = false;
  for (std::shared_ptr<Computed>& defined_value : DefinedValues()) {
    if (wrote_first_defined_value) {
      os << ", ";
    } else {
      wrote_first_defined_value = true;
    }
    defined_value->WriteRefStringWithType(os);
  }
  if (wrote_first_defined_value) {
    os << " = ";
  }
  os << OperationString();
  bool wrote_first_used_value = false;
  for (std::shared_ptr<Value>& used_value : UsedValues()) {
    if (wrote_first_used_value) {
      os << ", ";
    } else {
      os << " ";
      wrote_first_used_value = true;
    }
    if (used_value->kind() == Value::Kind::kConstant) {
      used_value->WriteRefStringWithType(os);
    } else {
      used_value->WriteRefString(os);
    }
  }
}

bool MovInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kMov) return false;
  auto that = static_cast<const MovInstr&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (!IsEqual(origin().get(), that.origin().get())) return false;
  return true;
}

std::shared_ptr<Value> PhiInstr::ValueInheritedFromBlock(block_num_t bnum) const {
  for (auto arg : args_) {
    if (arg->origin() == bnum) {
      return arg->value();
    }
  }
  common::fail("phi instr does not inherit from block");
}

std::vector<std::shared_ptr<Value>> PhiInstr::UsedValues() const {
  std::vector<std::shared_ptr<Value>> used_values;
  for (auto arg : args_) {
    used_values.push_back(arg->value());
  }
  return used_values;
}

void PhiInstr::WriteRefString(std::ostream& os) const {
  result()->WriteRefStringWithType(os);
  os << " = " << OperationString() << " ";
  for (size_t i = 0; i < args_.size(); i++) {
    if (i > 0) os << ", ";
    args_.at(i)->WriteRefString(os);
  }
}

bool PhiInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kPhi) return false;
  auto that = static_cast<const PhiInstr&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (args().size() != that.args().size()) return false;
  for (std::size_t i = 0; i < args().size(); i++) {
    const InheritedValue* value_a = args().at(i).get();
    const InheritedValue* value_b = that.args().at(i).get();
    if (!IsEqual(value_a, value_b)) return false;
  }
  return true;
}

bool Conversion::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kConversion) return false;
  auto that = static_cast<const Conversion&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (!IsEqual(operand().get(), that.operand().get())) return false;
  return true;
}

bool BoolNotInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kBoolNot) return false;
  auto that = static_cast<const BoolNotInstr&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (!IsEqual(operand().get(), that.operand().get())) return false;
  return true;
}

bool BoolBinaryInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kBoolBinary) return false;
  auto that = static_cast<const BoolBinaryInstr&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (operation() != that.operation()) return false;
  if (!IsEqual(operand_a().get(), that.operand_a().get())) return false;
  if (!IsEqual(operand_b().get(), that.operand_b().get())) return false;
  return true;
}

bool IntUnaryInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kIntUnary) return false;
  auto that = static_cast<const IntUnaryInstr&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (operation() != that.operation()) return false;
  if (!IsEqual(operand().get(), that.operand().get())) return false;
  return true;
}

bool IntCompareInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kIntCompare) return false;
  auto that = static_cast<const IntCompareInstr&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (operation() != that.operation()) return false;
  if (!IsEqual(operand_a().get(), that.operand_a().get())) return false;
  if (!IsEqual(operand_b().get(), that.operand_b().get())) return false;
  return true;
}

bool IntBinaryInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kIntBinary) return false;
  auto that = static_cast<const IntBinaryInstr&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (operation() != that.operation()) return false;
  if (!IsEqual(operand_a().get(), that.operand_a().get())) return false;
  if (!IsEqual(operand_b().get(), that.operand_b().get())) return false;
  return true;
}

bool IntShiftInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kIntShift) return false;
  auto that = static_cast<const IntShiftInstr&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (operation() != that.operation()) return false;
  if (!IsEqual(shifted().get(), that.shifted().get())) return false;
  if (!IsEqual(offset().get(), that.offset().get())) return false;
  return true;
}

bool PointerOffsetInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kPointerOffset) return false;
  auto that = static_cast<const PointerOffsetInstr&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (!IsEqual(pointer().get(), that.pointer().get())) return false;
  if (!IsEqual(offset().get(), that.offset().get())) return false;
  return true;
}

bool NilTestInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kNilTest) return false;
  auto that = static_cast<const NilTestInstr&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (!IsEqual(tested().get(), that.tested().get())) return false;
  return true;
}

bool MallocInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kMalloc) return false;
  auto that = static_cast<const MallocInstr&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (!IsEqual(size().get(), that.size().get())) return false;
  return true;
}

bool LoadInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kLoad) return false;
  auto that = static_cast<const LoadInstr&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (!IsEqual(address().get(), that.address().get())) return false;
  return true;
}

bool StoreInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kStore) return false;
  auto that = static_cast<const StoreInstr&>(that_instr);
  if (!IsEqual(address().get(), that.address().get())) return false;
  if (!IsEqual(value().get(), that.value().get())) return false;
  return true;
}

bool FreeInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kFree) return false;
  auto that = static_cast<const FreeInstr&>(that_instr);
  if (!IsEqual(address().get(), that.address().get())) return false;
  return true;
}

void JumpInstr::WriteRefString(std::ostream& os) const {
  os << OperationString() << " "
     << "{" << destination_ << "}";
}

bool JumpInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kJump) return false;
  auto that = static_cast<const JumpInstr&>(that_instr);
  if (destination() != that.destination()) return false;
  return true;
}

void JumpCondInstr::WriteRefString(std::ostream& os) const {
  os << OperationString() << " ";
  condition_->WriteRefString(os);
  os << ", {" << destination_true_ << "}, {" << destination_false_ << "}";
}

bool JumpCondInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kJumpCond) return false;
  auto that = static_cast<const JumpCondInstr&>(that_instr);
  if (!IsEqual(condition().get(), that.condition().get())) return false;
  if (destination_true() != that.destination_true()) return false;
  if (destination_false() != that.destination_false()) return false;
  return true;
}

std::vector<std::shared_ptr<Value>> SyscallInstr::UsedValues() const {
  std::vector<std::shared_ptr<Value>> used_values{syscall_num_};
  used_values.insert(used_values.end(), args_.begin(), args_.end());
  return used_values;
}

bool SyscallInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kSyscall) return false;
  auto that = static_cast<const SyscallInstr&>(that_instr);
  if (!IsEqual(result().get(), that.result().get())) return false;
  if (!IsEqual(syscall_num().get(), that.syscall_num().get())) return false;
  if (args().size() != that.args().size()) return false;
  for (std::size_t i = 0; i < args().size(); i++) {
    const Value* arg_a = args().at(i).get();
    const Value* arg_b = that.args().at(i).get();
    if (!IsEqual(arg_a, arg_b)) return false;
  }
  return true;
}

std::vector<std::shared_ptr<Value>> CallInstr::UsedValues() const {
  std::vector<std::shared_ptr<Value>> used_values{func_};
  used_values.insert(used_values.end(), args_.begin(), args_.end());
  return used_values;
}

bool CallInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kCall) return false;
  auto that = static_cast<const CallInstr&>(that_instr);
  if (!IsEqual(func().get(), that.func().get())) return false;
  if (results().size() != that.results().size()) return false;
  if (args().size() != that.args().size()) return false;
  for (std::size_t i = 0; i < results().size(); i++) {
    const Computed* result_a = results().at(i).get();
    const Computed* result_b = that.results().at(i).get();
    if (!IsEqual(result_a, result_b)) return false;
  }
  for (std::size_t i = 0; i < args().size(); i++) {
    const Value* arg_a = args().at(i).get();
    const Value* arg_b = that.args().at(i).get();
    if (!IsEqual(arg_a, arg_b)) return false;
  }
  return true;
}

bool ReturnInstr::operator==(const Instr& that_instr) const {
  if (that_instr.instr_kind() != InstrKind::kReturn) return false;
  auto that = static_cast<const ReturnInstr&>(that_instr);
  if (args().size() != that.args().size()) return false;
  for (std::size_t i = 0; i < args().size(); i++) {
    const Value* arg_a = args().at(i).get();
    const Value* arg_b = that.args().at(i).get();
    if (!IsEqual(arg_a, arg_b)) return false;
  }
  return true;
}

}  // namespace ir
