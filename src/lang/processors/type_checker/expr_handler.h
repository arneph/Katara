//
//  expr_handler.h
//  Katara
//
//  Created by Arne Philipeit on 11/8/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_expr_handler_h
#define lang_type_checker_expr_handler_h

#include <vector>

#include "lang/representation/ast/ast.h"
#include "lang/representation/types/types.h"
#include "lang/representation/types/objects.h"
#include "lang/representation/types/info.h"
#include "lang/representation/types/info_builder.h"
#include "lang/processors/issues/issues.h"

namespace lang {
namespace type_checker {

class ExprHandler {
public:
    static bool ProcessExpr(ast::Expr *expr,
                            types::InfoBuilder& info_builder,
                            std::vector<issues::Issue>& issues);
    
private:
    ExprHandler(types::InfoBuilder& info_builder,
                std::vector<issues::Issue>& issues)
    : info_(info_builder.info()), info_builder_(info_builder), issues_(issues) {}
    
    bool CheckExprs(std::vector<ast::Expr *> exprs);
    bool CheckExpr(ast::Expr *expr);
    
    bool CheckUnaryArithmeticOrBitExpr(ast::UnaryExpr *unary_expr);
    bool CheckUnaryLogicExpr(ast::UnaryExpr *unary_expr);
    bool CheckUnaryAddressExpr(ast::UnaryExpr *unary_expr);
    bool CheckBinaryArithmeticOrBitExpr(ast::BinaryExpr *binary_expr);
    bool CheckBinaryShiftExpr(ast::BinaryExpr *binary_expr);
    bool CheckBinaryLogicExpr(ast::BinaryExpr *binary_expr);
    bool CheckBinaryComparisonExpr(ast::BinaryExpr *binary_expr);
    bool CheckParenExpr(ast::ParenExpr *paren_expr);
    
    bool CheckSelectionExpr(ast::SelectionExpr *selection_expr);
    bool CheckTypeAssertExpr(ast::TypeAssertExpr *type_assert_expr);
    bool CheckIndexExpr(ast::IndexExpr *index_expr);
    
    bool CheckCallExpr(ast::CallExpr *call_expr);
    bool CheckCallExprWithTypeConversion(ast::CallExpr *call_expr,
                                         ast::Expr *func_expr,
                                         ast::TypeArgList *type_args_expr,
                                         std::vector<ast::Expr *> arg_exprs);
    bool CheckCallExprWithBuiltin(ast::CallExpr *call_expr,
                                 ast::Expr *func_expr,
                                 ast::TypeArgList *type_args_expr,
                                 std::vector<ast::Expr *> arg_exprs);
    bool CheckCallExprWithFuncCall(ast::CallExpr *call_expr,
                                   ast::Expr *func_expr,
                                   ast::TypeArgList *type_args_expr,
                                   std::vector<ast::Expr *> arg_exprs);
    types::Signature * CheckFuncCallTypeArgs(types::Signature *signature,
                                             ast::TypeArgList *type_args_expr);
    void CheckFuncCallArgs(types::Signature *signature,
                           ast::CallExpr *call_expr,
                           std::vector<ast::Expr *> arg_exprs);
    void CheckFuncCallResultType(types::Signature *signature,
                                 ast::CallExpr *call_expr);
    
    bool CheckFuncLit(ast::FuncLit *func_lit);
    bool CheckCompositeLit(ast::CompositeLit *composite_lit);
    
    bool CheckBasicLit(ast::BasicLit *basic_lit);
    bool CheckIdent(ast::Ident *ident);
    
    types::Info *info_;
    types::InfoBuilder& info_builder_;
    std::vector<issues::Issue>& issues_;
};

}
}

#endif /* lang_type_checker_expr_handler_h */
