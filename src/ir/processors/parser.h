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
#include <optional>

#include "ir/processors/scanner.h"
#include "ir/representation/block.h"
#include "ir/representation/func.h"
#include "ir/representation/instr.h"
#include "ir/representation/num_types.h"
#include "ir/representation/program.h"
#include "ir/representation/values.h"

namespace ir_proc {

class Parser {
 public:
  static std::unique_ptr<ir::Program> Parse(std::istream& in_stream);
  static std::unique_ptr<ir::Program> Parse(Scanner& scanner);

 private:
  Parser(Scanner& scanner);

  void ParseProg();

  void ParseFunc();
  void ParseFuncArgs(ir::Func* func);
  void ParseFuncResultTypes(ir::Func* func);
  void ParseFuncBody(ir::Func* func);
  void ConnectBlocks(ir::Func* func);

  void ParseBlock(ir::Func* func);

  std::unique_ptr<ir::Instr> ParseInstr();
  std::unique_ptr<ir::MovInstr> ParseMovInstr(std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir::PhiInstr> ParsePhiInstr(std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir::UnaryALInstr> ParseUnaryALInstr(std::shared_ptr<ir::Computed> result,
                                                      ir::UnaryALOperation op);
  std::unique_ptr<ir::BinaryALInstr> ParseBinaryALInstr(std::shared_ptr<ir::Computed> result,
                                                        ir::BinaryALOperation op);
  std::unique_ptr<ir::CompareInstr> ParseCompareInstr(std::shared_ptr<ir::Computed> result,
                                                      ir::CompareOperation op);
  std::unique_ptr<ir::JumpInstr> ParseJumpInstr();
  std::unique_ptr<ir::JumpCondInstr> ParseJumpCondInstr();
  std::unique_ptr<ir::CallInstr> ParseCallInstr(std::vector<std::shared_ptr<ir::Computed>> results);
  std::unique_ptr<ir::ReturnInstr> ParseReturnInstr();

  std::vector<std::shared_ptr<ir::Computed>> ParseInstrResults();

  std::shared_ptr<ir::InheritedValue> ParseInheritedValue(
      std::optional<ir::AtomicTypeKind> expected_type);
  std::shared_ptr<ir::Value> ParseValue(std::optional<ir::AtomicTypeKind> expected_type);
  std::shared_ptr<ir::Constant> ParseConstant(std::optional<ir::AtomicTypeKind> expected_type);
  std::shared_ptr<ir::Computed> ParseComputed(std::optional<ir::AtomicTypeKind> expected_type);
  ir::block_num_t ParseBlockValue();
  ir::AtomicType* ParseType();

  Scanner& scanner_;
  std::unique_ptr<ir::Program> program_;
};

}  // namespace ir_proc

#endif /* ir_proc_parser_h */
