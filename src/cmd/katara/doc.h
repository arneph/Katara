//
//  doc.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef katara_doc_h
#define katara_doc_h

#include <filesystem>
#include <vector>

#include "src/cmd/context.h"
#include "src/cmd/katara/debug.h"
#include "src/cmd/katara/error_codes.h"

namespace cmd {
namespace katara {

ErrorCode Doc(std::vector<std::filesystem::path>& paths, DebugHandler& debug_handler, Context* ctx);

}
}  // namespace cmd

#endif /* katara_doc_h */
