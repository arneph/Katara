//
//  filesystem_loader.hpp
//  Katara-tests
//
//  Created by Arne Philipeit on 6/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_packages_filesystem_loader_h
#define lang_packages_filesystem_loader_h

#include <filesystem>
#include <string>
#include <vector>

#include "src/lang/processors/packages/loader.h"

namespace lang {
namespace packages {

class FilesystemLoader : public Loader {
 public:
  FilesystemLoader(std::filesystem::path dir) : current_dir_(dir) {}

  std::string RelativeToAbsoluteDir(std::string dir_path) const override;

  bool CanReadRelativeDir(std::string dir_path) const override;
  std::vector<std::string> SourceFilesInRelativeDir(std::string dir_path) const override;

  bool CanReadAbsoluteDir(std::string dir_path) const override;
  std::vector<std::string> SourceFilesInAbsoluteDir(std::string dir_path) const override;

  bool CanReadSourceFile(std::string file_path) const override;
  std::string ReadSourceFile(std::string file_path) const override;

 private:
  bool CanReadSourceFile(std::filesystem::path file_path) const;

  std::filesystem::path current_dir_;
};

}  // namespace packages
}  // namespace lang

#endif /* filesystem_loader_h */
