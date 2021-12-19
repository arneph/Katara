//
//  util.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_util_h
#define cmd_util_h

#include <filesystem>
#include <string>

namespace cmd {

void WriteToFile(std::string text, std::filesystem::path out_file);

}

#endif /* cmd_util_h */
