//
//  version.cc
//  Katara
//
//  Created by Arne Philipeit on 8/18/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "version.h"

namespace cmd {

constexpr std::string_view kVersion = "0.1";

void Version(Context* ctx) { *ctx->stdout() << "Katara version " << kVersion << "\n"; }

}  // namespace cmd
