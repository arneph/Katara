//
//  unique_pointer_lowerer.h
//  Katara
//
//  Created by Arne Philipeit on 6/2/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_lowerers_unique_pointer_lowerer_h
#define ir_lowerers_unique_pointer_lowerer_h

#include "src/ir/representation/program.h"

namespace lang {
namespace ir_lowerers {

void LowerUniquePointersInProgram(ir::Program* program);

}
}  // namespace lang

#endif /* ir_lowerers_unique_pointer_lowerer_h */
