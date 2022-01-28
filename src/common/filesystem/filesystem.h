//
//  filesystem.h
//  Katara
//
//  Created by Arne Philipeit on 1/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef common_filesystem_filesystem_h
#define common_filesystem_filesystem_h

#include <filesystem>
#include <functional>
#include <iostream>

namespace common {

class Filesystem {
 public:
  virtual ~Filesystem() {}

  virtual std::filesystem::path Absolute(std::filesystem::path path) const = 0;
  virtual std::filesystem::path CurrentPath() const = 0;

  virtual bool Exists(std::filesystem::path path) const = 0;
  virtual bool Equivalent(std::filesystem::path path_a, std::filesystem::path path_b) const = 0;

  virtual bool IsDirectory(std::filesystem::path path) const = 0;

  virtual void ForEntriesInDirectory(std::filesystem::path path,
                                     std::function<void(std::filesystem::path)> func) const = 0;

  virtual void CreateDirectory(std::filesystem::path path) = 0;
  virtual void CreateDirectories(std::filesystem::path path) = 0;

  virtual void ReadFile(std::filesystem::path path,
                        std::function<void(std::istream*)> reader) const = 0;
  virtual void WriteFile(std::filesystem::path path, std::function<void(std::ostream*)> writer) = 0;

  virtual void Remove(std::filesystem::path path) = 0;
  virtual void RemoveAll(std::filesystem::path path) = 0;
};

}  // namespace common

#endif /* common_filesystem_filesystem_h */
