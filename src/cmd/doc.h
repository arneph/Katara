//
//  doc.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_doc_h
#define cmd_doc_h

#include <filesystem>
#include <vector>

#include "src/cmd/context/context.h"
#include "src/cmd/debug.h"
#include "src/cmd/error_codes.h"

namespace cmd {

ErrorCode Doc(std::vector<std::filesystem::path>& paths, DebugHandler& debug_handler, Context* ctx);

}

#endif /* cmd_doc_h */
