//
//  expr_handler.h
//  Katara
//
//  Created by Arne Philipeit on 11/8/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_expr_handler_h
#define lang_type_checker_expr_handler_h

#include "lang/representation/ast/ast.h"

namespace lang {
namespace type_checker {

class ExprHandler {
public:
    static bool ProcessExpr(ast::Expr *expr,
                            bool ignore_func_lit_bodies = false);
    
private:
    
};

}
}

#endif /* lang_type_checker_expr_handler_h */
