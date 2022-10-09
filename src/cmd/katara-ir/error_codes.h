//
//  error_codes.h
//  Katara
//
//  Created by Arne Philipeit on 08/19/22.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef katara_ir_error_codes_h
#define katara_ir_error_codes_h

namespace cmd {
namespace katara_ir {

enum ErrorCode : int {
  kNoError,
  kMoreThanOneArgument,
  kParseFailed,
  kCheckFailed,
};

}
}  // namespace cmd

#endif /* katara_ir_error_codes_h */
