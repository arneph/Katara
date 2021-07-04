//
//  values.cc
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "values.h"

#include <sstream>

namespace ir {

std::string PointerConstant::ToString() const {
  std::stringstream sstream;
  sstream << "0x" << std::hex << value_;
  return sstream.str();
}

}  // namespace ir
