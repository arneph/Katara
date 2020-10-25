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
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "lang/representation/positions/positions.h"
#include "lang/representation/ast/ast.h"
#include "lang/representation/constants/constants.h"
#include "lang/representation/types/types.h"
#include "lang/processors/issues/issues.h"

namespace lang {
namespace type_checker {

types::Package * Check(std::string package_path,
                       std::vector<ast::File *> package_files,
                       std::function<types::Package *(std::string)> importer,
                       types::TypeInfo *type_info,
                       std::vector<issues::Issue>& issues);

}
}

#endif /* lang_type_checker_h */
