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

#include "src/common/atomics.h"
#include "src/ir/processors/scanner.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/values.h"

namespace ir_proc {

class Parser {
 public:
  static std::unique_ptr<ir::Program> Parse(std::istream& in_stream);
  static std::unique_ptr<ir::Program> Parse(Scanner& scanner);

 private:
  Parser(Scanner& scanner);

  void ParseProgram();

  void ParseFunc();
  void ParseFuncArgs(ir::Func* func);
  void ParseFuncResultTypes(ir::Func* func);
  void ParseFuncBody(ir::Func* func);
  void ConnectBlocks(ir::Func* func);

  void ParseBlock(ir::Func* func);

  std::unique_ptr<ir::Instr> ParseInstr();
  std::unique_ptr<ir::MovInstr> ParseMovInstr(std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir::PhiInstr> ParsePhiInstr(std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir::Conversion> ParseConversionInstr(std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir::BoolNotInstr> ParseBoolNotInstr(std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir::BoolBinaryInstr> ParseBoolBinaryInstr(std::shared_ptr<ir::Computed> result,
                                                            common::Bool::BinaryOp op);
  std::unique_ptr<ir::IntUnaryInstr> ParseIntUnaryInstr(std::shared_ptr<ir::Computed> result,
                                                        common::Int::UnaryOp op);
  std::unique_ptr<ir::IntCompareInstr> ParseIntCompareInstr(std::shared_ptr<ir::Computed> result,
                                                            common::Int::CompareOp op);
  std::unique_ptr<ir::IntBinaryInstr> ParseIntBinaryInstr(std::shared_ptr<ir::Computed> result,
                                                          common::Int::BinaryOp op);
  std::unique_ptr<ir::IntShiftInstr> ParseIntShiftInstr(std::shared_ptr<ir::Computed> result,
                                                        common::Int::ShiftOp op);
  std::unique_ptr<ir::JumpInstr> ParseJumpInstr();
  std::unique_ptr<ir::JumpCondInstr> ParseJumpCondInstr();
  std::unique_ptr<ir::CallInstr> ParseCallInstr(std::vector<std::shared_ptr<ir::Computed>> results);
  std::unique_ptr<ir::ReturnInstr> ParseReturnInstr();

  std::vector<std::shared_ptr<ir::Computed>> ParseInstrResults();

  std::shared_ptr<ir::InheritedValue> ParseInheritedValue(const ir::Type* expected_type);
  std::shared_ptr<ir::Value> ParseValue(const ir::Type* expected_type);
  std::shared_ptr<ir::Constant> ParseConstant(const ir::Type* expected_type);
  std::shared_ptr<ir::Computed> ParseComputed(const ir::Type* expected_type);
  ir::block_num_t ParseBlockValue();
  const ir::AtomicType* ParseType();

  Scanner& scanner_;
  std::unique_ptr<ir::Program> program_;
};

}  // namespace ir_proc

#endif /* ir_proc_parser_h */
