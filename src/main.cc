//
//  main.cc
//  Katara
//
//  Created by Arne Philipeit on 11/22/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "src/cmd/cmd.h"
#include "src/cmd/context.h"
#include "src/cmd/util.h"

int main(int argc, char* argv[]) {
  cmd::RealContext ctx(cmd::ConvertMainArgs(argc, argv));
  return cmd::Execute(&ctx);
}
