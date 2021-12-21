//
//  util.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_util_h
#define cmd_util_h

#include <string>
#include <vector>

namespace cmd {

std::vector<std::string> ConvertMainArgs(int argc, char* argv[]);

}  // namespace cmd

#endif /* cmd_util_h */
