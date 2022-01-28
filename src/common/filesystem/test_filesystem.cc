//
//  test_filesystem.cc
//  Katara
//
//  Created by Arne Philipeit on 1/27/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "test_filesystem.h"

#include <sstream>
#include <type_traits>

#include "src/common/logging/logging.h"

namespace common {

void TestFilesystem::Visit(Entry entry, std::function<void()> not_present_handler,
                           std::function<void(File*)> file_handler,
                           std::function<void(Directory*)> directory_handler) {
  std::visit(
      [&](auto&& entry) {
        using T = std::decay_t<decltype(entry)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          not_present_handler();
        } else if constexpr (std::is_same_v<T, File*>) {
          file_handler(entry);
        } else if constexpr (std::is_same_v<T, Directory*>) {
          directory_handler(entry);
        }
      },
      entry);
}

std::variant<std::monostate, const TestFilesystem::File*, const TestFilesystem::Directory*>
TestFilesystem::GetEntryInDirectory(const Directory* directory, std::string name) {
  auto it = directory->entries.find(name);
  if (it == directory->entries.end()) {
    return std::monostate();
  }
  auto& entry = it->second;
  if (std::holds_alternative<File>(entry)) {
    return &std::get<File>(entry);
  } else if (std::holds_alternative<Directory>(entry)) {
    return &std::get<Directory>(entry);
  } else {
    fail("unexpected test filesystem directory entry");
  }
}

std::variant<std::monostate, TestFilesystem::File*, TestFilesystem::Directory*>
TestFilesystem::GetEntryInDirectory(Directory* directory, std::string name) {
  auto it = directory->entries.find(name);
  if (it == directory->entries.end()) {
    return std::monostate();
  }
  auto& entry = it->second;
  if (std::holds_alternative<File>(entry)) {
    return &std::get<File>(entry);
  } else if (std::holds_alternative<Directory>(entry)) {
    return &std::get<Directory>(entry);
  } else {
    fail("unexpected test filesystem directory entry");
  }
}

std::variant<std::monostate, const TestFilesystem::File*, const TestFilesystem::Directory*>
TestFilesystem::GetEntry(std::filesystem::path path) const {
  if (path.empty()) {
    return std::monostate();
  }
  path = Absolute(path);
  std::variant<std::monostate, const TestFilesystem::File*, const TestFilesystem::Directory*> entry;
  for (auto element : path) {
    if (element.string() == "/") {
      entry = &root_;
      continue;
    }
    if (!std::holds_alternative<const Directory*>(entry)) {
      return std::monostate();
    }
    entry = GetEntryInDirectory(std::get<const Directory*>(entry), element.string());
  }
  return entry;
}

std::variant<std::monostate, TestFilesystem::File*, TestFilesystem::Directory*>
TestFilesystem::GetEntry(std::filesystem::path path) {
  if (path.empty()) {
    return std::monostate();
  }
  path = Absolute(path);
  std::variant<std::monostate, TestFilesystem::File*, TestFilesystem::Directory*> entry;
  for (auto element : path) {
    if (element.string() == "/") {
      entry = &root_;
      continue;
    }
    if (!std::holds_alternative<Directory*>(entry)) {
      return std::monostate();
    }
    entry = GetEntryInDirectory(std::get<Directory*>(entry), element.string());
  }
  return entry;
}

std::filesystem::path TestFilesystem::Absolute(std::filesystem::path path) const {
  if (path.empty()) {
    return path;
  } else if (path.string().starts_with("/")) {
    return path;
  } else {
    return current_path_ / path;
  }
}

bool TestFilesystem::Exists(std::filesystem::path path) const {
  return !std::holds_alternative<std::monostate>(GetEntry(path));
}

bool TestFilesystem::Equivalent(std::filesystem::path path_a, std::filesystem::path path_b) const {
  if (path_a.string() == path_b.string() ||
      Absolute(path_a).string() == Absolute(path_b).string()) {
    return true;
  }
  auto entry_a = GetEntry(path_a);
  auto entry_b = GetEntry(path_b);
  if (!std::holds_alternative<std::monostate>(entry_a) &&
      !std::holds_alternative<std::monostate>(entry_b)) {
    return entry_a == entry_b;
  }
  return false;
}

