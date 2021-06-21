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
  } else if (IssueKind::kPackageManagerErrorStart < issue_kind &&
             issue_kind < IssueKind::kPackageManagerErrorEnd) {
    return Severity::kError;
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

void IssueTracker::PrintIssues(PrintFormat format, std::ostream& out) const {
  for (auto& issue : issues_) {
    switch (format) {
      case PrintFormat::kPlain:
        switch (issue.severity()) {
          case Severity::kWarning:
            out << "Warning:";
            break;
          case Severity::kError:
          case Severity::kFatal:
            out << "Error:";
        }
        break;
      case PrintFormat::kTerminal:
        switch (issue.severity()) {
          case lang::issues::Severity::kWarning:
            out << "\033[93;1m"
                   "Warning:"
                   "\033[0;0m"
                   " ";
            break;
          case lang::issues::Severity::kError:
          case lang::issues::Severity::kFatal:
            out << "\033[91;1m"
                   "Error:"
                   "\033[0;0m"
                   " ";
            break;
        }
        break;
    }
    out << issue.message() << " [" << issue.kind_id() << "]\n";
    for (lang::pos::pos_t pos : issue.positions()) {
      lang::pos::Position position = file_set_->PositionFor(pos);
      std::string line = file_set_->FileAt(pos)->LineFor(pos);
      size_t whitespace = 0;
      for (; whitespace < line.length(); whitespace++) {
        if (line.at(whitespace) != ' ' && line.at(whitespace) != '\t') {
          break;
        }
      }
      std::cout << "  " << position.ToString() << ": ";
      std::cout << line.substr(whitespace);
      size_t pointer = 4 + position.ToString().size() + position.column_ - whitespace;
      for (size_t i = 0; i < pointer; i++) {
        out << " ";
      }
      out << "^\n";
    }
  }
}

}  // namespace issues
}  // namespace lang
