//
//  error_codes.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef katara_error_codes_h
#define katara_error_codes_h

namespace cmd {

enum ErrorCode : int {
  kNoError,
  kLoadErrorMixedSourceFileArgsWithPackagePathArgs,
  kLoadErrorMultiplePackagePathArgs,
  kLoadErrorForPackage,
  kBuildErrorNoMainPackage,
  kBuildErrorTranslationToIRProgramFailed,
};

}

#endif /* katara_error_codes_h */
