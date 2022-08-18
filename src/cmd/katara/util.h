//
//  util.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef katara_util_h
#define katara_util_h

#include <string>
#include <vector>

namespace cmd {

std::vector<std::string> ConvertMainArgs(int argc, char* argv[]);

}  // namespace cmd

#endif /* katara_util_h */
