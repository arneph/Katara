//
//  issues.cpp
//  Katara
//
//  Created by Arne Philipeit on 10/24/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "issues.h"

namespace lang {
namespace issues {

Issue::Issue(Origin origin,
             Severity severity,
             pos::pos_t position,
             std::string message)
    : Issue(origin, severity, std::vector<pos::pos_t>{position}, message) {}

Issue::Issue(Origin origin,
             Severity severity,
             std::vector<pos::pos_t> positions,
             std::string message)
    : origin_(origin), severity_(severity), positions_(positions), message_(message) {}

Origin Issue::origin() const {
    return origin_;
}

Severity Issue::severity() const {
    return severity_;
}

const std::vector<pos::pos_t>& Issue::positions() const {
    return positions_;
}

std::string Issue::message() const {
    return message_;
}

}
}
