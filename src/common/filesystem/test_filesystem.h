//
//  test_filesystem.h
//  Katara
//
//  Created by Arne Philipeit on 1/27/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef common_filesystem_test_filesystem_h
#define common_filesystem_test_filesystem_h

#include <filesystem>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <variant>

#include "src/common/filesystem/filesystem.h"

namespace common {

class TestFilesystem : public Filesystem {
 public:
  TestFilesystem(std::filesystem::path current_path = "/") : current_path_(current_path) {}

  std::filesystem::path Absolute(std::filesystem::path path) const override;
  std::filesystem::path CurrentPath() const override { return current_path_; }

  bool Exists(std::filesystem::path path) const override;
  bool Equivalent(std::filesystem::path path_a, std::filesystem::path path_b) const override;

  bool IsDirectory(std::filesystem::path path) const override;

  void ForEntriesInDirectory(std::filesystem::path path,
                             std::function<void(std::filesystem::path)> func) const override;

  void CreateDirectory(std::filesystem::path path) override;
  void CreateDirectories(std::filesystem::path path) override;

  void ReadFile(std::filesystem::path path,
                std::function<void(std::istream*)> reader) const override;
  void WriteFile(std::filesystem::path path, std::function<void(std::ostream*)> writer) override;

  void Remove(std::filesystem::path path) override;
  void RemoveAll(std::filesystem::path path) override;

 private:
  struct File {
    std::string contents;
  };
  struct Directory {
    std::unordered_map<std::string, std::variant<File, Directory>> entries;
  };

  typedef std::variant<std::monostate, const File*, const Directory*> ConstEntry;
  typedef std::variant<std::monostate, File*, Directory*> Entry;

  static void Visit(Entry entry, std::function<void()> not_present_handler,
                    std::function<void(File*)> file_handler,
                    std::function<void(Directory*)> directory_handler);

  static ConstEntry GetEntryInDirectory(const Directory* directory, std::string name);
  static Entry GetEntryInDirectory(Directory* directory, std::string name);

  ConstEntry GetEntry(std::filesystem::path path) const;
  Entry GetEntry(std::filesystem::path path);

  void CreateFile(std::filesystem::path path);
  void RemoveEntries(Directory* directory);

  std::filesystem::path current_path_;
  Directory root_;
};

}  // namespace common

#endif /* common_filesystem_test_filesystem_h */
