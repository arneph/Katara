//
//  func_parser.h
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_func_parser_h
#define ir_serialization_func_parser_h

#include <iostream>
#include <memory>
#include <optional>

#include "src/common/atomics/atomics.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/scanner.h"

namespace ir_serialization {

class FuncParser {
 public:
  FuncParser(Scanner& scanner, ir::Program* program) : scanner_(scanner), program_(program) {}
  virtual ~FuncParser() = default;

  ir::Func* ParseFunc();

 protected:
  virtual std::unique_ptr<ir::Instr> ParseInstrWithResults(
      std::vector<std::shared_ptr<ir::Computed>> results, std::string instr_name);
  virtual std::shared_ptr<ir::Constant> ParseConstant(const ir::Type* expected_type);
  virtual const ir::Type* ParseType();

  std::vector<std::shared_ptr<ir::Value>> ParseValues(const ir::Type* expected_type);
  std::shared_ptr<ir::Value> ParseValue(const ir::Type* expected_type);
  std::vector<std::shared_ptr<ir::Computed>> ParseComputeds(const ir::Type* expected_type);
  std::shared_ptr<ir::Computed> ParseComputed(const ir::Type* expected_type);
  std::vector<const ir::Type*> ParseTypes();

  Scanner& scanner() { return scanner_; }
  ir::Program* program() { return program_; }

 private:
  void ParseFuncArgs();
  void ParseFuncResultTypes();
  void ParseFuncBody();
  void ConnectBlocks();

  void ParseBlock();

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
  std::unique_ptr<ir::PointerOffsetInstr> ParsePointerOffsetInstr(
      std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir::NilTestInstr> ParseNilTestInstr(std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir::MallocInstr> ParseMallocInstr(std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir::LoadInstr> ParseLoadInstr(std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir::StoreInstr> ParseStoreInstr();
  std::unique_ptr<ir::FreeInstr> ParseFreeInstr();
  std::unique_ptr<ir::JumpInstr> ParseJumpInstr();
  std::unique_ptr<ir::JumpCondInstr> ParseJumpCondInstr();
  std::unique_ptr<ir::CallInstr> ParseCallInstr(std::vector<std::shared_ptr<ir::Computed>> results);
  std::unique_ptr<ir::ReturnInstr> ParseReturnInstr();

  std::vector<std::shared_ptr<ir::Computed>> ParseInstrResults();

  std::shared_ptr<ir::InheritedValue> ParseInheritedValue(const ir::Type* expected_type);
  std::shared_ptr<ir::PointerConstant> ParsePointerConstant();
  std::shared_ptr<ir::FuncConstant> ParseFuncConstant();
  std::shared_ptr<ir::Constant> ParseBoolOrIntConstant(const ir::Type* expected_type);
  ir::block_num_t ParseBlockValue();

  Scanner& scanner_;
  ir::Program* program_;
  ir::Func* func_;

  std::unordered_map<ir::value_num_t, std::shared_ptr<ir::Computed>> computed_values_;
};

}  // namespace ir_serialization

#endif /* ir_serialization_func_parser_h */
