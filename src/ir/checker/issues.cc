//
//  issues.cc
//  Katara
//
//  Created by Arne Philipeit on 8/21/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "issues.h"

#include <sstream>

namespace ir_checker {

std::string Issue::ToShortString() const {
  return "[" + std::to_string(int64_t(kind_)) + "] " + message_;
}

std::string Issue::ToDetailedString() const {
  std::stringstream buf;
  buf << "[" << int64_t(kind_) << "] " << message_ << "\n";
  buf << "\tScope: " << scope_object_->RefString() << "\n";
  if (!involved_objects_.empty()) {
    buf << "\tInvolved Objects:\n";
    for (const ir::Object* object : involved_objects_) {
      buf << "\t\t" << object->RefString() << "\n";
    }
  }
  return buf.str();
}

}  // namespace ir_checker
