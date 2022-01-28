//
//  real_filesystem.h
//  Katara
//
//  Created by Arne Philipeit on 1/27/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef common_filesystem_real_filesystem_h
#define common_filesystem_real_filesystem_h

#include <filesystem>
#include <functional>
#include <iostream>

#include "src/common/filesystem/filesystem.h"

namespace common {

class RealFilesystem : public Filesystem {
 public:
  std::filesystem::path Absolute(std::filesystem::path path) const override;
  std::filesystem::path CurrentPath() const override;

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
};

}  // namespace common

#endif /* common_filesystem_real_filesystem_h */
