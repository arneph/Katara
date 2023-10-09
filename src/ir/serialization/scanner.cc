//
//  scanner.cc
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "scanner.h"

#include <cctype>

#include "src/common/logging/logging.h"

namespace ir_serialization {

using ::common::atomics::Int;
using ::common::logging::fail;
using ::common::positions::pos_t;
using ::common::positions::range_t;

std::string Scanner::TokenToString(Token token) {
  switch (token) {
    case kUnknown:
      return "unknown";
    case kIdentifier:
      return "identifier";
    case kNumber:
      return "number";
    case kAddress:
      return "address";
    case kArrow:
      return "'=>'";
    case kEoF:
      return "end of file";
    case kNewLine:
      return "new line";
    default:
      return "'" + std::string(1, token) + "'";
  }
}

std::string Scanner::token_text() const {
  if (token_ == kUnknown || token_ == kEoF) {
    fail("token has no associated text");
  }
  return file_->contents(token_range_);
}

Int Scanner::token_number() const {
  if (token_ != kNumber) {
    fail("token has no associated number");
  }
  std::optional<Int> number = common::atomics::ToI64(token_text());
  if (number.has_value()) {
    return *number;
  }
  number = common::atomics::ToU64(token_text());
  if (!number.has_value()) {
    issue_tracker_.Add(ir_issues::IssueKind::kNumberCannotBeRepresented, token_range_,
                       "The token cannot be represented as a number");
    return Int(uint8_t{0});
  } else {
    return *number;
  }
}

Int Scanner::token_address() const {
  if (token_ != kAddress) {
    fail("token has no associated address");
  }
  std::optional<Int> address = common::atomics::ToU64(token_text(), /*base=*/16);
  if (!address.has_value()) {
    issue_tracker_.Add(ir_issues::IssueKind::kAddressCannotBeRepresented, token_range_,
                       "The token cannot be represented as an address");
    return Int(uint8_t{0});
  } else {
    return *address;
  }
}

std::string Scanner::token_string() const {
  if (token_ != kString) {
    fail("token has no associated string");
  }
  std::string str;
  for (std::size_t i = 1; i < token_text().length() - 1; i++) {
    char c = token_text().at(i);
    if (c == '\\') {
      c = token_text().at(++i);
    }
    str += std::string(1, c);
  }
  return str;
}

void Scanner::Next() {
  if (token_ == kEoF) {
    fail("can not advance Scanner at EoF");
  }
  NextIfPossible();
}

void Scanner::NextIfPossible() {
  if (token_ == kEoF) {
    return;
  }
  SkipWhitespace();

  pos_t token_start = pos_;
  if (pos_ > file_->end()) {
    token_ = kEoF;
    token_range_ = range_t{.start = token_start, .end = pos_};
    return;
  }

  char c = file_->at(pos_);
  switch (c) {
    case EOF:
      token_ = kEoF;
      token_range_ = range_t{.start = token_start, .end = pos_};
      return;
    case '\n':
    case '#':
    case '%':
    case ':':
    case '{':
    case '}':
    case '@':
    case ',':
    case '(':
    case ')':
    case '<':
    case '>':
      token_ = (Token)c;
      token_range_ = range_t{.start = token_start, .end = pos_++};
      return;
    case '=':
      token_ = kEqualSign;
      token_range_ = range_t{.start = token_start, .end = pos_++};
      if (pos_ < file_->end() && file_->at(pos_) == '>') {
        token_ = kArrow;
        token_range_ = range_t{.start = token_start, .end = pos_++};
      }
      return;
    case '"':
      NextString();
      return;
    default:
      break;
  }
  if (std::isalpha(c)) {
    NextIdentifier();
  } else if (c == '+' || c == '-' || std::isdigit(c)) {
    NextNumberOrAddress();
  } else {
    token_ = kUnknown;
    token_range_ = range_t{.start = token_start, .end = pos_++};
  }
}

void Scanner::SkipWhitespace() {
  for (; pos_ < file_->end() && file_->at(pos_) != '\n' && std::isspace(file_->at(pos_)); pos_++) {
  }
}

void Scanner::NextIdentifier() {
  token_ = kIdentifier;
  pos_t token_start = pos_;
  pos_t token_end = pos_++;
  for (; pos_ <= file_->end() && (std::isalnum(file_->at(pos_)) || file_->at(pos_) == '_');
       token_end = pos_++) {
  }
  token_range_ = range_t{.start = token_start, .end = token_end};
}

void Scanner::NextNumberOrAddress() {
  token_ = kNumber;
  pos_t token_start = pos_;
  pos_t token_end = pos_++;
  for (; pos_ <= file_->end() && std::isalnum(file_->at(pos_)); token_end = pos_++) {
  }
  token_range_ = range_t{.start = token_start, .end = token_end};
  if (token_text().starts_with("0x") || token_text().starts_with("0X")) {
    token_ = kAddress;
  }
}

void Scanner::NextString() {
  token_ = kString;
  pos_t token_start = pos_++;
  for (; pos_ <= file_->end() && file_->at(pos_) != '"'; pos_++) {
    if (file_->at(pos_) != '\\') {
      continue;
    }
    if (pos_ + 1 < file_->end()) {
      pos_++;
    } else {
      issue_tracker_.Add(ir_issues::IssueKind::kEOFInsteadOfEscapedCharacter, pos_,
                         "Expected escape at end of file.");
      token_ = kUnknown;
      token_range_ = range_t{.start = token_start, .end = pos_++};
      return;
    }
  }
  if (pos_ > file_->end()) {
    token_range_ = range_t{.start = token_start, .end = pos_};
    issue_tracker_.Add(ir_issues::IssueKind::kEOFInsteadOfStringEndQuote, token_range_,
                       "String constant has no end quote.");
    token_ = kUnknown;
  } else {
    token_range_ = range_t{.start = token_start, .end = pos_++};
  }
}

std::optional<int64_t> Scanner::ConsumeInt64() {
  if (token_ != Scanner::kNumber) {
    AddErrorForUnexpectedToken({kNumber});
    NextIfPossible();
    return std::nullopt;
  }
  int64_t number = token_number().AsInt64();
  Next();
  return number;
}

std::optional<std::string> Scanner::ConsumeIdentifier() {
  if (token_ != Scanner::kIdentifier) {
    AddErrorForUnexpectedToken({kIdentifier});
    NextIfPossible();
    return std::nullopt;
  }
  std::string identifier = token_text();
  Next();
  return identifier;
}

bool Scanner::ConsumeToken(Scanner::Token token) {
  if (token_ != token) {
    AddErrorForUnexpectedToken({token});
    NextIfPossible();
    return false;
  }
  Next();
  return true;
}

void Scanner::AddErrorForUnexpectedToken(std::vector<Token> expected_tokens) {
  std::string error = "expected ";
  for (std::size_t i = 0; i < expected_tokens.size(); i++) {
    Scanner::Token expected_token = expected_tokens.at(i);
    if (i > 0) {
      error += ", ";
      if (i == expected_tokens.size() - 1) {
        error += "or ";
      }
    }
    error += Scanner::TokenToString(expected_token);
  }
  error += "; got ";
  switch (token()) {
    case kUnknown:
    case kNewLine:
    case kEoF:
      error += Scanner::TokenToString(token());
      break;
    default:
      error += "'" + token_text() + "'";
      break;
  }
  issue_tracker_.Add(ir_issues::IssueKind::kUnexpectedToken, token_range_, error);
}

void Scanner::SkipPastTokenSequence(std::vector<Token> sequence) {
  while (token() != kEoF) {
  start:
    if (token() != sequence.at(0)) {
      Next();
      continue;
    }
    Next();
    for (std::size_t i = 1; i < sequence.size(); i++) {
      if (token() == kEoF) return;
      if (token() != sequence.at(i)) goto start;
      Next();
    }
    return;
  }
}

}  // namespace ir_serialization
