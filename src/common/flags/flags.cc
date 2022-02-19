//
//  flags.cc
//  Katara
//
//  Created by Arne Philipeit on 1/30/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "flags.h"

#include <iomanip>

namespace common {

AbstractFlag* FlagSet::FlagWithName(std::string name) const {
  auto it = flag_lookup_.find(name);
  if (it != flag_lookup_.end()) {
    return it->second;
  } else if (parent_ != nullptr) {
    return parent_->FlagWithName(name);
  }
  return nullptr;
}

bool FlagSet::Set(std::string flag_name, std::string value) const {
  AbstractFlag* flag = FlagWithName(flag_name);
  if (flag == nullptr) {
    return false;
  }
  return flag->SetCurrentValueString(value);
}

bool FlagSet::Parse(std::vector<std::string>& args, std::ostream* error_stream) {
  bool successful = true;
  for (auto it = args.begin(); it != args.end();) {
    std::string original_arg = *it;
    std::string current_arg = original_arg;
    if (current_arg == "--") {
      args.erase(it);
      break;
    }
    if (current_arg.starts_with("--")) {
      current_arg = current_arg.substr(/*pos=*/2);
    } else if (current_arg.starts_with("-")) {
      current_arg = current_arg.substr(/*pos=*/1);
    } else {
      ++it;
      continue;
    }
    it = args.erase(it);
    std::string flag_name;
    std::string flag_value;
    AbstractFlag* flag;
    std::string::size_type equal_pos = current_arg.find('=');
    if (equal_pos != std::string_view::npos) {
      flag_name = current_arg.substr(/*pos=*/0, /*count=*/equal_pos);
      flag_value = current_arg.substr(/*pos=*/equal_pos + 1);
      flag = FlagWithName(flag_name);
    } else {
      flag_name = current_arg;
      flag = FlagWithName(flag_name);
      if (flag != nullptr && flag->IsBoolFlag()) {
        flag_value = "t";
      } else if (it != args.end()) {
        flag_value = *it;
        it = args.erase(it);
      }
    }
    if (flag_name.empty()) {
      *error_stream << "missing flag name: " << original_arg << "\n";
      successful = false;
    } else if (flag == nullptr) {
      *error_stream << "flag -" << flag_name << " does not exist\n";
      successful = false;
    } else if (!flag->SetCurrentValueString(flag_value)) {
      *error_stream << "flag -" << flag_name << " does not accept value " << std::quoted(flag_value)
                    << "\n";
      successful = false;
    }
  }
  return successful;
}

void FlagSet::PrintDefaults(std::ostream* output_stream) const {
  std::map<std::string, AbstractFlag*> flags_to_print = flag_lookup_;
  FlagSet* ancestor = parent_;
  while (ancestor != nullptr) {
    flags_to_print.insert(ancestor->flag_lookup_.begin(), ancestor->flag_lookup_.end());
    ancestor = ancestor->parent_;
  }

  for (auto [name, flag] : flags_to_print) {
    *output_stream << "  -" << flag->name();
    if (flag->name().size() > 1) {
      *output_stream << "\n      ";
    } else {
      *output_stream << "  ";
    }
    for (char c : flag->usage()) {
      *output_stream << c;
      if (c == '\n') {
        *output_stream << "      ";
      }
    }
    if (!flag->IsDefaultValueZeroValue()) {
      *output_stream << " (default " << flag->DefaultValueString() << ")";
    }
    *output_stream << "\n";
  }
}

}  // namespace common
