//
//  print.h
//  Katara
//
//  Created by Arne Philipeit on 3/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_print_h
#define ir_serialization_print_h

#include <ostream>
#include <string>

#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"

namespace ir_serialization {

std::string Print(const ir::Program* program);
void Print(const ir::Program* program, std::ostream& os);

std::string Print(const ir::Func* func);
void Print(const ir::Func* func, std::ostream& os);

std::string Print(const ir::Block* block);
void Print(const ir::Block* block, std::ostream& os);

std::string Print(const ir::Instr* instr);
void Print(const ir::Instr* instr, std::ostream& os);

std::string Print(const ir::Value* value);
void Print(const ir::Value* value, std::ostream& os);

std::string Print(const ir::Type* type);
void Print(const ir::Type* type, std::ostream& os);

std::string Print(const ir::Object* object);
void Print(const ir::Object* object, std::ostream& os);

}  // namespace ir_serialization

#endif /* ir_serialization_print_h */
