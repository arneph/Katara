//
//  print.cc
//  Katara
//
//  Created by Arne Philipeit on 3/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "print.h"

#include <sstream>

namespace ir_serialization {

std::string Print(const ir::Program* program) {
  std::stringstream ss;
  Print(program, ss);
  return ss.str();
}

void Print(const ir::Program* program, std::ostream& os) {
  std::vector<ir::func_num_t> fnums;
  fnums.reserve(program->funcs().size());
  for (auto& func : program->funcs()) {
    fnums.push_back(func->number());
  }
  std::sort(fnums.begin(), fnums.end());
  bool first = true;
  for (ir::func_num_t fnum : fnums) {
    if (!first) os << "\n\n";
    Print(program->GetFunc(fnum), os);
    first = false;
  }
}

std::string Print(const ir::Func* func) {
  std::stringstream ss;
  Print(func, ss);
  return ss.str();
}

void Print(const ir::Func* func, std::ostream& os) {
  func->WriteRefString(os);
  os << " (";
  for (size_t i = 0; i < func->args().size(); i++) {
    if (i > 0) os << ", ";
    func->args().at(i)->WriteRefStringWithType(os);
  }
  os << ") => (";
  for (size_t i = 0; i < func->result_types().size(); i++) {
    if (i > 0) os << ", ";
    func->result_types().at(i)->WriteRefString(os);
  }
  os << ") {";
  std::vector<ir::block_num_t> bnums;
  bnums.reserve(func->blocks().size());
  for (auto& block : func->blocks()) {
    bnums.push_back(block->number());
  }
  std::sort(bnums.begin(), bnums.end());
  for (ir::block_num_t bnum : bnums) {
    os << "\n";
    Print(func->GetBlock(bnum), os);
  }
  os << "\n}";
}

std::string Print(const ir::Block* block) {
  std::stringstream ss;
  Print(block, ss);
  return ss.str();
}

void Print(const ir::Block* block, std::ostream& os) {
  block->WriteRefString(os);
  for (auto& instr : block->instrs()) {
    os << "\n\t";
    instr->WriteRefString(os);
  }
}

std::string Print(const ir::Instr* instr) {
  std::stringstream ss;
  Print(instr, ss);
  return ss.str();
}

void Print(const ir::Instr* instr, std::ostream& os) { instr->WriteRefString(os); }

std::string Print(const ir::Value* value) {
  std::stringstream ss;
  Print(value, ss);
  return ss.str();
}

void Print(const ir::Value* value, std::ostream& os) { value->WriteRefStringWithType(os); }

std::string Print(const ir::Type* type) {
  std::stringstream ss;
  Print(type, ss);
  return ss.str();
}

void Print(const ir::Type* type, std::ostream& os) { type->WriteRefString(os); }

std::string Print(const ir::Object* object) {
  switch (object->object_kind()) {
    case ir::Object::Kind::kType:
      return Print(static_cast<const ir::Type*>(object));
    case ir::Object::Kind::kValue:
      return Print(static_cast<const ir::Value*>(object));
    case ir::Object::Kind::kInstr:
      return Print(static_cast<const ir::Instr*>(object));
    case ir::Object::Kind::kBlock:
      return Print(static_cast<const ir::Block*>(object));
    case ir::Object::Kind::kFunc:
      return Print(static_cast<const ir::Func*>(object));
    case ir::Object::Kind::kProgram:
      return Print(static_cast<const ir::Program*>(object));
  }
}

void Print(const ir::Object* object, std::ostream& os) {
  switch (object->object_kind()) {
    case ir::Object::Kind::kType:
      Print(static_cast<const ir::Type*>(object), os);
      break;
    case ir::Object::Kind::kValue:
      Print(static_cast<const ir::Value*>(object), os);
      break;
    case ir::Object::Kind::kInstr:
      Print(static_cast<const ir::Instr*>(object), os);
      break;
    case ir::Object::Kind::kBlock:
      Print(static_cast<const ir::Block*>(object), os);
      break;
    case ir::Object::Kind::kFunc:
      Print(static_cast<const ir::Func*>(object), os);
      break;
    case ir::Object::Kind::kProgram:
      Print(static_cast<const ir::Program*>(object), os);
      break;
  }
}

}  // namespace ir_serialization
