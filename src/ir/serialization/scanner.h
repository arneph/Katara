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

#include "src/common/atomics/atomics.h"

namespace ir_serialization {

class Scanner {
 public:
  typedef enum : char {
    kUnknown = 0,
    kIdentifier = 1,
    kNumber = 2,
    kAddress = 3,
    kArrow = 4,
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
    kParenClose = ')'
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

  void Next();

 private:
  void SkipWhitespace();
  void NextIdentifier();
  void NextNumberOrAddress();

  std::istream& in_stream_;

  int64_t line_ = 1;
  int64_t column_ = 1;
  Token token_ = kUnknown;
  std::string token_text_;
};

}  // namespace ir_serialization

#endif /* ir_serialization_scanner_h */
