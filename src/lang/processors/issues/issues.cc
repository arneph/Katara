//
//  issues.cpp
//  Katara
//
//  Created by Arne Philipeit on 10/24/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "issues.h"

namespace lang {
namespace issues {

Origin OriginOf(IssueKind issue_kind) {
  if (IssueKind::kParserStart < issue_kind && issue_kind < IssueKind::kParserEnd) {
    return Origin::kParser;
  } else if (IssueKind::kIdentifierResolverStart < issue_kind &&
             issue_kind < IssueKind::kIdentiferResolverEnd) {
    return Origin::kIdentifierResolver;
  } else if (IssueKind::kTypeResolverStart < issue_kind &&
             issue_kind < IssueKind::kTypeResolverEnd) {
    return Origin::kTypeResolver;
  } else if (IssueKind::kPackageManagerStart < issue_kind &&
             issue_kind < IssueKind::kPackageManagerEnd) {
    return Origin::kPackageManager;
  } else {
    throw "internal error: unexpected issue kind";
  }
}

Severity SeverityOf(IssueKind issue_kind) {
  if (IssueKind::kParserFatalStart < issue_kind && issue_kind < IssueKind::kParserFatalEnd) {
    return Severity::kFatal;
  } else if (IssueKind::kIdentifierResolverErrorStart < issue_kind &&
             issue_kind < IssueKind::kIdentifierResolverErrorEnd) {
    return Severity::kError;
  } else if (IssueKind::kTypeResolverErrorStart < issue_kind &&
             issue_kind < IssueKind::kTypeResolverErrorEnd) {
    return Severity::kError;
  } else if (IssueKind::kPackageManagerWarningStart < issue_kind &&
             issue_kind < IssueKind::kPackageManagerWarningEnd) {
    return Severity::kWarning;
  } else {
    throw "internal error: unexpected issue kind";
  }
}

bool IssueTracker::has_errors() const {
  for (auto& issue : issues_) {
    if (issue.severity() == issues::Severity::kError ||
        issue.severity() == issues::Severity::kFatal) {
      return true;
    }
  }
  return false;
}

bool IssueTracker::has_fatal_errors() const {
  for (auto& issue : issues_) {
    if (issue.severity() == issues::Severity::kFatal) {
      return true;
    }
  }
  return false;
}

void IssueTracker::Add(IssueKind kind, pos::pos_t position, std::string message) {
  Add(kind, std::vector<pos::pos_t>{position}, message);
}

void IssueTracker::Add(IssueKind kind, std::vector<pos::pos_t> positions, std::string message) {
  issues_.push_back(Issue(kind, positions, message));
}

}  // namespace issues
}  // namespace lang
