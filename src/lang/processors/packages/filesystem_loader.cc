//
//  filesystem_loader.cc
//  Katara-tests
//
//  Created by Arne Philipeit on 6/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "filesystem_loader.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace lang {
namespace packages {

std::string FilesystemLoader::RelativeToAbsoluteDir(std::string dir_path) const {
  return current_dir_ / dir_path;
}

bool FilesystemLoader::CanReadRelativeDir(std::string dir_path) const {
  std::filesystem::path rel_path{dir_path};
  if (!rel_path.is_relative()) {
    return false;
  }
  return CanReadAbsoluteDir(RelativeToAbsoluteDir(rel_path));
}

std::vector<std::string> FilesystemLoader::SourceFilesInRelativeDir(std::string dir_path) const {
  std::filesystem::path rel_path{dir_path};
  return SourceFilesInAbsoluteDir(RelativeToAbsoluteDir(dir_path));
}

bool FilesystemLoader::CanReadAbsoluteDir(std::string dir_path) const {
  std::filesystem::path abs_path{dir_path};
  return abs_path.is_absolute() && std::filesystem::is_directory(abs_path);
}

std::vector<std::string> FilesystemLoader::SourceFilesInAbsoluteDir(std::string dir_path) const {
  std::vector<std::string> file_paths;
  for (auto& entry : std::filesystem::directory_iterator(dir_path)) {
    if (CanReadSourceFile(entry.path())) {
      file_paths.push_back(entry.path());
    }
  }
  return file_paths;
}

bool FilesystemLoader::CanReadSourceFile(std::string file_path) const {
  return CanReadSourceFile(std::filesystem::path{file_path});
}

bool FilesystemLoader::CanReadSourceFile(std::filesystem::path file_path) const {
  return file_path.is_absolute() && file_path.extension() == ".kat" &&
         std::filesystem::is_regular_file(file_path);
}

std::string FilesystemLoader::ReadSourceFile(std::string file_path) const {
  std::ifstream in_stream(file_path, std::ios::in);
  std::stringstream str_stream;
  str_stream << in_stream.rdbuf();
  return str_stream.str();
}

}  // namespace packages
}  // namespace lang
