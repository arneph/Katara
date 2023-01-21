//
//  real_filesystem.cc
//  Katara
//
//  Created by Arne Philipeit on 1/27/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "real_filesystem.h"

#include <fstream>

#include "src/common/logging/logging.h"

namespace common::filesystem {
namespace {

std::string ErrorCodeToString(std::error_code& ec) {
  return std::string(ec.category().name()) + ":" + std::to_string(ec.value()) +
         (ec.message().empty() ? "" : "[" + ec.message() + "]");
}

}  // namespace

std::filesystem::path RealFilesystem::Absolute(std::filesystem::path path) const {
  std::error_code ec;
  std::filesystem::path absolute = std::filesystem::absolute(path, ec);
  if (ec) {
    fail("could not get absolute path for " + path.string() + ": " + ErrorCodeToString(ec));
  }
  return absolute;
}

std::filesystem::path RealFilesystem::CurrentPath() const {
  std::error_code ec;
  std::filesystem::path current_path = std::filesystem::current_path(ec);
  if (ec) {
    fail("could not get current path: " + ErrorCodeToString(ec));
  }
  return current_path;
}

bool RealFilesystem::Exists(std::filesystem::path path) const {
  std::error_code ec;
  bool exists = std::filesystem::exists(path, ec);
  if (ec) {
    fail("could not determine if " + path.string() + " exists: " + ErrorCodeToString(ec));
  }
  return exists;
}

bool RealFilesystem::Equivalent(std::filesystem::path path_a, std::filesystem::path path_b) const {
  std::error_code ec;
  bool equivalent = std::filesystem::equivalent(path_a, path_b, ec);
  if (ec) {
    fail("could not determine if " + path_a.string() + " and " + path_b.string() +
         " are equivalent: " + ErrorCodeToString(ec));
  }
  return equivalent;
}

bool RealFilesystem::IsDirectory(std::filesystem::path path) const {
  std::error_code ec;
  bool is_directory = std::filesystem::is_directory(path, ec);
  if (ec) {
    fail("could not determine if " + path.string() + " is a directory: " + ErrorCodeToString(ec));
  }
  return is_directory;
}

void RealFilesystem::ForEntriesInDirectory(std::filesystem::path path,
                                           std::function<void(std::filesystem::path)> func) const {
  std::error_code ec;
  std::filesystem::directory_iterator iterator(path, ec);
  if (ec) {
    fail("could not iterate over " + path.string() + ": " + ErrorCodeToString(ec));
  }
  while (iterator != std::filesystem::end(iterator)) {
    const std::filesystem::directory_entry& entry = *iterator;
    func(entry.path());

    iterator.increment(ec);
    if (ec) {
      fail("could iterate over " + path.string() + ": " + ErrorCodeToString(ec));
    }
  }
}

void RealFilesystem::CreateDirectory(std::filesystem::path path) {
  std::error_code ec;
  std::filesystem::create_directory(path, ec);
  if (ec) {
    fail("could not create directory " + path.string() + ": " + ErrorCodeToString(ec));
  }
}

void RealFilesystem::CreateDirectories(std::filesystem::path path) {
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
  if (ec) {
    fail("could not create directories " + path.string() + ": " + ErrorCodeToString(ec));
  }
}

void RealFilesystem::ReadFile(std::filesystem::path path,
                              std::function<void(std::istream*)> reader) const {
  std::ifstream stream(path, std::ios::in);
  reader(&stream);
}

void RealFilesystem::WriteFile(std::filesystem::path path,
                               std::function<void(std::ostream*)> writer) {
  std::ofstream stream(path, std::ios::out);
  writer(&stream);
}

void RealFilesystem::Remove(std::filesystem::path path) {
  std::error_code ec;
  std::filesystem::remove(path, ec);
  if (ec) {
    fail("could not remove " + path.string() + ": " + ErrorCodeToString(ec));
  }
}

void RealFilesystem::RemoveAll(std::filesystem::path path) {
  std::error_code ec;
  std::filesystem::remove_all(path, ec);
  if (ec) {
    fail("could not remove " + path.string() + ": " + ErrorCodeToString(ec));
  }
}

}  // namespace common::filesystem
