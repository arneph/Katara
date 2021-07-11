//
//  live_range_analyzer.h
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_proc_live_range_analyzer_h
#define ir_proc_live_range_analyzer_h

#include "src/ir/info/block_live_ranges.h"
#include "src/ir/info/func_live_ranges.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"

namespace ir_proc {

class LiveRangeAnalyzer {
 public:
  LiveRangeAnalyzer(ir::Func* func) : func_(func), func_info_(func) {}

  ir_info::FuncLiveRanges& func_info();
  ir_info::InterferenceGraph& interference_graph();

 private:
  ir::Func* func_;

  bool func_info_ok_ = false;
  ir_info::FuncLiveRanges func_info_;

  bool interference_graph_ok_ = false;
  ir_info::InterferenceGraph interference_graph_;

  void FindLiveRanges();
  void BacktraceBlock(ir::Block* block, ir_info::BlockLiveRanges& info);

  void BuildInterferenceGraph();
  void BuildInterferenceGraph(ir::Block* block, ir_info::BlockLiveRanges& info);
};

}  // namespace ir_proc

#endif /* ir_proc_live_range_analyzer_h */
