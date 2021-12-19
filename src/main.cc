//
//  main.cc
//  Katara
//
//  Created by Arne Philipeit on 11/22/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "src/cmd/cmd.h"

int main(int argc, char* argv[]) {
  return cmd::Execute(argc, argv, std::cin, std::cout, std::cerr);
}