bool TestFilesystem::IsDirectory(std::filesystem::path path) const {
  return std::holds_alternative<const Directory*>(GetEntry(path));
}

void TestFilesystem::ForEntriesInDirectory(std::filesystem::path path,
                                           std::function<void(std::filesystem::path)> func) const {
  for (const auto& [name, entry] : std::get<const Directory*>(GetEntry(path))->entries) {
    func(path / name);
  }
}

void TestFilesystem::CreateDirectory(std::filesystem::path path) {
  path = Absolute(path);
  if (path.empty()) {
    return;
  }
  Directory* parent = std::get<Directory*>(GetEntry(path.parent_path()));
  std::string name = path.filename();
  Visit(
      GetEntryInDirectory(parent, name),
      [&] {
        parent->entries.insert({name, Directory()});
      },
      [&](File*) {
        fail("could not create directory " + path.string() +
             " because a file of the same name already exists");
      },
      [](Directory*) {});
}

void TestFilesystem::CreateDirectories(std::filesystem::path path) {
  path = Absolute(path);
  if (path.empty()) {
    return;
  }
  Directory* parent = nullptr;
  for (auto element : path) {
    if (element.string() == "/") {
      parent = &root_;
      continue;
    }
    std::string name = element.string();
    Visit(
        GetEntryInDirectory(parent, name),
        [&] {
          parent->entries.insert({name, Directory()});
          parent = &std::get<Directory>(parent->entries.at(name));
        },
        [&](File*) {
          fail("could not create directory " + path.string() +
               " because a file of the same name (as a parent) already exists");
        },
        [&](Directory* child) { parent = child; });
  }
}

void TestFilesystem::ReadFile(std::filesystem::path path,
                              std::function<void(std::istream*)> reader) const {
  const File* file = std::get<const File*>(GetEntry(path));
  std::stringstream ss;
  ss << file->contents;
  reader(&ss);
}

void TestFilesystem::CreateFile(std::filesystem::path path) {
  path = Absolute(path);
  if (path.empty()) {
    return;
  }
  Directory* parent = std::get<Directory*>(GetEntry(path.parent_path()));
  std::string name = path.filename();
  Visit(
      GetEntryInDirectory(parent, name),
      [&] {
        parent->entries.insert({name, File()});
      },
      [](File*) {},
      [&](Directory*) {
        fail("could not create file " + path.string() +
             " because a directory of the same name already exists");
      });
}

void TestFilesystem::WriteFile(std::filesystem::path path,
                               std::function<void(std::ostream*)> writer) {
  CreateFile(path);
  File* file = std::get<File*>(GetEntry(path));
  std::stringstream ss;
  writer(&ss);
  file->contents = ss.str();
}

void TestFilesystem::Remove(std::filesystem::path path) {
  path = Absolute(path);
  Directory* parent = std::get<Directory*>(GetEntry(path.parent_path()));
  std::string name = path.filename();
  Visit(
      GetEntry(path), [] {}, [](File*) {},
      [&](Directory* directory) {
        if (!directory->entries.empty()) {
          fail("could not remove non-empty directory " + path.string());
        }
      });
  parent->entries.erase(name);
}

void TestFilesystem::RemoveAll(std::filesystem::path path) {
  path = Absolute(path);
  Directory* parent = std::get<Directory*>(GetEntry(path.parent_path()));
  std::string name = path.filename();
  Visit(
      GetEntry(path), [] {}, [](File*) {}, [&](Directory* directory) { RemoveEntries(directory); });
  parent->entries.erase(name);
}

void TestFilesystem::RemoveEntries(Directory* directory) {
  for (auto& [name, entry] : directory->entries) {
    Visit(
        GetEntryInDirectory(directory, name), [] {}, [](File*) {},
        [&](Directory* subdirectory) { RemoveEntries(subdirectory); });
  }
  directory->entries.clear();
}

}  // namespace common
