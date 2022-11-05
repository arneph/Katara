//
//  base_handler.h
//  Katara
//
//  Created by Arne Philipeit on 1/9/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_base_handler_h
#define lang_type_checker_base_handler_h

#include <vector>

#include "src/lang/processors/issues/issues.h"
#include "src/lang/representation/types/info.h"
#include "src/lang/representation/types/info_builder.h"

namespace lang {
namespace type_checker {

class BaseHandler {
 public:
  BaseHandler(class TypeResolver& type_resolver, types::InfoBuilder& info_builder,
              issues::IssueTracker& issues)
      : info_(info_builder.info()),
        info_builder_(info_builder),
        issues_(issues),
        type_resolver_(type_resolver) {}
  virtual ~BaseHandler() {}

 protected:
  types::Info* info() const { return info_; }
  types::InfoBuilder& info_builder() { return info_builder_; }
  issues::IssueTracker& issues() { return issues_; }

  class TypeResolver& type_resolver() const { return type_resolver_; }

 private:
  types::Info* info_;
  types::InfoBuilder& info_builder_;
  issues::IssueTracker& issues_;

  class TypeResolver& type_resolver_;
};

}  // namespace type_checker
}  // namespace lang

#endif /* lang_type_checker_base_handler_h */
