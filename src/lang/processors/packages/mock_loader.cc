//
//  mock_loader.cc
//  Katara
//
//  Created by Arne Philipeit on 6/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "mock_loader.h"

namespace {

std::pair<std::string, std::string> DirAndNameFromPath(std::string file_path) {
  size_t last_slash_index = file_path.find_last_of('/');
  if (last_slash_index == std::string::npos) {
    return {"", file_path};
  } else {
    std::string dir = file_path.substr(0, last_slash_index);
    std::string name = file_path.substr(last_slash_index + 1);
    if (dir.empty()) {
      dir = "/";
    }
    return {dir, name};
  }
}

}  // namespace

namespace lang {
namespace packages {

std::string MockLoader::RelativeToAbsoluteDir(std::string dir_path) const {
  return current_dir_ + "/" + dir_path;
}

bool MockLoader::CanReadRelativeDir(std::string dir_path) const {
  return CanReadAbsoluteDir(RelativeToAbsoluteDir(dir_path));
}

std::vector<std::string> MockLoader::SourceFilesInRelativeDir(std::string dir_path) const {
  return SourceFilesInAbsoluteDir(RelativeToAbsoluteDir(dir_path));
}

bool MockLoader::CanReadAbsoluteDir(std::string dir_path) const { return dirs_.contains(dir_path); }

std::vector<std::string> MockLoader::SourceFilesInAbsoluteDir(std::string dir_path) const {
  std::vector<std::string> file_paths;
  for (auto [file_name, file_contents] : dirs_.at(dir_path).file_contents_) {
    if (dir_path != "/") {
      file_paths.push_back(dir_path + "/" + file_name);
    } else {
      file_paths.push_back("/" + file_name);
    }
  }
  return file_paths;
}

bool MockLoader::CanReadSourceFile(std::string file_path) const {
  auto [dir, name] = DirAndNameFromPath(file_path);
  auto it = dirs_.find(dir);
  if (it == dirs_.end()) {
    return false;
  }
  return it->second.file_contents_.contains(name);
}

std::string MockLoader::ReadSourceFile(std::string file_path) const {
  auto [dir, name] = DirAndNameFromPath(file_path);
  return dirs_.at(dir).file_contents_.at(name);
}

MockLoaderBuilder& MockLoaderBuilder::SetCurrentDir(std::string dir) {
  while (dir.length() > 1 && dir.ends_with('/')) {
    dir = dir.substr(0, dir.length() - 1);
  }
  loader_->current_dir_ = dir;
  return *this;
}

MockLoaderBuilder& MockLoaderBuilder::AddSourceFile(std::string dir, std::string name,
                                                    std::string contents) {
  while (dir.length() > 1 && dir.ends_with('/')) {
    dir = dir.substr(0, dir.length() - 1);
  }
  loader_->dirs_[dir].file_contents_.insert({name, contents});
  return *this;
}

}  // namespace packages
}  // namespace lang
