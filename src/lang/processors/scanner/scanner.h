//
//  scanner.h
//  Katara
//
//  Created by Arne Philipeit on 5/24/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_scanner_h
#define lang_scanner_h

#include <string>

#include "lang/representation/positions/positions.h"
#include "lang/representation/tokens/tokens.h"

namespace lang {
namespace scanner {

class Scanner {
 public:
  Scanner(pos::File* file);

  tokens::Token token() const;
  pos::pos_t token_start() const;
  pos::pos_t token_end() const;
  std::string token_string() const;

  void Next(bool split_shift_ops = false);
  void SkipPastLine();

 private:
  const pos::File* file_;

  pos::pos_t pos_;

  tokens::Token tok_;
  pos::pos_t tok_start_;
  pos::pos_t tok_end_;

  void NextArithmeticOrBitOpStart(tokens::Token tok);
};

}  // namespace scanner
}  // namespace lang

#endif /* lang_scanner_h */
