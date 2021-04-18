//
//  live_range_analyzer.h
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_proc_live_range_analyzer_h
#define ir_proc_live_range_analyzer_h

#include "ir/representation/block.h"
#include "ir/representation/func.h"
#include "ir/info/func_live_range_info.h"

namespace ir_proc {

class LiveRangeAnalyzer {
 public:
  LiveRangeAnalyzer(ir::Func* func);
  ~LiveRangeAnalyzer();

  ir_info::FuncLiveRangeInfo& func_info();
  ir_info::InterferenceGraph& interference_graph();

 private:
  ir::Func* func_;

  bool func_info_ok_ = false;
  ir_info::FuncLiveRangeInfo func_info_;

  bool interference_graph_ok_ = false;
  ir_info::InterferenceGraph interference_graph_;

  void FindLiveRanges();
  void BacktraceBlock(ir::Block* block, ir_info::BlockLiveRangeInfo& info);

  void BuildInterferenceGraph();
  void BuildInterferenceGraph(ir::Block* block, ir_info::BlockLiveRangeInfo& info);
};

}  // namespace ir_proc

#endif /* ir_proc_live_range_analyzer_h */
