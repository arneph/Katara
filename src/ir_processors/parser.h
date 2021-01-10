//
//  parser.h
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_proc_parser_h
#define ir_proc_parser_h

#include <iostream>
#include <memory>

#include "ir/block.h"
#include "ir/func.h"
#include "ir/instr.h"
#include "ir/prog.h"
#include "ir/value.h"
#include "ir_processors/scanner.h"

namespace ir_proc {

class Parser {
 public:
  static ir::Prog* Parse(std::istream& in_stream);
  static ir::Prog* Parse(Scanner& scanner);

 private:
  Parser(Scanner& scanner);
  ~Parser();

  void ParseProg();

  void ParseFunc();
  void ParseFuncArgs(ir::Func* func);
  void ParseFuncResultTypes(ir::Func* func);
  void ParseFuncBody(ir::Func* func);
  void ConnectBlocks(ir::Func* func);

  void ParseBlock(ir::Func* func);

  ir::Instr* ParseInstr();
  ir::MovInstr* ParseMovInstr(ir::Computed result);
  ir::PhiInstr* ParsePhiInstr(ir::Computed result);
  ir::UnaryALInstr* ParseUnaryALInstr(ir::Computed result, ir::UnaryALOperation op);
  ir::BinaryALInstr* ParseBinaryALInstr(ir::Computed result, ir::BinaryALOperation op);
  ir::CompareInstr* ParseCompareInstr(ir::Computed result, ir::CompareOperation op);
  ir::JumpInstr* ParseJumpInstr();
  ir::JumpCondInstr* ParseJumpCondInstr();
  ir::CallInstr* ParseCallInstr(std::vector<ir::Computed> results);
  ir::ReturnInstr* ParseReturnInstr();

  std::vector<ir::Computed> ParseInstrResults();

  ir::InheritedValue ParseInheritedValue(ir::Type expected_type);
  ir::Value ParseValue(ir::Type expected_type = ir::Type::kUnknown);
  ir::Constant ParseConstant(ir::Type expected_type = ir::Type::kUnknown);
  ir::Computed ParseComputed(ir::Type expected_type = ir::Type::kUnknown);
  ir::BlockValue ParseBlockValue();
  ir::Type ParseType();

  Scanner& scanner_;
  ir::Prog* prog_;
};

}  // namespace ir_proc

#endif /* ir_proc_parser_h */
