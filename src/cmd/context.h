//
//  context.h
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef cmd_context_h
#define cmd_context_h

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace cmd {

class Context {
 public:
  virtual ~Context() {}

  std::vector<std::string>& args() { return args_; }
  bool generate_debug_info() const { return generate_debug_info_; }
  std::filesystem::path debug_path() const { return debug_path_; }

  virtual std::istream& stdin() const = 0;
  virtual std::ostream& stdout() const = 0;
  virtual std::ostream& stderr() const = 0;

  virtual std::string ReadFromFile(std::filesystem::path in_file) const = 0;
  virtual void WriteToFile(std::string text, std::filesystem::path out_file) const = 0;
  virtual void CreateDirectory(std::filesystem::path path) const = 0;

  void CreateDebugSubDirectory(std::string subdir_name) const;
  void WriteToDebugFile(std::string text, std::string subdir_name, std::string file_name) const;

 protected:
  Context(std::vector<std::string> args);

 private:
  std::vector<std::string> args_;
  bool generate_debug_info_ = false;
  std::filesystem::path debug_path_ = "./debug";
};

class RealContext : public Context {
 public:
  RealContext(std::vector<std::string> args) : Context(args) {}

  std::istream& stdin() const override { return std::cin; }
  std::ostream& stdout() const override { return std::cout; }
  std::ostream& stderr() const override { return std::cerr; }

  std::string ReadFromFile(std::filesystem::path in_file) const override;
  void WriteToFile(std::string text, std::filesystem::path out_file) const override;
  void CreateDirectory(std::filesystem::path path) const override;
};

}  // namespace cmd

#endif /* cmd_context_h */
