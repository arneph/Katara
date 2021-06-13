//
//  type_checker.h
//  Katara
//
//  Created by Arne Philipeit on 7/26/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_h
#define lang_type_checker_h

#include <functional>
#include <string>
#include <vector>

#include "src/lang/processors/issues/issues.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/types/info.h"
#include "src/lang/representation/types/package.h"

namespace lang {
namespace type_checker {

types::Package* Check(std::string package_path, ast::Package* package,
                      std::function<types::Package*(std::string)> importer, types::Info* info,
                      issues::IssueTracker& issues);

}
}  // namespace lang

#endif /* lang_type_checker_h */
