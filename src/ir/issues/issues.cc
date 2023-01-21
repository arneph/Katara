//
//  issues.cc
//  Katara
//
//  Created by Arne Philipeit on 11/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "issues.h"

#include "src/common/logging/logging.h"

namespace ir_issues {

using ::common::issues::Severity;
using ::common::logging::fail;

Origin Issue::origin() const {
  if (IssueKind::kScannerStart < kind() && kind() < IssueKind::kScannerEnd) {
    return Origin::kScanner;
  } else if (IssueKind::kParserStart < kind() && kind() < IssueKind::kParserEnd) {
    return Origin::kParser;
  } else if (IssueKind::kCheckerStart < kind() && kind() < IssueKind::kCheckerEnd) {
    return Origin::kChecker;
  } else {
    fail("unexpected issue kind");
  }
}

Severity Issue::severity() const {
  if (IssueKind::kScannerStart < kind() && kind() < IssueKind::kScannerEnd) {
    return Severity::kFatal;
  } else if (IssueKind::kParserStart < kind() && kind() < IssueKind::kParserEnd) {
    return Severity::kError;
  } else if (IssueKind::kCheckerStart < kind() && kind() < IssueKind::kCheckerEnd) {
    return Severity::kError;
  } else {
    fail("unexpected issue kind");
  }
}

}  // namespace ir_issues
