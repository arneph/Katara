//
//  flags_fuzz_test.cc
//  Katara
//
//  Created by Arne Philipeit on 2/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "src/common/flags/flags.h"

std::vector<std::string> InputToArgs(const uint8_t* data, size_t size) {
  std::string raw_args(reinterpret_cast<const char*>(data), size);
  std::vector<std::string> args;
  std::string current_arg;
  for (char c : raw_args) {
    if (c != ' ') {
      current_arg += c;
    } else {
      args.push_back(current_arg);
      current_arg.clear();
    }
  }
  return args;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  using namespace common;

  std::vector<std::string> args = InputToArgs(data, size);
  std::stringstream ss;

  bool bool_flag_a = false;
  bool bool_flag_b = true;
  int64_t int_flag_a = 1234;
  int64_t int_flag_b = 6789;
  std::string string_flag_a = "yo";
  std::string string_flag_b = "hey";

  FlagSet flags;
  flags.Add<bool>("a", "bool_flag_a usage", bool_flag_a);
  flags.Add<bool>("b", "bool_flag_b usage", bool_flag_b);
  flags.Add<int64_t>("c", "int_flag_a usage", int_flag_a);
  flags.Add<int64_t>("d", "int_flag_b usage", int_flag_b);
  flags.Add<std::string>("e", "string_flag_a usage", string_flag_a);
  flags.Add<std::string>("f", "string_flag_b usage", string_flag_b);
  flags.Parse(args, &ss);

  return 0;
}
