//
//  scanner.cc
//  Katara
//
//  Created by Arne Philipeit on 5/24/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "scanner.h"

namespace lang {
namespace scanner {

Scanner::Scanner(std::string raw) : raw_(raw), pos_(0) {
    Next();
}

token::Token Scanner::token() const {
    return tok_;
}

int64_t Scanner::token_start() const {
    return tok_start_;
}

int64_t Scanner::token_end() const {
    return tok_end_;
}

std::string Scanner::token_string() const {
    return raw_.substr(tok_start_, tok_end_-tok_start_+1);
}

void Scanner::Next(bool split_shift_ops) {
    bool insert_semicolon = false;
    if (pos_ > 0) {
        switch (tok_) {
            case token::kIdent:
            case token::kInt:
            case token::kFallthrough:
            case token::kContinue:
            case token::kBreak:
            case token::kReturn:
            case token::kInc:
            case token::kDec:
            case token::kGtr:
            case token::kRParen:
            case token::kRBrack:
            case token::kRBrace:
                insert_semicolon = true;
            default:;
        }
    }
    for (; pos_ < raw_.size() &&
         (raw_.at(pos_) == ' ' ||
          raw_.at(pos_) == '\t' ||
          (raw_.at(pos_) == '\n' && !insert_semicolon));
         pos_++);
    tok_start_ = pos_;
    if (pos_ == raw_.size()) {
        tok_ = token::kEOF;
        tok_end_ = pos_;
        return;
    }
    
    switch (raw_.at(pos_++)) {
        case '\n':
            tok_ = token::kSemicolon;
            tok_end_ = pos_ - 1;
            return;
        case '+':
            if (pos_ < raw_.size() &&
                raw_.at(pos_) == '+') {
                tok_ = token::kInc;
                tok_end_ = pos_++;
                return;
            }
            NextArithmeticOrBitOpStart(token::kAdd);
            return;
        case '-':
            if (pos_ < raw_.size() &&
                raw_.at(pos_) == '-') {
                tok_ = token::kDec;
                tok_end_ = pos_++;
                return;
            }
            NextArithmeticOrBitOpStart(token::kSub);
            return;
        case '*':
            NextArithmeticOrBitOpStart(token::kMul);
            return;
        case '/':
            if (pos_ < raw_.size() &&
                raw_.at(pos_) == '/') {
                for (; pos_ < raw_.size() && raw_.at(pos_) != '\n';
                     pos_++);
                tok_ = token::kComment;
                tok_end_ = pos_ - 1;
                return;
            } else if (pos_ < raw_.size() &&
                       raw_.at(pos_) == '*') {
                for (; pos_ < raw_.size() - 1 &&
                     (raw_.at(pos_) != '*' || raw_.at(pos_ + 1) != '/');
                     pos_++);
                tok_ = token::kComment;
                tok_end_ = (pos_ < raw_.size() - 1) ? pos_ + 1 : pos_;
                pos_ += 2;
                return;
            }
            NextArithmeticOrBitOpStart(token::kQuo);
            return;
        case '%':
            NextArithmeticOrBitOpStart(token::kRem);
            return;
        case '&':
            if (pos_ < raw_.size() &&
                raw_.at(pos_) == '&') {
                tok_ = token::kLAnd;
                tok_end_ = pos_++;
                return;
            } else if (pos_ < raw_.size() &&
                       raw_.at(pos_) == '^') {
                pos_++;
                NextArithmeticOrBitOpStart(token::kAndNot);
                return;
            }
            NextArithmeticOrBitOpStart(token::kAnd);
            return;
        case '|':
            if (pos_ < raw_.size() &&
                raw_.at(pos_) == '|') {
                tok_ = token::kLOr;
                tok_end_ = pos_++;
                return;
            }
            NextArithmeticOrBitOpStart(token::kOr);
            return;
        case '^':
            NextArithmeticOrBitOpStart(token::kXor);
            return;
        case '<':
            if (!split_shift_ops &&
                pos_ < raw_.size() &&
                raw_.at(pos_) == '<') {
                pos_++;
                NextArithmeticOrBitOpStart(token::kShl);
                return;
            } else if (pos_ < raw_.size() &&
                       raw_.at(pos_) == '=') {
                tok_ = token::kLeq;
                tok_end_ = pos_++;
                return;
            } else {
                tok_ = token::kLss;
                tok_end_ = pos_ - 1;
                return;
            }
        case '>':
            if (!split_shift_ops &&
                pos_ < raw_.size() &&
                raw_.at(pos_) == '>') {
                pos_++;
                NextArithmeticOrBitOpStart(token::kShr);
                return;
            } else if (pos_ < raw_.size() &&
                       raw_.at(pos_) == '=') {
                tok_ = token::kGeq;
                tok_end_ = pos_++;
                return;
            } else {
                tok_ = token::kGtr;
                tok_end_ = pos_ - 1;
                return;
            }
        case '=':
            if (pos_ < raw_.size() &&
                raw_.at(pos_) == '=') {
                tok_ = token::kEql;
                tok_end_ = pos_++;
                return;
            } else {
                tok_ = token::kAssign;
                tok_end_ = pos_ - 1;
                return;
            }
        case '!':
            if (pos_ < raw_.size() &&
                raw_.at(pos_) == '=') {
                tok_ = token::kNeq;
                tok_end_ = pos_++;
                return;
            } else {
                tok_ = token::kNot;
                tok_end_ = pos_ - 1;
                return;
            }
        case ':':
            if (pos_ < raw_.size() &&
                raw_.at(pos_) == '=') {
                tok_ = token::kDefine;
                tok_end_ = pos_++;
                return;
            } else {
                tok_ = token::kColon;
                tok_end_ = pos_ - 1;
                return;
            }
        case '(':
            tok_ = token::kLParen;
            tok_end_ = pos_ - 1;
            return;
        case '[':
            tok_ = token::kLBrack;
            tok_end_ = pos_ - 1;
            return;
        case '{':
            tok_ = token::kLBrace;
            tok_end_ = pos_ - 1;
            return;
        case ',':
            tok_ = token::kComma;
            tok_end_ = pos_ - 1;
            return;
        case '.':
            tok_ = token::kPeriod;
            tok_end_ = pos_ - 1;
            return;
        case ')':
            tok_ = token::kRParen;
            tok_end_ = pos_ - 1;
            return;
        case ']':
            tok_ = token::kRBrack;
            tok_end_ = pos_ - 1;
            return;
        case '}':
            tok_ = token::kRBrace;
            tok_end_ = pos_ - 1;
            return;
        case ';':
            tok_ = token::kSemicolon;
            tok_end_ = pos_ - 1;
            return;
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
            for (; pos_ < raw_.size() &&
                 '0' <= raw_.at(pos_) && raw_.at(pos_) <= '9';
                 pos_++);
            tok_ = token::kInt;
            tok_end_ = pos_ - 1;
            return;
    }
    
    for (; pos_ < raw_.size() &&
         (('A' <= raw_.at(pos_) && raw_.at(pos_) <= 'Z') ||
          ('a' <= raw_.at(pos_) && raw_.at(pos_) <= 'z') ||
          ('0' <= raw_.at(pos_) && raw_.at(pos_) <= '9') ||
          raw_.at(pos_) == '_');
         pos_++);
    tok_ = token::kIdent;
    tok_end_ = pos_ - 1;
    
    std::string ident = raw_.substr(tok_start_, tok_end_-tok_start_+1);
    if (ident == "const") {
        tok_ = token::kConst;
    } else if (ident == "var") {
        tok_ = token::kVar;
    } else if (ident == "type") {
        tok_ = token::kType;
    } else if (ident == "interface") {
        tok_ = token::kInterface;
    } else if (ident == "struct") {
        tok_ = token::kStruct;
    } else if (ident == "if") {
        tok_ = token::kIf;
    } else if (ident == "else") {
        tok_ = token::kElse;
    } else if (ident == "for") {
        tok_ = token::kFor;
    } else if (ident == "switch") {
        tok_ = token::kSwitch;
    } else if (ident == "case") {
        tok_ = token::kCase;
    } else if (ident == "default") {
        tok_ = token::kDefault;
    } else if (ident == "fallthrough") {
        tok_ = token::kElse;
    } else if (ident == "continue") {
        tok_ = token::kContinue;
    } else if (ident == "break") {
        tok_ = token::kBreak;
    } else if (ident == "return") {
        tok_ = token::kReturn;
    } else if (ident == "func") {
        tok_ = token::kFunc;
    }
}

void Scanner::NextArithmeticOrBitOpStart(token::Token tok) {
    if (pos_ < raw_.size() &&
        raw_.at(pos_) == '=') {
        tok_ = token::Token(tok + token::kAddAssign - token::kAdd);
        pos_++;
    } else {
        tok_ = tok;
    }
    tok_end_ = pos_ - 1;
}

void Scanner::SkipPastLine() {
    for (; pos_ < raw_.size() &&
         (raw_.at(pos_) != '\n');
         pos_++);
    Next();
}

}
}
