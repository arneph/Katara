//
//  print.cc
//  Katara
//
//  Created by Arne Philipeit on 3/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "print.h"

#include <memory>
#include <sstream>

#include "src/common/positions/positions.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/ir/serialization/positions.h"
#include "src/ir/serialization/printer.h"

namespace ir_serialization {
namespace {

using common::positions::range_t;

std::vector<range_t> PrintDefinedValuesList(const ir::Instr* instr, Printer& printer) {
  std::vector<range_t> defined_value_ranges;
  defined_value_ranges.reserve(instr->DefinedValues().size());
  for (size_t i = 0; i < instr->DefinedValues().size(); i++) {
    if (i > 0) {
      printer.Write(", ");
    }
    if (ir::Computed* defined_value = instr->DefinedValues().at(i).get();
        defined_value != nullptr) {
      defined_value_ranges.push_back(printer.Write(defined_value->RefStringWithType()));
    } else {
      defined_value_ranges.push_back(printer.Write("NULL"));
    }
  }
  return defined_value_ranges;
}

range_t PrintUsedValue(ir::Value* used_value, Printer& printer) {
  if (used_value != nullptr) {
    if (used_value->kind() == ir::Value::Kind::kConstant) {
      return printer.Write(used_value->RefStringWithType());
    } else {
      return printer.Write(used_value->RefString());
    }
  } else {
    return printer.Write("NULL");
  }
}

std::vector<range_t> PrintUsedValuesList(const ir::Instr* instr, Printer& printer) {
  std::vector<range_t> used_value_ranges;
  used_value_ranges.reserve(instr->DefinedValues().size());
  for (size_t i = 0; i < instr->UsedValues().size(); i++) {
    if (i > 0) {
      printer.Write(", ");
    }
    used_value_ranges.push_back(PrintUsedValue(instr->UsedValues().at(i).get(), printer));
  }
  return used_value_ranges;
}

range_t PrintBlockValue(ir::block_num_t block_num, Printer& printer) {
  return printer.WriteWithFunc([block_num, &printer] {
    printer.Write("{");
    printer.Write(std::to_string(block_num));
    printer.Write("}");
  });
}

void PrintJumpInstr(const ir::JumpInstr* jump_instr, Printer& printer,
                    ProgramPositions& program_positions) {
  InstrPositions jump_instr_positions;
  jump_instr_positions.set_name(printer.Write(jump_instr->OperationString()));
  printer.Write(" ");
  jump_instr_positions.set_used_value_ranges({PrintBlockValue(jump_instr->destination(), printer)});
  program_positions.AddInstrPositions(jump_instr, jump_instr_positions);
}

void PrintJumpCondInstr(const ir::JumpCondInstr* jump_cond_instr, Printer& printer,
                        ProgramPositions& program_positions) {
  InstrPositions jump_cond_instr_positions;
  jump_cond_instr_positions.set_name(printer.Write(jump_cond_instr->OperationString()));
  printer.Write(" ");
  range_t condition_range = PrintUsedValue(jump_cond_instr->condition().get(), printer);
  printer.Write(", ");
  range_t destination_true_range = PrintBlockValue(jump_cond_instr->destination_true(), printer);
  printer.Write(", ");
  range_t destination_false_range = PrintBlockValue(jump_cond_instr->destination_false(), printer);
  jump_cond_instr_positions.set_used_value_ranges(
      {condition_range, destination_true_range, destination_false_range});
  program_positions.AddInstrPositions(jump_cond_instr, jump_cond_instr_positions);
}

void PrintInstr(const ir::Instr* instr, Printer& printer, ProgramPositions& program_positions) {
  if (instr->instr_kind() == ir::InstrKind::kJump) {
    PrintJumpInstr(static_cast<const ir::JumpInstr*>(instr), printer, program_positions);
  } else if (instr->instr_kind() == ir::InstrKind::kJumpCond) {
    PrintJumpCondInstr(static_cast<const ir::JumpCondInstr*>(instr), printer, program_positions);
  } else {
    InstrPositions instr_positions;
    if (!instr->DefinedValues().empty()) {
      instr_positions.set_defined_value_ranges(PrintDefinedValuesList(instr, printer));
      printer.Write(" = ");
    }
    instr_positions.set_name(printer.Write(instr->OperationString()));
    if (!instr->UsedValues().empty()) {
      printer.Write(" ");
      instr_positions.set_used_value_ranges(PrintUsedValuesList(instr, printer));
    }
    program_positions.AddInstrPositions(instr, instr_positions);
  }
}

void PrintBlock(const ir::Block* block, Printer& printer, ProgramPositions& program_positions) {
  BlockPositions block_positions;
  block_positions.set_number(printer.WriteWithFunc([block, &printer] {
    printer.Write("{");
    printer.Write(std::to_string(block->number()));
    printer.Write("}");
  }));
  if (!block->name().empty()) {
    printer.Write(" ");
    block_positions.set_name(printer.Write(block->name()));
  }
  block_positions.set_body(printer.WriteWithFunc([block, &printer, &program_positions] {
    for (auto& instr : block->instrs()) {
      printer.Write("\n\t");
      PrintInstr(instr.get(), printer, program_positions);
    }
  }));
  program_positions.AddBlockPositions(block, block_positions);
}

struct FuncArgsPositions {
  range_t args_range;
  std::vector<range_t> arg_ranges;
};
FuncArgsPositions PrintFuncArgsList(const ir::Func* func, Printer& printer) {
  std::vector<range_t> arg_ranges;
  arg_ranges.reserve(func->args().size());
  range_t args_range = printer.WriteWithFunc([func, &printer, &arg_ranges] {
    printer.Write("(");
    for (size_t i = 0; i < func->args().size(); i++) {
      if (i > 0) {
        printer.Write(", ");
      }
      if (ir::Computed* arg = func->args().at(i).get(); arg != nullptr) {
        arg_ranges.push_back(printer.Write(arg->RefStringWithType()));
      } else {
        arg_ranges.push_back(printer.Write("NULL"));
      }
    }
    printer.Write(")");
  });
  return FuncArgsPositions{
      .args_range = args_range,
      .arg_ranges = arg_ranges,
  };
}

struct FuncResultsPositions {
  range_t results_range;
  std::vector<range_t> result_ranges;
};
FuncResultsPositions PrintFuncResultsList(const ir::Func* func, Printer& printer) {
  std::vector<range_t> result_ranges;
  result_ranges.reserve(func->result_types().size());
  range_t results_range = printer.WriteWithFunc([func, &printer, &result_ranges] {
    printer.Write("(");
    for (size_t i = 0; i < func->result_types().size(); i++) {
      if (i > 0) {
        printer.Write(", ");
      }
      if (const ir::Type* result_type = func->result_types().at(i); result_type != nullptr) {
        result_ranges.push_back(printer.Write(result_type->RefString()));
      } else {
        result_ranges.push_back(printer.Write("NULL"));
      }
    }
    printer.Write(")");
  });
  return FuncResultsPositions{
      .results_range = results_range,
      .result_ranges = result_ranges,
  };
}

void PrintFunc(const ir::Func* func, Printer& printer, ProgramPositions& program_positions) {
  FuncPositions func_positions;
  func_positions.set_number(printer.WriteWithFunc([func, &printer] {
    printer.Write("@");
    printer.Write(std::to_string(func->number()));
  }));
  if (!func->name().empty()) {
    printer.Write(" ");
    func_positions.set_name(printer.Write(func->name()));
  }
  printer.Write(" ");
  auto [args_range, arg_ranges] = PrintFuncArgsList(func, printer);
  func_positions.set_args_range(args_range);
  func_positions.set_arg_ranges(arg_ranges);
  printer.Write(" => ");
  auto [results_range, result_ranges] = PrintFuncResultsList(func, printer);
  func_positions.set_results_range(results_range);
  func_positions.set_result_ranges(result_ranges);
  printer.Write(" ");

  func_positions.set_body(printer.WriteWithFunc([func, &printer, &program_positions] {
    printer.Write("{");
    std::vector<ir::block_num_t> bnums;
    bnums.reserve(func->blocks().size());
    for (auto& block : func->blocks()) {
      bnums.push_back(block->number());
    }
    std::sort(bnums.begin(), bnums.end());
    for (ir::block_num_t bnum : bnums) {
      printer.Write("\n");
      PrintBlock(func->GetBlock(bnum), printer, program_positions);
    }
    printer.Write("\n}");
  }));
  program_positions.AddFuncPositions(func, func_positions);
}

ProgramPositions PrintProgram(const ir::Program* program, Printer& printer) {
  std::vector<ir::func_num_t> fnums;
  fnums.reserve(program->funcs().size());
  for (auto& func : program->funcs()) {
    fnums.push_back(func->number());
  }
  std::sort(fnums.begin(), fnums.end());

  ProgramPositions program_positions;
  for (ir::func_num_t fnum : fnums) {
    PrintFunc(program->GetFunc(fnum), printer, program_positions);
    printer.Write("\n\n");
  }
  return program_positions;
}

}  // namespace

std::string PrintProgram(const ir::Program* program) {
  Printer printer = Printer::FromPostion(common::positions::kNoPos);
  ProgramPositions program_positions = PrintProgram(program, printer);
  return printer.contents();
}

std::string PrintFunc(const ir::Func* func) {
  Printer printer = Printer::FromPostion(common::positions::kNoPos);
  ProgramPositions program_positions;
  PrintFunc(func, printer, program_positions);
  return printer.contents();
}

std::string PrintBlock(const ir::Block* block) {
  Printer printer = Printer::FromPostion(common::positions::kNoPos);
  ProgramPositions program_positions;
  PrintBlock(block, printer, program_positions);
  return printer.contents();
}

std::string PrintInstr(const ir::Instr* instr) {
  Printer printer = Printer::FromPostion(common::positions::kNoPos);
  ProgramPositions program_positions;
  PrintInstr(instr, printer, program_positions);
  return printer.contents();
}

FilePrintResults PrintProgramToNewFile(std::string file_name, const ir::Program* program,
                                       common::positions::FileSet& file_set) {
  Printer printer = Printer::FromPostion(file_set.NextFileStart());
  ProgramPositions program_positions = PrintProgram(program, printer);
  common::positions::File* file = file_set.AddFile(file_name, printer.contents());
  return FilePrintResults{
      .file = file,
      .program_positions = program_positions,
  };
}

}  // namespace ir_serialization
