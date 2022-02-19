//
//  flags.h
//  Katara
//
//  Created by Arne Philipeit on 1/30/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef common_flags_flags_h
#define common_flags_flags_h

#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

#include "src/common/flags/flag_values.h"

namespace common {

class AbstractFlag {
 public:
  AbstractFlag(std::string name, std::string usage) : name_(name), usage_(usage) {}
  virtual ~AbstractFlag() = default;

  std::string name() const { return name_; }
  std::string usage() const { return usage_; }

  virtual std::string DefaultValueString() const = 0;
  virtual std::string CurrentValueString() const = 0;
  virtual bool SetCurrentValueString(std::string value_string) = 0;

  virtual bool IsDefaultValueZeroValue() const = 0;

  virtual bool IsBoolFlag() const = 0;

 private:
  std::string name_;
  std::string usage_;
};

template <typename ValueT>
class Flag : public AbstractFlag {
 public:
  Flag(std::string name, std::string usage, ValueT& current_value)
      : AbstractFlag(name, usage), default_value_(current_value), current_value_(current_value) {
    current_value_ = default_value_;
  }

  ValueT DefaultValue() const { return default_value_; }
  ValueT CurrentValue() const { return current_value_; }
  void SetCurrentValue(ValueT new_value) { *current_value_ = new_value; }

  std::string DefaultValueString() const override { return FlagValueToString(default_value_); }
  std::string CurrentValueString() const override { return FlagValueToString(current_value_); }
  bool SetCurrentValueString(std::string value_string) override {
    std::optional<ValueT> new_value = ParseFlagValue<ValueT>(value_string);
    if (new_value.has_value()) {
      current_value_ = *std::move(new_value);
      return true;
    }
    return false;
  }

  bool IsDefaultValueZeroValue() const override {
    return default_value_ == ZeroFlagValue<ValueT>();
  }

  bool IsBoolFlag() const override { return std::is_same_v<ValueT, bool>; }

 private:
  ValueT default_value_;
  ValueT& current_value_;
};

class FlagSet {
 public:
  FlagSet() {}

  FlagSet* parent() const { return parent_; }
  const std::vector<std::unique_ptr<AbstractFlag>>& flags() const { return flags_; }
  AbstractFlag* FlagWithName(std::string name) const;

  template <typename ValueT>
  void Add(std::string name, std::string usage, ValueT& current_value) {
    auto& flag = flags_.emplace_back(std::make_unique<Flag<ValueT>>(name, usage, current_value));
    flag_lookup_.insert({name, flag.get()});
  }

  bool Set(std::string flag_name, std::string value) const;
  bool Parse(std::vector<std::string>& args, std::ostream* error_stream);
  void PrintDefaults(std::ostream* output_stream) const;

  FlagSet CreateChild() { return FlagSet(this); }

 private:
  FlagSet(FlagSet* parent) : parent_(parent) {}

  FlagSet* parent_ = nullptr;
  std::vector<std::unique_ptr<AbstractFlag>> flags_;
  std::map<std::string, AbstractFlag*> flag_lookup_;
};

}  // namespace common

#endif /* common_flags_flag_set_h */
