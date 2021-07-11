//
//  phi_resolver.h
//  Katara
//
//  Created by Arne Philipeit on 2/23/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_proc_phi_resolver_h
#define ir_proc_phi_resolver_h

#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/values.h"

namespace ir_proc {

void ResolvePhisInFunc(ir::Func* func);

}  // namespace ir_proc

#endif /* ir_proc_phi_resolver_h */
