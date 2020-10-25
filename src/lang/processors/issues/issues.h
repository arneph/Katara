//
//  issues.h
//  Katara
//
//  Created by Arne Philipeit on 10/24/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_issues_h
#define lang_issues_h

#include <string>
#include <vector>

#include "lang/representation/positions/positions.h"

namespace lang {
namespace issues {

enum class Origin {
    Parser,
    TypeChecker,
    PackageManager,
};

enum class Severity {
    Warning, // compilation can still complete
    Error,   // compilation can partially continue but not complete
    Fatal,   // compilation can not continue
};

class Issue {
public:
    Issue(Origin origin,
          Severity severity,
          pos::pos_t position,
          std::string message);
    Issue(Origin origin,
          Severity severity,
          std::vector<pos::pos_t> positions,
          std::string message);

    Origin origin() const;
    Severity severity() const;
    const std::vector<pos::pos_t>& positions() const;
    std::string message() const;
    
private:
    Origin origin_;
    Severity severity_;
    std::vector<pos::pos_t> positions_;
    std::string message_;
};

}
}

#endif /* lang_issues_h */
