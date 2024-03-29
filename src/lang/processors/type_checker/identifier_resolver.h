//
//  identifier_resolver.h
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_identifier_resolver_h
#define lang_type_checker_identifier_resolver_h

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "src/lang/processors/issues/issues.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/types/info.h"
#include "src/lang/representation/types/info_builder.h"
#include "src/lang/representation/types/objects.h"
#include "src/lang/representation/types/package.h"
#include "src/lang/representation/types/scope.h"
#include "src/lang/representation/types/types.h"

namespace lang {
namespace type_checker {

class IdentifierResolver {
 public:
  static types::Package* CreatePackageAndResolveIdentifiers(
      std::string package_path, std::vector<ast::File*> package_files,
      std::function<types::Package*(std::string)> importer, types::InfoBuilder& info_builder,
      issues::IssueTracker& issues);

 private:
  IdentifierResolver(std::string package_path, std::vector<ast::File*> package_files,
                     std::function<types::Package*(std::string)> importer,
                     types::InfoBuilder& info_builder, issues::IssueTracker& issues)
      : package_path_(package_path),
        package_files_(package_files),
        importer_(importer),
        info_(info_builder.info()),
        info_builder_(info_builder),
        issues_(issues) {}

  void CreatePackage();
  void CreateFileScopes();

  void ResolveIdentifiers();
  void AddObjectToScope(types::Scope* scope, types::Object* object);

  void AddDefinedObjectsFromGenDecl(ast::GenDecl* gen_decl, types::Scope* scope,
                                    ast::File* file = nullptr);
  void AddDefinedObjectsFromImportSpec(ast::ImportSpec* import_spec, ast::File* file);
  void AddDefinedObjectsFromConstSpec(ast::ValueSpec* value_spec, types::Scope* scope);
  void AddDefinedObjectsFromVarSpec(ast::ValueSpec* value_spec, types::Scope* scope);
  void AddDefinedObjectFromTypeSpec(ast::TypeSpec* type_spec, types::Scope* scope);
  void AddDefinedObjectFromFuncDecl(ast::FuncDecl* func_decl, types::Scope* scope);

  void ResolveIdentifiersInGenDecl(ast::GenDecl* gen_decl, types::Scope* scope);
  void ResolveIdentifiersInValueSpec(ast::ValueSpec* value_spec, types::Scope* scope);
  void ResolveIdentifiersInTypeSpec(ast::TypeSpec* type_spec, types::Scope* scope);
  void ResolveIdentifiersInFuncDecl(ast::FuncDecl* func_decl, types::Scope* scope);

  void ResolveIdentifiersInTypeParamList(ast::TypeParamList* type_param_list, types::Scope* scope);
  void ResolveIdentifiersInExprReceiver(ast::ExprReceiver* expr_receiver, types::Scope* scope);
  void ResolveIdentifiersInTypeReceiver(ast::TypeReceiver* type_receiver, types::Scope* scope);
  void ResolveIdentifiersInRegularFuncFieldList(ast::FieldList* field_list, types::Scope* scope);

  void ResolveIdentifiersInStmt(ast::Stmt* stmt, types::Scope* scope);
  void ResolveIdentifiersInBlockStmt(ast::BlockStmt* body, types::Scope* scope);
  void ResolveIdentifiersInDeclStmt(ast::DeclStmt* decl_stmt, types::Scope* scope);
  void ResolveIdentifiersInAssignStmt(ast::AssignStmt* assign_stmt, types::Scope* scope);
  void ResolveIdentifiersInIfStmt(ast::IfStmt* if_stmt, types::Scope* scope);
  void ResolveIdentifiersInExprSwitchStmt(ast::ExprSwitchStmt* switch_stmt, types::Scope* scope);
  void ResolveIdentifiersInTypeSwitchStmt(ast::TypeSwitchStmt* switch_stmt, types::Scope* scope);
  void ResolveIdentifiersInCaseClause(ast::CaseClause* case_clause, types::Scope* scope,
                                      ast::Ident* type_switch_var = nullptr);
  void ResolveIdentifiersInForStmt(ast::ForStmt* for_stmt, types::Scope* scope);
  void ResolveIdentifiersInBranchStmt(ast::BranchStmt* branch_stmt, types::Scope* scope);

  void ResolveIdentifiersInExpr(ast::Expr* expr, types::Scope* scope);
  void ResolveIdentifiersInSelectionExpr(ast::SelectionExpr* sel, types::Scope* scope);
  void ResolveIdentifiersInFuncLit(ast::FuncLit* func_lit, types::Scope* scope);
  void ResolveIdentifiersInCompositeLit(ast::CompositeLit* composite_lit, types::Scope* scope);
  void ResolveIdentifiersInFuncType(ast::FuncType* func_type, types::Scope* scope);
  void ResolveIdentifiersInInterfaceType(ast::InterfaceType* interface_type, types::Scope* scope);
  void ResolveIdentifiersInStructType(ast::StructType* struct_type, types::Scope* scope);
  void ResolveIdentifier(ast::Ident* ident, types::Scope* scope);

  const std::string package_path_;
  const std::vector<ast::File*> package_files_;
  const std::function<types::Package*(std::string)> importer_;
  types::Info* info_;
  types::InfoBuilder& info_builder_;
  issues::IssueTracker& issues_;

  types::Package* package_;

  std::unordered_map<ast::File*, std::unordered_set<std::string>> file_imports_;
};

}  // namespace type_checker
}  // namespace lang

#endif /* lang_type_checker_identifier_resolver_h */
