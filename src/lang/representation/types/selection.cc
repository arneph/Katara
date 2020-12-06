//
//  selection.cc
//  Katara
//
//  Created by Arne Philipeit on 12/6/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "selection.h"

namespace lang {
namespace types {

Selection::Kind Selection::kind() const {
    return kind_;
}

Type * Selection::receiver_type() const {
    return receiver_type_;
}

Type * Selection::type() const {
    return type_;
}

Object * Selection::object() const {
    return object_;
}

}
}
