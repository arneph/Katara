//
//  coordinator.h
//  Katara
//
//  Created by Arne Philipeit on 11/8/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_coordinator_h
#define lang_type_checker_coordinator_h

#include <memory>
#include <unordered_set>
#include <vector>

#include "src/lang/processors/issues/issues.h"
#include "src/lang/processors/type_checker/type_resolver.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/types/info.h"
#include "src/lang/representation/types/info_builder.h"
#include "src/lang/representation/types/objects.h"
#include "src/lang/representation/types/package.h"
#include "src/lang/representation/types/types.h"

namespace lang {
namespace type_checker {

class Coordinator {
 public:
  static bool ProcessPackage(std::vector<ast::File*> package_files, types::Package* package,
                             types::InfoBuilder& info_builder, issues::IssueTracker& issues);

 private:
  class Action {
   public:
    Action(std::unordered_set<types::Object*> prerequisites,
           std::unordered_set<types::Object*> defined_objects, std::function<bool()> executor)
        : prerequisites_(prerequisites), defined_objects_(defined_objects), executor_(executor) {}

    const std::unordered_set<types::Object*>& prerequisites() const { return prerequisites_; }

    const std::unordered_set<types::Object*>& defined_objects() const { return defined_objects_; }

    bool execute() { return executor_(); }

   private:
    std::unordered_set<types::Object*> prerequisites_;
    std::unordered_set<types::Object*> defined_objects_;

    std::function<bool()> executor_;
  };

  Coordinator(std::vector<ast::File*> package_files, types::Package* package,
              types::InfoBuilder& info_builder, issues::IssueTracker& issues);

  Action* CreateAction(std::function<bool()> executor);
  Action* CreateAction(std::unordered_set<types::Object*> prerequisites,
                       types::Object* defined_object, std::function<bool()> executor);
  Action* CreateAction(std::unordered_set<types::Object*> prerequisites,
                       std::unordered_set<types::Object*> defined_objects,
                       std::function<bool()> executor);

  void FindActions();
  void FindActionsForTypeDecl(ast::GenDecl* type_decl);
  void FindActionsForConstDecl(ast::GenDecl* const_decl);
  void FindActionsForVarDecl(ast::GenDecl* var_decl);
  void FindActionsForFuncDecl(ast::FuncDecl* func_decl);

  std::unordered_set<types::Object*> FindPrerequisites(ast::Node* node);

  std::vector<Action*> FindActionOrder();
  std::vector<Action*> FindActionOrderForActions(
      const std::vector<Action*>& actions, std::unordered_set<types::Object*>& defined_objects);
  void ReportLoopInActions(const std::vector<Action*>& actions);
  std::unordered_set<types::Object*> FindLoop(const std::vector<Action*>& actions,
                                              std::vector<Action*> stack);

  bool ExecuteActions(std::vector<Action*> ordered_actions);

  std::vector<ast::File*> package_files_;
  types::Package* package_;
  types::Info* info_;
  types::InfoBuilder& info_builder_;
  issues::IssueTracker& issues_;

  std::vector<std::unique_ptr<Action>> actions_;

  std::vector<Action*> const_and_type_actions_;
  std::vector<Action*> variable_and_func_decl_actions_;
  std::vector<Action*> func_body_actions_;

  TypeResolver type_resolver_;
};

}  // namespace type_checker
}  // namespace lang

#endif /* lang_type_checker_coordinator_h */
