//
//  positions.h
//  Katara
//
//  Created by Arne Philipeit on 10/6/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_positions_h
#define ir_serialization_positions_h

#include <unordered_map>
#include <vector>

#include "src/common/positions/positions.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"

namespace ir_serialization {

class FuncPositions;
class BlockPositions;
class InstrPositions;

class ProgramPositions {
 public:
  const FuncPositions& GetFuncPositions(const ir::Func* func) const;
  void AddFuncPositions(const ir::Func* func, FuncPositions func_positions);

  const BlockPositions& GetBlockPositions(const ir::Block* block) const;
  void AddBlockPositions(const ir::Block* block, BlockPositions block_positions);

  const InstrPositions& GetInstrPositions(const ir::Instr* instr) const;
  void AddInstrPositions(const ir::Instr* instr, InstrPositions instr_positions);

 private:
  std::unordered_map<const ir::Func*, FuncPositions> func_positions_;
  std::unordered_map<const ir::Block*, BlockPositions> block_positions_;
  std::unordered_map<const ir::Instr*, InstrPositions> instr_positions_;
};

class FuncPositions {
 public:
  // Entire function, from number to the end of the body.
  common::positions::range_t entire_func() const;
  // Function header, from number to the end of results.
  common::positions::range_t header() const;

  // Function number, including @ sign.
  common::positions::range_t number() const { return number_; }
  void set_number(common::positions::range_t number_range) { number_ = number_range; }

  // Function name, if present.
  common::positions::range_t name() const { return name_; }
  void set_name(common::positions::range_t name_range) { name_ = name_range; }

  // Function arguments list, including parentheses.
  common::positions::range_t args_range() const { return args_range_; }
  void set_args_range(common::positions::range_t args_range) { args_range_ = args_range; }

  // Individual function arguments.
  const std::vector<common::positions::range_t>& arg_ranges() const { return arg_ranges_; }
  void set_arg_ranges(std::vector<common::positions::range_t> arg_ranges) {
    arg_ranges_ = arg_ranges;
  }

  // Function result types list, including parentheses.
  common::positions::range_t results_range() const { return results_range_; }
  void set_results_range(common::positions::range_t results_range) {
    results_range_ = results_range;
  }

  // Individual function result types.
  const std::vector<common::positions::range_t>& result_ranges() const { return result_ranges_; }
  void set_result_ranges(std::vector<common::positions::range_t> result_ranges) {
    result_ranges_ = result_ranges;
  }

  // Function body, including opening and closing curly braces.
  common::positions::range_t body() const { return body_; }
  void set_body(common::positions::range_t body_range) { body_ = body_range; }

 private:
  common::positions::range_t number_ = common::positions::kNoRange;
  common::positions::range_t name_ = common::positions::kNoRange;
  common::positions::range_t args_range_ = common::positions::kNoRange;
  common::positions::range_t results_range_ = common::positions::kNoRange;
  common::positions::range_t body_ = common::positions::kNoRange;

  std::vector<common::positions::range_t> arg_ranges_;
  std::vector<common::positions::range_t> result_ranges_;
};

class BlockPositions {
 public:
  // Entire block, from number to the end of the body.
  common::positions::range_t entire_block() const;
  // Block header, from number to the end of number or name.
  common::positions::range_t header() const;

  // Block number, including curly braces.
  common::positions::range_t number() const { return number_; }
  void set_number(common::positions::range_t number_range) { number_ = number_range; }

  // Block name, if present.
  common::positions::range_t name() const { return name_; }
  void set_name(common::positions::range_t name_range) { name_ = name_range; }

  common::positions::range_t body() const { return body_; }
  void set_body(common::positions::range_t body_range) { body_ = body_range; }

 private:
  common::positions::range_t number_ = common::positions::kNoRange;
  common::positions::range_t name_ = common::positions::kNoRange;
  common::positions::range_t body_ = common::positions::kNoRange;
};

class InstrPositions {
 public:
  // Entire instruction, from defined values or name to the end of name or used values.
  common::positions::range_t entire_instr() const;

  // Instruction name.
  common::positions::range_t name() const { return name_; }
  void set_name(common::positions::range_t name_range) { name_ = name_range; }

  // List of defined values of the instruction.
  common::positions::range_t defined_values_range() const;

  // Individual defined values of the instruction.
  const std::vector<common::positions::range_t>& defined_value_ranges() const {
    return defined_value_ranges_;
  }
  void set_defined_value_ranges(std::vector<common::positions::range_t> defined_value_ranges) {
    defined_value_ranges_ = defined_value_ranges;
  }

  // List of used values of the instruction.
  common::positions::range_t used_values_range() const;

  // Individual used values of the instruction.
  const std::vector<common::positions::range_t>& used_value_ranges() const {
    return used_value_ranges_;
  }
  void set_used_value_ranges(std::vector<common::positions::range_t> used_value_ranges) {
    used_value_ranges_ = used_value_ranges;
  }

 private:
  common::positions::range_t name_ = common::positions::kNoRange;

  std::vector<common::positions::range_t> defined_value_ranges_;
  std::vector<common::positions::range_t> used_value_ranges_;
};

}  // namespace ir_serialization

#endif /* ir_serialization_positions_h */
