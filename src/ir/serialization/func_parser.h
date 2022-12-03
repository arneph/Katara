//
//  func_parser.h
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_func_parser_h
#define ir_serialization_func_parser_h

#include <memory>
#include <unordered_map>
#include <vector>

#include "src/ir/issues/issues.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/constant_parser.h"
#include "src/ir/serialization/scanner.h"
#include "src/ir/serialization/type_parser.h"

namespace ir_serialization {

class FuncParser {
 public:
  FuncParser(Scanner& scanner, ir_issues::IssueTracker& issue_tracker, TypeParser* type_parser,
             ConstantParser* constant_parser, ir::Program* program, int64_t func_num_offset)
      : scanner_(scanner),
        issue_tracker_(issue_tracker),
        type_parser_(type_parser),
        constant_parser_(constant_parser),
        program_(program),
        func_num_offset_(func_num_offset) {}
  virtual ~FuncParser() = default;

  ir::Func* ParseFunc();

 protected:
  virtual std::unique_ptr<ir::Instr> ParseInstrWithResults(
      std::vector<std::shared_ptr<ir::Computed>> results, std::string instr_name);

  std::vector<std::shared_ptr<ir::Value>> ParseValues(const ir::Type* expected_type);
  std::shared_ptr<ir::Value> ParseValue(const ir::Type* expected_type);
  std::vector<std::shared_ptr<ir::Computed>> ParseComputedValues(const ir::Type* expected_type);
  std::shared_ptr<ir::Computed> ParseComputedValue(const ir::Type* expected_type);

  Scanner& scanner() { return scanner_; }
  ir_issues::IssueTracker& issue_tracker() { return issue_tracker_; }
  TypeParser* type_parser() { return type_parser_; }
  ConstantParser* constant_parser() { return constant_parser_; }
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
  std::unique_ptr<ir::SyscallInstr> ParseSyscallInstr(std::shared_ptr<ir::Computed> result);
  std::unique_ptr<ir::CallInstr> ParseCallInstr(std::vector<std::shared_ptr<ir::Computed>> results);
  std::unique_ptr<ir::ReturnInstr> ParseReturnInstr();

  std::vector<std::shared_ptr<ir::Computed>> ParseInstrResults();
  std::shared_ptr<ir::InheritedValue> ParseInheritedValue(const ir::Type* expected_type);
  ir::block_num_t ParseBlockValue();

  Scanner& scanner_;
  ir_issues::IssueTracker& issue_tracker_;
  TypeParser* type_parser_;
  ConstantParser* constant_parser_;
  ir::Program* program_;
  int64_t func_num_offset_;
  ir::Func* func_;
  std::unordered_map<ir::value_num_t, std::shared_ptr<ir::Computed>> computed_values_;
};

}  // namespace ir_serialization

#endif /* ir_serialization_func_parser_h */
