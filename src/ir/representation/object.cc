//
//  object.cc
//  Katara
//
//  Created by Arne Philipeit on 3/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "object.h"

#include <sstream>

namespace ir {

std::string Object::RefString() const {
  std::stringstream ss;
  WriteRefString(ss);
  return ss.str();
}

}  // namespace ir
