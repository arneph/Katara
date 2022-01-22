//
//  func_call_graph_optimizer.h
//  Katara
//
//  Created by Arne Philipeit on 1/22/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_optimizers_func_call_graph_optimizer_h
#define ir_optimizers_func_call_graph_optimizer_h

#include "src/ir/representation/program.h"

namespace ir_optimizers {

void RemoveUnusedFunctions(ir::Program* program);

}

#endif /* ir_optimizers_func_call_graph_optimizer_h */
