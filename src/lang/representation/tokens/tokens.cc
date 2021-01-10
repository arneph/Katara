//
//  tokens.cc
//  Katara
//
//  Created by Arne Philipeit on 6/7/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "tokens.h"

namespace lang {
namespace tokens {

precedence_t prececende(Token token) {
  switch (token) {
    case kMul:
    case kQuo:
    case kRem:
    case kShl:
    case kShr:
    case kAnd:
    case kAndNot:
      return 5;
    case kAdd:
    case kSub:
    case kOr:
    case kXor:
      return 4;
    case kEql:
    case kNeq:
    case kLss:
    case kLeq:
    case kGtr:
    case kGeq:
      return 3;
    case kLAnd:
      return 2;
    case kLOr:
      return 1;
    default:
      return 0;
  }
}

}  // namespace tokens
}  // namespace lang
