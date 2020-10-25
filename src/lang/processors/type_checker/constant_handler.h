//
//  constant_handler.h
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_constant_handler_h
#define lang_type_checker_constant_handler_h

#include <unordered_set>
#include <vector>

#include "lang/representation/ast/ast.h"
#include "lang/representation/types/types.h"
#include "lang/processors/issues/issues.h"

namespace lang {
namespace type_checker {

class ConstantHandler {
public:
    static void HandleConstants(std::vector<ast::File *> package_files,
                                types::Package *package,
                                types::TypeInfo *info,
                                std::vector<issues::Issue>& issues);
    
private:
    struct EvalInfo {
        types::Constant *constant_;
        
        ast::Ident *name_;
        ast::Expr *type_;
        ast::Expr *value_;
        
        int64_t iota_;
        
        std::unordered_set<types::Constant *> dependencies_;
    };
    
    ConstantHandler(std::vector<ast::File *> package_files,
                    types::Package *package,
                    types::TypeInfo *info,
                    std::vector<issues::Issue>& issues)
        : package_files_(package_files), package_(package), info_(info), issues_(issues) {}
    
    void EvaluateConstants();
    
    std::vector<EvalInfo>
    FindConstantsEvaluationOrder(std::vector<EvalInfo> eval_info);
    std::vector<EvalInfo> FindConstantEvaluationInfo();
    std::unordered_set<types::Constant *> FindConstantDependencies(ast::Expr *expr);
    
    void EvaluateConstant(EvalInfo& eval_info);
    bool EvaluateConstantExpr(ast::Expr *expr, int64_t iota);
    bool EvaluateConstantUnaryExpr(ast::UnaryExpr *expr, int64_t iota);
    bool EvaluateConstantCompareExpr(ast::BinaryExpr *expr, int64_t iota);
    bool EvaluateConstantShiftExpr(ast::BinaryExpr *expr, int64_t iota);
    bool EvaluateConstantBinaryExpr(ast::BinaryExpr *expr, int64_t iota);
    bool CheckTypesForRegualarConstantBinaryExpr(ast::BinaryExpr *expr,
                                                 constants::Value &x_value,
                                                 constants::Value &y_value,
                                                 types::Basic* &result_type);
    
    static constants::Value ConvertUntypedInt(constants::Value value, types::Basic::Kind kind);
    
    std::vector<ast::File *> package_files_;
    types::Package *package_;
    types::TypeInfo *info_;
    std::vector<issues::Issue>& issues_;
};

}
}

#endif /* lang_type_checker_constant_handler_h */
