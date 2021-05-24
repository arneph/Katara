//
//  types.cc
//  Katara
//
//  Created by Arne Philipeit on 5/14/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "types.h"

namespace lang {
namespace ir_ext {

ArrayBuilder::ArrayBuilder() { array_ = std::unique_ptr<Array>(new Array()); }

StructBuilder::StructBuilder() { struct_ = std::unique_ptr<Struct>(new Struct()); }

void StructBuilder::AddField(std::string name, ir::Type* field_type) {
  struct_->fields_.push_back({name, field_type});
}

}  // namespace ir_ext
}  // namespace lang
