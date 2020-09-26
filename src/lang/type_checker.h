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

#include "lang/positions.h"
#include "lang/ast.h"
#include "lang/types.h"

namespace lang {
namespace type_checker {

class TypeChecker {
public:
    struct Error {
        std::vector<pos::pos_t> pos_;
        std::string message_;
    };
    
    static void Check(ast::File *ast_file,
                      types::TypeInfo *info,
                      std::function<types::Package *(std::string)> importer,
                      std::vector<Error>& errors);
    
private:
    struct ConstantEvaluationInfo {
        types::Constant *constant_;
        
        ast::Ident *name_;
        ast::Expr *type_;
        ast::Expr *value_;
        
        int64_t iota_;
        
        std::unordered_set<types::Constant *> dependencies_;
    };
    
    TypeChecker(ast::File *ast_file,
                types::TypeInfo *info,
                std::function<types::Package *(std::string)> importer,
                std::vector<Error>& errors)
        : ast_file_(ast_file),
          info_(info),
          importer_(importer),
          errors_(errors) {}
    ~TypeChecker() {}
    
// Preparation:
    void SetupUniverse();
    void SetupPredeclaredTypes();
    void SetupPredeclaredConstants();
    void SetupPredeclaredNil();
    
// Identifier Resolution:
    void ResolveIdentifiers();
    void AddObjectToScope(types::Object *object, types::Scope *scope);
    
    void AddDefinedObjectsFromGenDecl(ast::GenDecl *gen_decl, types::Scope *scope);
    void AddDefinedObjectsFromImportSpec(ast::ImportSpec *import_spec);
    void AddDefinedObjectsFromConstSpec(ast::ValueSpec *value_spec, types::Scope *scope);
    void AddDefinedObjectsFromVarSpec(ast::ValueSpec *value_spec, types::Scope *scope);
    void AddDefinedObjectFromTypeSpec(ast::TypeSpec *type_spec, types::Scope *scope);
    void AddDefinedObjectFromFuncDecl(ast::FuncDecl *func_decl, types::Scope *scope);
    
    void ResolveIdentifiersInGenDecl(ast::GenDecl *gen_decl, types::Scope *scope);
    void ResolveIdentifiersInValueSpec(ast::ValueSpec *value_spec, types::Scope *scope);
    void ResolveIdentifiersInTypeSpec(ast::TypeSpec *type_spec, types::Scope *scope);
    void ResolveIdentifiersInFuncDecl(ast::FuncDecl *func_decl, types::Scope *scope);
    
    void ResolveIdentifiersInTypeParamList(ast::TypeParamList *type_param_list,
                                           types::Scope *scope);
    void ResolveIdentifiersInFuncReceiverFieldList(ast::FieldList *field_list, types::Scope *scope);
    void ResolveIdentifiersInRegularFuncFieldList(ast::FieldList *field_list, types::Scope *scope);
    
    void ResolveIdentifiersInStmt(ast::Stmt *stmt, types::Scope *scope);
    void ResolveIdentifiersInBlockStmt(ast::BlockStmt *body, types::Scope *scope);
    void ResolveIdentifiersInDeclStmt(ast::DeclStmt *decl_stmt, types::Scope *scope);
    void ResolveIdentifiersInAssignStmt(ast::AssignStmt *assign_stmt, types::Scope *scope);
    void ResolveIdentifiersInIfStmt(ast::IfStmt *if_stmt, types::Scope *scope);
    void ResolveIdentifiersInSwitchStmt(ast::SwitchStmt *switch_stmt, types::Scope *scope);
    void ResolveIdentifiersInCaseClause(ast::CaseClause *case_clause,
                                        types::Scope *scope,
                                        ast::Ident *type_switch_var_ident = nullptr);
    void ResolveIdentifiersInForStmt(ast::ForStmt *for_stmt, types::Scope *scope);
    void ResolveIdentifiersInBranchStmt(ast::BranchStmt *branch_stmt, types::Scope *scope);
    
    void ResolveIdentifiersInExpr(ast::Expr *expr, types::Scope *scope);
    void ResolveIdentifiersInSelectionExpr(ast::SelectionExpr *sel, types::Scope *scope);
    void ResolveIdentifiersInFuncLit(ast::FuncLit *func_lit, types::Scope *scope);
    void ResolveIdentifiersInCompositeLit(ast::CompositeLit *composite_lit, types::Scope *scope);
    void ResolveIdentifiersInFuncType(ast::FuncType *func_type, types::Scope *scope);
    void ResolveIdentifiersInInterfaceType(ast::InterfaceType *interface_type, types::Scope *scope);
    void ResolveIdentifiersInStructType(ast::StructType *struct_type, types::Scope *scope);
    void ResolveIdentifier(ast::Ident *ident, types::Scope *scope);
    
// Type building:
    
// Init Order:
    void FindInitOrder();
    void FindInitializersAndDependencies(std::map<types::Variable *,
                                                  types::Initializer *>& inits,
                                         std::map<types::Object *,
                                                  std::unordered_set<types::Object *>>& deps);
    std::unordered_set<types::Object *> FindInitDependenciesOfNode(ast::Node *node);
    
// Constant evaluation:
    void EvaluateConstants();
    
    std::vector<ConstantEvaluationInfo>
        FindConstantsEvaluationOrder(std::vector<ConstantEvaluationInfo> info);
    std::vector<ConstantEvaluationInfo> FindConstantEvaluationInfo();
    std::unordered_set<types::Constant *> FindConstantDependencies(ast::Expr *expr);
    
    void EvaluateConstant(ConstantEvaluationInfo& info);
    bool EvaluateConstantExpr(ast::Expr *expr, int64_t iota);
    bool EvaluateConstantUnaryExpr(ast::UnaryExpr *expr, int64_t iota);
    bool EvaluateConstantCompareExpr(ast::BinaryExpr *expr, int64_t iota);
    bool EvaluateConstantShiftExpr(ast::BinaryExpr *expr, int64_t iota);
    bool EvaluateConstantBinaryExpr(ast::BinaryExpr *expr, int64_t iota);
    bool CheckTypesForRegualarConstantBinaryExpr(ast::BinaryExpr *expr,
                                                 constant::Value &x_value,
                                                 constant::Value &y_value,
                                                 types::Basic* &result_type);
    
    static constant::Value ConvertUntypedInt(constant::Value value, types::Basic::Kind kind);
    
    ast::File *ast_file_;
    types::TypeInfo *info_;
    std::function<types::Package *(std::string)> importer_;
    std::vector<Error>& errors_;
    
    std::unordered_set<std::string> imported_;
    
    types::Package *package_;
    types::Scope *file_scope_;
    types::Scope *current_func_scope_;
};

}
}

#endif /* lang_type_checker_h */
