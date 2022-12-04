//
//  scanner.h
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_scanner_h
#define ir_serialization_scanner_h

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "src/common/atomics/atomics.h"
#include "src/common/positions/positions.h"
#include "src/ir/issues/issues.h"

namespace ir_serialization {

class Scanner {
 public:
  typedef enum : char {
    kUnknown = 0,
    kIdentifier = 1,
    kNumber = 2,
    kAddress = 3,
    kString = 4,
    kArrow = 5,
    kEoF = EOF,
    kNewLine = '\n',
    kHashSign = '#',
    kPercentSign = '%',
    kColon = ':',
    kCurlyBracketOpen = '{',
    kCurlyBracketClose = '}',
    kAtSign = '@',
    kComma = ',',
    kEqualSign = '=',
    kParenOpen = '(',
    kParenClose = ')',
    kAngleOpen = '<',
    kAngleClose = '>',
  } Token;

  static std::string TokenToString(Token token);

  Scanner(common::PosFile* file, ir_issues::IssueTracker& issue_tracker)
      : file_(file), issue_tracker_(issue_tracker), pos_(file->start()) {}

  Token token() const { return token_; }
  common::pos_t token_start() const { return token_start_; }
  common::pos_t token_end() const { return token_end_; }
  std::string token_text() const;
  common::Int token_number() const;
  common::Int token_address() const;
  std::string token_string() const;

  void Next();
  void NextIfPossible();

  std::optional<int64_t> ConsumeInt64();
  std::optional<std::string> ConsumeIdentifier();
  bool ConsumeToken(Token token);

  void AddErrorForUnexpectedToken(std::vector<Token> expected_tokens);
  void SkipPastTokenSequence(std::vector<Token> sequence);

 private:
  void SkipWhitespace();
  void NextIdentifier();
  void NextNumberOrAddress();
  void NextString();

  const common::PosFile* file_;
  ir_issues::IssueTracker& issue_tracker_;

  common::pos_t pos_;

  Token token_ = kUnknown;
  common::pos_t token_start_ = common::kNoPos;
  common::pos_t token_end_ = common::kNoPos;
};

}  // namespace ir_serialization

#endif /* ir_serialization_scanner_h */
