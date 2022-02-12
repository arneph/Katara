//
//  flag_values.h
//  Katara
//
//  Created by Arne Philipeit on 2/3/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef common_flags_flag_values_h
#define common_flags_flag_values_h

#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>

namespace common {

template <typename ValueT>
std::optional<ValueT> ParseFlagValue(std::string value_string);

template <typename ValueT>
std::string FlagValueToString(ValueT value);

template <typename ValueT>
ValueT ZeroFlagValue();

template <>
inline std::optional<bool> ParseFlagValue<bool>(std::string value_string) {
  if (value_string == "0" || value_string == "f" || value_string == "F" ||
      value_string == "false" || value_string == "False" || value_string == "FALSE") {
    return false;
  } else if (value_string == "1" || value_string == "t" || value_string == "T" ||
             value_string == "true" || value_string == "True" || value_string == "TRUE") {
    return true;
  } else {
    return std::nullopt;
  }
}

template <>
inline std::string FlagValueToString<bool>(bool value) {
  return value ? "true" : "false";
}

template <>
inline bool ZeroFlagValue<bool>() {
  return false;
}

template <>
inline std::optional<int64_t> ParseFlagValue<int64_t>(std::string value_string) {
  if (value_string.empty()) {
    return std::nullopt;
  }
  const char* str_start = value_string.data();
  char* str_end = nullptr;
  int64_t result = std::strtoll(str_start, &str_end, /*base=*/0);
  if (str_start + value_string.length() != str_end) {
    return std::nullopt;
  }
  return result;
}

template <>
inline std::string FlagValueToString<int64_t>(int64_t value) {
  return std::to_string(value);
}

template <>
inline int64_t ZeroFlagValue<int64_t>() {
  return 0;
}

template <>
inline std::optional<std::string> ParseFlagValue<std::string>(std::string value_string) {
  return value_string;
}

template <>
inline std::string FlagValueToString<std::string>(std::string value) {
  return value;
}

template <>
inline std::string ZeroFlagValue<std::string>() {
  return "";
}

}  // namespace common

#endif /* common_flags_flag_values_h */
