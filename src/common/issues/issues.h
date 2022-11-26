//
//  issues.h
//  Katara
//
//  Created by Arne Philipeit on 10/24/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef common_issues_h
#define common_issues_h

#include <algorithm>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "src/common/positions/positions.h"

namespace common {

template <typename T>
concept IssueKindType = std::is_enum<T>::value;

template <typename T>
concept OriginType = std::is_enum<T>::value;

enum class Severity {
  kWarning,  // can still complete
  kError,    // can partially continue but not complete
  kFatal,    // cannot continue
};

template <IssueKindType IssueKind, OriginType Origin>
class Issue {
 public:
  Issue(IssueKind kind, std::vector<pos_t> positions, std::string message)
      : kind_(kind), positions_(positions), message_(message) {}
  virtual ~Issue() = default;

  int64_t kind_id() const { return static_cast<int64_t>(kind_); }
  IssueKind kind() const { return kind_; }
  virtual Origin origin() const = 0;
  virtual Severity severity() const = 0;
  const std::vector<pos_t>& positions() const { return positions_; }
  std::string message() const { return message_; }

 private:
  IssueKind kind_;
  std::vector<pos_t> positions_;
  std::string message_;
};

template <class T, typename IssueKind, typename Origin>
concept IssueSubclass = std::is_base_of<Issue<IssueKind, Origin>, T>::value;

template <IssueKindType IssueKind, OriginType Origin, IssueSubclass<IssueKind, Origin> Issue>
class IssueTracker {
 public:
  enum class PrintFormat {
    kPlain,
    kTerminal,
  };

  IssueTracker(const PosFileSet* file_set) : file_set_(file_set) {}

  bool has_warnings() const {
    return std::any_of(issues_.begin(), issues_.end(),
                       [](auto& issue) { return issue.severity() == Severity::kWarning; });
  }
  bool has_errors() const {
    return std::any_of(issues_.begin(), issues_.end(), [](auto& issue) {
      return issue.severity() == Severity::kError || issue.severity() == Severity::kFatal;
    });
  }
  bool has_fatal_errors() const {
    return std::any_of(issues_.begin(), issues_.end(),
                       [](auto& issue) { return issue.severity() == Severity::kFatal; });
  }

  const std::vector<Issue>& issues() const { return issues_; }

  void Add(IssueKind kind, pos_t position, std::string message) {
    Add(kind, std::vector<pos_t>{position}, message);
  }
  void Add(IssueKind kind, std::vector<pos_t> positions, std::string message) {
    issues_.push_back(Issue(kind, positions, message));
  }

  void PrintIssues(PrintFormat format, std::ostream* out) const {
    for (auto& issue : issues_) {
      switch (format) {
        case PrintFormat::kPlain:
          switch (issue.severity()) {
            case Severity::kWarning:
              *out << "Warning:";
              break;
            case Severity::kError:
            case Severity::kFatal:
              *out << "Error:";
          }
          break;
        case PrintFormat::kTerminal:
          switch (issue.severity()) {
            case Severity::kWarning:
              *out << "\033[93;1m"
                      "Warning:"
                      "\033[0;0m"
                      " ";
              break;
            case Severity::kError:
            case Severity::kFatal:
              *out << "\033[91;1m"
                      "Error:"
                      "\033[0;0m"
                      " ";
              break;
          }
          break;
      }
      *out << issue.message() << " [" << issue.kind_id() << "]\n";
      for (pos_t pos : issue.positions()) {
        Position position = file_set_->PositionFor(pos);
        std::string line = file_set_->FileAt(pos)->LineFor(pos);
        size_t whitespace = 0;
        for (; whitespace < line.length(); whitespace++) {
          if (line.at(whitespace) != ' ' && line.at(whitespace) != '\t') {
            break;
          }
        }
        *out << "  " << position.ToString() << ": ";
        *out << line.substr(whitespace);
        size_t pointer = 4 + position.ToString().size() + position.column_ - whitespace;
        for (size_t i = 0; i < pointer; i++) {
          *out << " ";
        }
        *out << "^\n";
      }
    }
  }

 private:
  const PosFileSet* file_set_;
  std::vector<Issue> issues_;
};

}  // namespace common

#endif /* lang_issues_h */
