//
//  run.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_run_h
#define cmd_run_h

#include "src/cmd/context.h"
#include "src/cmd/error_codes.h"

namespace cmd {

ErrorCode Run(Context* ctx);

}

#endif /* cmd_run_h */
