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

#include "src/common/positions/positions.h"
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
#include "src/ir/serialization/positions.h"
#include "src/ir/serialization/scanner.h"
#include "src/ir/serialization/type_parser.h"

namespace ir_serialization {

class FuncParser {
 public:
  FuncParser(Scanner& scanner, ir_issues::IssueTracker& issue_tracker, TypeParser* type_parser,
             ConstantParser* constant_parser, ir::Program* program,
             ProgramPositions& program_positions, int64_t func_num_offset)
      : scanner_(scanner),
        issue_tracker_(issue_tracker),
        type_parser_(type_parser),
        constant_parser_(constant_parser),
        program_(program),
        program_positions_(program_positions),
        func_num_offset_(func_num_offset) {}
  virtual ~FuncParser() = default;

  ir::Func* ParseFunc();

 protected:
  struct ValuesParseResult {
    std::vector<std::shared_ptr<ir::Value>> values;
    std::vector<common::positions::range_t> value_ranges;
    common::positions::range_t range;
  };
  ValuesParseResult ParseValues(const ir::Type* expected_type);
  struct ValueParseResult {
    std::shared_ptr<ir::Value> value;
    common::positions::range_t range;
  };
  ValueParseResult ParseValue(const ir::Type* expected_type);

  struct ComputedValuesParseResult {
    std::vector<std::shared_ptr<ir::Computed>> values;
    std::vector<common::positions::range_t> value_ranges;
    common::positions::range_t range;
  };
  ComputedValuesParseResult ParseComputedValues(const ir::Type* expected_type);
  struct ComputedValueParseResult {
    std::shared_ptr<ir::Computed> value;
    common::positions::range_t range;
  };
  ComputedValueParseResult ParseComputedValue(const ir::Type* expected_type);

  struct InstrParseResult {
    std::unique_ptr<ir::Instr> instr;
    std::vector<common::positions::range_t> arg_ranges;
    common::positions::range_t args_range;
  };
  static InstrParseResult NoInstrParseResult();
  virtual InstrParseResult ParseInstrWithResults(std::vector<std::shared_ptr<ir::Computed>> results,
                                                 std::string instr_name);

  Scanner& scanner() { return scanner_; }
  ir_issues::IssueTracker& issue_tracker() { return issue_tracker_; }
  TypeParser* type_parser() { return type_parser_; }
  ConstantParser* constant_parser() { return constant_parser_; }
  ir::Program* program() { return program_; }

 private:
  struct FuncNumberParseResult {
    ir::func_num_t func_num;
    common::positions::range_t range;
  };
  FuncNumberParseResult ParseFuncNumber();
  void ParseFuncArgs();
  void ParseFuncResultTypes();
  void ParseFuncBody();
  void ConnectBlocks();

  void ParseBlock();
  struct BlockNumberParseResult {
    ir::block_num_t block_num;
    common::positions::range_t range;
  };
  BlockNumberParseResult ParseBlockNumber();
  void ParseBlockBody(ir::Block* block, BlockPositions& block_positions);

  std::unique_ptr<ir::Instr> ParseInstr();
  InstrParseResult ParseMovInstr(std::shared_ptr<ir::Computed> result);
  InstrParseResult ParsePhiInstr(std::shared_ptr<ir::Computed> result);
  InstrParseResult ParseConversionInstr(std::shared_ptr<ir::Computed> result);
  InstrParseResult ParseBoolNotInstr(std::shared_ptr<ir::Computed> result);
  InstrParseResult ParseBoolBinaryInstr(std::shared_ptr<ir::Computed> result,
                                        common::atomics::Bool::BinaryOp op);
  InstrParseResult ParseIntUnaryInstr(std::shared_ptr<ir::Computed> result,
                                      common::atomics::Int::UnaryOp op);
  InstrParseResult ParseIntCompareInstr(std::shared_ptr<ir::Computed> result,
                                        common::atomics::Int::CompareOp op);
  InstrParseResult ParseIntBinaryInstr(std::shared_ptr<ir::Computed> result,
                                       common::atomics::Int::BinaryOp op);
  InstrParseResult ParseIntShiftInstr(std::shared_ptr<ir::Computed> result,
                                      common::atomics::Int::ShiftOp op);
  InstrParseResult ParsePointerOffsetInstr(std::shared_ptr<ir::Computed> result);
  InstrParseResult ParseNilTestInstr(std::shared_ptr<ir::Computed> result);
  InstrParseResult ParseMallocInstr(std::shared_ptr<ir::Computed> result);
  InstrParseResult ParseLoadInstr(std::shared_ptr<ir::Computed> result);
  InstrParseResult ParseStoreInstr();
  InstrParseResult ParseFreeInstr();
  InstrParseResult ParseJumpInstr();
  InstrParseResult ParseJumpCondInstr();
  InstrParseResult ParseSyscallInstr(std::shared_ptr<ir::Computed> result);
  InstrParseResult ParseCallInstr(std::vector<std::shared_ptr<ir::Computed>> results);
  InstrParseResult ParseReturnInstr();

  ComputedValuesParseResult ParseInstrResults();
  struct InheritedValueParseResult {
    std::shared_ptr<ir::InheritedValue> value;
    common::positions::range_t range;
  };
  InheritedValueParseResult ParseInheritedValue(const ir::Type* expected_type);
  struct BlockValueParseResult {
    ir::block_num_t value;
    common::positions::range_t range;
  };
  BlockValueParseResult ParseBlockValue();

  Scanner& scanner_;
  ir_issues::IssueTracker& issue_tracker_;
  TypeParser* type_parser_;
  ConstantParser* constant_parser_;
  ir::Program* program_;
  ProgramPositions& program_positions_;
  int64_t func_num_offset_;
  ir::Func* func_;
  FuncPositions func_positions_;
  std::unordered_map<ir::value_num_t, std::shared_ptr<ir::Computed>> computed_values_;
};

}  // namespace ir_serialization

#endif /* ir_serialization_func_parser_h */
