//
//  error_codes.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_error_codes_h
#define cmd_error_codes_h

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

#endif /* cmd_error_codes_h */
