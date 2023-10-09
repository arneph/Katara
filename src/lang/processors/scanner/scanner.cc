//
//  scanner.cc
//  Katara
//
//  Created by Arne Philipeit on 5/24/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "scanner.h"

#include "src/common/positions/positions.h"

namespace lang {
namespace scanner {

using ::common::positions::pos_t;
using ::common::positions::range_t;

Scanner::Scanner(common::positions::File* file)
    : file_(file), pos_(file->start()), tok_(tokens::kIllegal) {
  Next();
}

std::string Scanner::token_string() const { return file_->contents(tok_range_); }

void Scanner::Next(bool split_shift_ops) {
  bool insert_semicolon = false;
  switch (tok_) {
    case tokens::kIdent:
    case tokens::kInt:
    case tokens::kChar:
    case tokens::kString:
    case tokens::kFallthrough:
    case tokens::kContinue:
    case tokens::kBreak:
    case tokens::kReturn:
    case tokens::kInc:
    case tokens::kDec:
    case tokens::kGtr:
    case tokens::kRParen:
    case tokens::kRBrack:
    case tokens::kRBrace:
      insert_semicolon = true;
    default:;
  }
  for (; pos_ <= file_->end() && (file_->at(pos_) == ' ' || file_->at(pos_) == '\t' ||
                                  (file_->at(pos_) == '\n' && !insert_semicolon));
       pos_++) {
  }
  pos_t tok_start = pos_;
  if (pos_ > file_->end()) {
    tok_ = tokens::kEOF;
    tok_range_ = range_t{.start = tok_start, .end = pos_};
    return;
  }

  switch (file_->at(pos_++)) {
    case '\n':
      tok_ = tokens::kSemicolon;
      tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
      return;
    case '+':
      if (pos_ <= file_->end() && file_->at(pos_) == '+') {
        tok_ = tokens::kInc;
        tok_range_ = range_t{.start = tok_start, .end = pos_++};
        return;
      }
      NextArithmeticOrBitOpStart(tokens::kAdd, tok_start);
      return;
    case '-':
      if (pos_ <= file_->end() && file_->at(pos_) == '-') {
        tok_ = tokens::kDec;
        tok_range_ = range_t{.start = tok_start, .end = pos_++};
        return;
      }
      NextArithmeticOrBitOpStart(tokens::kSub, tok_start);
      return;
    case '*':
      NextArithmeticOrBitOpStart(tokens::kMul, tok_start);
      return;
    case '/':
      if (pos_ <= file_->end() && file_->at(pos_) == '/') {
        for (; pos_ <= file_->end() && file_->at(pos_) != '\n'; pos_++)
          ;
        tok_ = tokens::kComment;
        tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
        return;
      } else if (pos_ <= file_->end() && file_->at(pos_) == '*') {
        for (; pos_ <= file_->end() && (file_->at(pos_) != '*' || file_->at(pos_ + 1) != '/');
             pos_++)
          ;
        if (pos_ == file_->end()) {
          tok_ = tokens::kIllegal;
          tok_range_ = range_t{.start = tok_start, .end = file_->end()};
        } else {
          tok_ = tokens::kComment;
          tok_range_ = range_t{
              .start = tok_start,
              .end = (pos_ < file_->end()) ? pos_ + 1 : pos_,
          };
          pos_ += 2;
        }
        return;
      }
      NextArithmeticOrBitOpStart(tokens::kQuo, tok_start);
      return;
    case '%':
      NextArithmeticOrBitOpStart(tokens::kRem, tok_start);
      return;
    case '&':
      if (pos_ <= file_->end() && file_->at(pos_) == '&') {
        tok_ = tokens::kLAnd;
        tok_range_ = range_t{.start = tok_start, .end = pos_++};
        return;
      } else if (pos_ <= file_->end() && file_->at(pos_) == '^') {
        pos_++;
        NextArithmeticOrBitOpStart(tokens::kAndNot, tok_start);
        return;
      }
      NextArithmeticOrBitOpStart(tokens::kAnd, tok_start);
      return;
    case '|':
      if (pos_ <= file_->end() && file_->at(pos_) == '|') {
        tok_ = tokens::kLOr;
        tok_range_ = range_t{.start = tok_start, .end = pos_++};
        return;
      }
      NextArithmeticOrBitOpStart(tokens::kOr, tok_start);
      return;
    case '^':
      NextArithmeticOrBitOpStart(tokens::kXor, tok_start);
      return;
    case '<':
      if (!split_shift_ops && pos_ <= file_->end() && file_->at(pos_) == '<') {
        pos_++;
        NextArithmeticOrBitOpStart(tokens::kShl, tok_start);
        return;
      } else if (pos_ <= file_->end() && file_->at(pos_) == '=') {
        tok_ = tokens::kLeq;
        tok_range_ = range_t{.start = tok_start, .end = pos_++};
        return;
      } else {
        tok_ = tokens::kLss;
        tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
        return;
      }
    case '>':
      if (!split_shift_ops && pos_ <= file_->end() && file_->at(pos_) == '>') {
        pos_++;
        NextArithmeticOrBitOpStart(tokens::kShr, tok_start);
        return;
      } else if (pos_ <= file_->end() && file_->at(pos_) == '=') {
        tok_ = tokens::kGeq;
        tok_range_ = range_t{.start = tok_start, .end = pos_++};
        return;
      } else {
        tok_ = tokens::kGtr;
        tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
        return;
      }
    case '=':
      if (pos_ <= file_->end() && file_->at(pos_) == '=') {
        tok_ = tokens::kEql;
        tok_range_ = range_t{.start = tok_start, .end = pos_++};
        return;
      } else {
        tok_ = tokens::kAssign;
        tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
        return;
      }
    case '!':
      if (pos_ <= file_->end() && file_->at(pos_) == '=') {
        tok_ = tokens::kNeq;
        tok_range_ = range_t{.start = tok_start, .end = pos_++};
        return;
      } else {
        tok_ = tokens::kNot;
        tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
        return;
      }
    case ':':
      if (pos_ <= file_->end() && file_->at(pos_) == '=') {
        tok_ = tokens::kDefine;
        tok_range_ = range_t{.start = tok_start, .end = pos_++};
        return;
      } else {
        tok_ = tokens::kColon;
        tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
        return;
      }
    case '(':
      tok_ = tokens::kLParen;
      tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
      return;
    case '[':
      tok_ = tokens::kLBrack;
      tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
      return;
    case '{':
      tok_ = tokens::kLBrace;
      tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
      return;
    case ',':
      tok_ = tokens::kComma;
      tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
      return;
    case '.':
      tok_ = tokens::kPeriod;
      tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
      return;
    case ')':
      tok_ = tokens::kRParen;
      tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
      return;
    case ']':
      tok_ = tokens::kRBrack;
      tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
      return;
    case '}':
      tok_ = tokens::kRBrace;
      tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
      return;
    case ';':
      tok_ = tokens::kSemicolon;
      tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
      return;
    case '\'': {
      bool escaped = false;
      for (; pos_ <= file_->end() && !escaped && file_->at(pos_) != '\''; pos_++) {
        if (escaped) {
          escaped = false;
        } else if (file_->at(pos_) == '\\') {
          escaped = true;
        }
      }
      if (pos_ > file_->end()) {
        tok_ = tokens::kIllegal;
        tok_range_ = range_t{.start = tok_start, .end = file_->end()};
      } else {
        tok_ = tokens::kChar;
        tok_range_ = range_t{.start = tok_start, .end = pos_++};
      }
      return;
    }
    case '\"': {
      bool escaped = false;
      for (; pos_ <= file_->end() && !escaped && file_->at(pos_) != '\"'; pos_++) {
        if (escaped) {
          escaped = false;
        } else if (file_->at(pos_) == '\\') {
          escaped = true;
        }
      }
      if (pos_ > file_->end()) {
        tok_ = tokens::kIllegal;
        tok_range_ = range_t{.start = tok_start, .end = file_->end()};
      } else {
        tok_ = tokens::kString;
        tok_range_ = range_t{.start = tok_start, .end = pos_++};
      }
      return;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      for (; pos_ <= file_->end() && '0' <= file_->at(pos_) && file_->at(pos_) <= '9'; pos_++)
        ;
      tok_ = tokens::kInt;
      tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
      return;
  }

  for (; pos_ <= file_->end() &&
         (('A' <= file_->at(pos_) && file_->at(pos_) <= 'Z') ||
          ('a' <= file_->at(pos_) && file_->at(pos_) <= 'z') ||
          ('0' <= file_->at(pos_) && file_->at(pos_) <= '9') || file_->at(pos_) == '_');
       pos_++)
    ;
  tok_ = tokens::kIdent;
  tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};

  std::string ident = file_->contents(tok_range_);
  if (ident == "const") {
    tok_ = tokens::kConst;
  } else if (ident == "var") {
    tok_ = tokens::kVar;
  } else if (ident == "type") {
    tok_ = tokens::kType;
  } else if (ident == "interface") {
    tok_ = tokens::kInterface;
  } else if (ident == "struct") {
    tok_ = tokens::kStruct;
  } else if (ident == "if") {
    tok_ = tokens::kIf;
  } else if (ident == "else") {
    tok_ = tokens::kElse;
  } else if (ident == "for") {
    tok_ = tokens::kFor;
  } else if (ident == "switch") {
    tok_ = tokens::kSwitch;
  } else if (ident == "case") {
    tok_ = tokens::kCase;
  } else if (ident == "default") {
    tok_ = tokens::kDefault;
  } else if (ident == "fallthrough") {
    tok_ = tokens::kElse;
  } else if (ident == "continue") {
    tok_ = tokens::kContinue;
  } else if (ident == "break") {
    tok_ = tokens::kBreak;
  } else if (ident == "return") {
    tok_ = tokens::kReturn;
  } else if (ident == "func") {
    tok_ = tokens::kFunc;
  } else if (ident == "import") {
    tok_ = tokens::kImport;
  } else if (ident == "package") {
    tok_ = tokens::kPackage;
  }
}

void Scanner::NextArithmeticOrBitOpStart(tokens::Token tok, pos_t tok_start) {
  if (pos_ <= file_->end() && file_->at(pos_) == '=') {
    tok_ = tokens::Token(tok + tokens::kAddAssign - tokens::kAdd);
    pos_++;
  } else {
    tok_ = tok;
  }
  tok_range_ = range_t{.start = tok_start, .end = pos_ - 1};
}

void Scanner::SkipPastLine() {
  for (; pos_ <= file_->end() && (file_->at(pos_) != '\n'); pos_++)
    ;
  Next();
}

}  // namespace scanner
}  // namespace lang
