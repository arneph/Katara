//
//  func_values_builder.h
//  Katara
//
//  Created by Arne Philipeit on 2/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_analyzers_func_values_builder_h
#define ir_analyzers_func_values_builder_h

#include "src/ir/info/func_values.h"
#include "src/ir/representation/func.h"

namespace ir_analyzers {

const ir_info::FuncValues FindValuesInFunc(const ir::Func* func);

}

#endif /* ir_analyzers_func_values_builder_h */
