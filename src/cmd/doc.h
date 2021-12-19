//
//  doc.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_doc_h
#define cmd_doc_h

#include <ostream>
#include <string>
#include <vector>

#include "src/cmd/error_codes.h"

namespace cmd {

ErrorCode Doc(const std::vector<std::string> args, std::ostream& err);

}

#endif /* cmd_doc_h */
