//
//  issues.cpp
//  Katara
//
//  Created by Arne Philipeit on 10/24/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "issues.h"

#include "src/common/logging/logging.h"

namespace lang {
namespace issues {

using ::common::issues::Severity;
using ::common::logging::fail;

Origin Issue::origin() const {
  if (IssueKind::kParserStart < kind() && kind() < IssueKind::kParserEnd) {
    return Origin::kParser;
  } else if (IssueKind::kIdentifierResolverStart < kind() &&
             kind() < IssueKind::kIdentiferResolverEnd) {
    return Origin::kIdentifierResolver;
  } else if (IssueKind::kTypeResolverStart < kind() && kind() < IssueKind::kTypeResolverEnd) {
    return Origin::kTypeResolver;
  } else if (IssueKind::kPackageManagerStart < kind() && kind() < IssueKind::kPackageManagerEnd) {
    return Origin::kPackageManager;
  } else {
    fail("unexpected issue kind");
  }
}

Severity Issue::severity() const {
  if (IssueKind::kParserFatalStart < kind() && kind() < IssueKind::kParserFatalEnd) {
    return Severity::kFatal;
  } else if (IssueKind::kIdentifierResolverErrorStart < kind() &&
             kind() < IssueKind::kIdentifierResolverErrorEnd) {
    return Severity::kError;
  } else if (IssueKind::kTypeResolverErrorStart < kind() &&
             kind() < IssueKind::kTypeResolverErrorEnd) {
    return Severity::kError;
  } else if (IssueKind::kPackageManagerWarningStart < kind() &&
             kind() < IssueKind::kPackageManagerWarningEnd) {
    return Severity::kWarning;
  } else if (IssueKind::kPackageManagerErrorStart < kind() &&
             kind() < IssueKind::kPackageManagerErrorEnd) {
    return Severity::kError;
  } else {
    fail("unexpected issue kind");
  }
}

}  // namespace issues
}  // namespace lang
