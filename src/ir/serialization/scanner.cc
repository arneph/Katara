//
//  scanner.cc
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "scanner.h"

#include <cctype>

#include "src/common/logging.h"

namespace ir_serialization {

Scanner::Scanner(std::istream& in_stream) : in_stream_(in_stream) { token_ = kUnknown; }

Scanner::Token Scanner::token() const { return token_; }

std::string Scanner::string() const {
  if (token_ == kUnknown || token_ == kEoF) common::fail("token has no associated string");

  return string_;
}

int64_t Scanner::sign() const {
  if (token_ != kNumber) common::fail("token has no associated sign");

  return sign_;
}

uint64_t Scanner::number() const {
  if (token_ != kNumber) common::fail("token has no associated number");

  return number_;
}

void Scanner::Next() {
  if (token_ == kEoF) common::fail("can not advance Scanner at EoF");

  int c = in_stream_.get();
  while (c == ' ' || c == '\t') {
    c = in_stream_.get();
  }

  switch (c) {
    case EOF:
      token_ = kEoF;
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
      token_ = (Token)c;
      string_ = std::string(1, c);
      return;
    case '=':
      token_ = kEqualSign;
      string_ = "=";
      if (in_stream_.peek() == '>') {
        in_stream_.get();
        token_ = kArrow;
        string_ = "=>";
      }
      return;
    default:
      break;
  }

  string_ = std::string(1, c);
  if (std::isalpha(c)) {
    token_ = kIdentifier;

    c = in_stream_.get();
    while (std::isalnum(c) || c == '_') {
      string_ += std::string(1, c);
      c = in_stream_.get();
    }
    in_stream_.putback(c);
  } else if (c == '+' || c == '-' || std::isdigit(c)) {
    token_ = kNumber;
    if (c == '+') {
      sign_ = +1;
      number_ = 0;
    } else if (c == '-') {
      sign_ = -1;
      number_ = 0;
    } else {
      sign_ = +1;
      number_ = c - '0';
    }

    c = in_stream_.get();
    while (std::isdigit(c)) {
      string_ += std::string(1, c);
      number_ *= 10;
      number_ += c - '0';
      c = in_stream_.get();
    }
    in_stream_.putback(c);
  } else {
    token_ = kUnknown;
  }
}

}  // namespace ir_serialization
