//
//  cmd.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_cmd_h
#define cmd_cmd_h

#include "src/cmd/context/context.h"
#include "src/cmd/error_codes.h"

namespace cmd {

ErrorCode Execute(Context* ctx);

}  // namespace cmd

#endif /* cmd_cmd_h */
