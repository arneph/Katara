//
//  scanner.h
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_scanner_h
#define ir_serialization_scanner_h

#include <istream>
#include <memory>
#include <string>
#include <vector>

#include "src/common/atomics/atomics.h"

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

  Scanner(std::istream& in_stream) : in_stream_(in_stream) {}

  int64_t line() const { return line_; }
  int64_t column() const { return column_; }
  std::string PositionString() const;

  Token token() const { return token_; }
  std::string token_text() const;
  common::Int token_number() const;
  common::Int token_address() const;
  std::string token_string() const;

  void Next();

  int64_t ConsumeInt64();
  std::string ConsumeIdentifier();
  void ConsumeToken(Token token);

  void FailForUnexpectedToken(std::vector<Token> expected_tokens);

 private:
  void SkipWhitespace();
  void NextIdentifier();
  void NextNumberOrAddress();
  void NextString();

  std::istream& in_stream_;

  int64_t line_ = 1;
  int64_t column_ = 1;
  Token token_ = kUnknown;
  std::string token_text_;
};

}  // namespace ir_serialization

#endif /* ir_serialization_scanner_h */
