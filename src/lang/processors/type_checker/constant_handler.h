//
//  constant_handler.h
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_constant_handler_h
#define lang_type_checker_constant_handler_h

#include <vector>

#include "lang/representation/ast/ast.h"
#include "lang/representation/types/types.h"
#include "lang/processors/issues/issues.h"

namespace lang {
namespace type_checker {

class ConstantHandler {
public:
    static bool ProcessConstant(types::Constant *constant,
                                types::Type *type,
                                ast::Expr *value,
                                int64_t iota,
                                types::TypeInfo *info,
                                std::vector<issues::Issue>& issues);
    
    static bool ProcessConstantExpr(ast::Expr *constant_expr,
                                    int64_t iota,
                                    types::TypeInfo *info,
                                    std::vector<issues::Issue>& issues);
    
private:
    ConstantHandler(int64_t iota,
                    types::TypeInfo *info,
                    std::vector<issues::Issue>& issues)
    : iota_(iota), info_(info), issues_(issues) {}
    
    bool ProcessConstantDefinition(types::Constant *constant,
                                   types::Type *type,
                                   ast::Expr *value);
    
    bool EvaluateConstantExpr(ast::Expr *expr);
    bool EvaluateConstantUnaryExpr(ast::UnaryExpr *expr);
    bool EvaluateConstantCompareExpr(ast::BinaryExpr *expr);
    bool EvaluateConstantShiftExpr(ast::BinaryExpr *expr);
    bool EvaluateConstantBinaryExpr(ast::BinaryExpr *expr);
    bool CheckTypesForRegualarConstantBinaryExpr(ast::BinaryExpr *expr,
                                                 constants::Value &x_value,
                                                 constants::Value &y_value,
                                                 types::Basic* &result_type);
    
    static constants::Value ConvertUntypedInt(constants::Value value, types::Basic::Kind kind);
    
    int64_t iota_;
    types::TypeInfo *info_;
    std::vector<issues::Issue>& issues_;
};

}
}

#endif /* lang_type_checker_constant_handler_h */
