//
//  cmd.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_cmd_h
#define cmd_cmd_h

#include <iostream>

namespace cmd {

int Execute(int argc, char* argv[], std::istream& in, std::ostream& out, std::ostream& err);

}

#endif /* cmd_cmd_h */
