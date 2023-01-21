//
//  test_filesystem.cc
//  Katara
//
//  Created by Arne Philipeit on 1/27/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "test_filesystem.h"

#include <sstream>

#include "src/common/logging/logging.h"

namespace common::filesystem {

void TestFilesystem::Visit(Entry* entry, std::function<void()> not_present_handler,
                           std::function<void(File*)> file_handler,
                           std::function<void(Directory*)> directory_handler) {
  if (entry == nullptr) {
    not_present_handler();
  } else {
    switch (entry->kind()) {
      case Entry::Kind::kFile:
        file_handler(static_cast<File*>(entry));
        break;
      case Entry::Kind::kDirectory:
        directory_handler(static_cast<Directory*>(entry));
        break;
      default:
        fail("unexpected test filesystem entry kind");
    }
  }
}

const TestFilesystem::Entry* TestFilesystem::GetEntryInDirectory(const Directory* directory,
                                                                 std::string name) {
  auto it = directory->entries.find(name);
  if (it == directory->entries.end()) {
    return nullptr;
  } else {
    return it->second.get();
  }
}

TestFilesystem::Entry* TestFilesystem::GetEntryInDirectory(Directory* directory, std::string name) {
  auto it = directory->entries.find(name);
  if (it == directory->entries.end()) {
    return nullptr;
  } else {
    return it->second.get();
  }
}

const TestFilesystem::Entry* TestFilesystem::GetEntry(std::filesystem::path path) const {
  if (path.empty()) {
    return nullptr;
  }
  path = Absolute(path);
  const TestFilesystem::Entry* entry = nullptr;
  for (auto element : path) {
    if (element.string() == "/") {
      entry = &root_;
      continue;
    }
    if (entry == nullptr || entry->kind() != Entry::Kind::kDirectory) {
      return nullptr;
    }
    entry = GetEntryInDirectory(static_cast<const Directory*>(entry), element.string());
  }
  return entry;
}

TestFilesystem::Entry* TestFilesystem::GetEntry(std::filesystem::path path) {
  if (path.empty()) {
    return nullptr;
  }
  path = Absolute(path);
  TestFilesystem::Entry* entry = nullptr;
  for (auto element : path) {
    if (element.string() == "/") {
      entry = &root_;
      continue;
    }
    if (entry->kind() != Entry::Kind::kDirectory) {
      return nullptr;
    }
    entry = GetEntryInDirectory(static_cast<Directory*>(entry), element.string());
  }
  return entry;
}

const TestFilesystem::Directory* TestFilesystem::GetDirectory(std::filesystem::path path) const {
  const Entry* entry = GetEntry(path);
  if (entry == nullptr || entry->kind() != Entry::Kind::kDirectory) {
    fail("the given path does not refer to an existing directory");
  }
  return static_cast<const Directory*>(entry);
}

TestFilesystem::Directory* TestFilesystem::GetDirectory(std::filesystem::path path) {
  Entry* entry = GetEntry(path);
  if (entry == nullptr || entry->kind() != Entry::Kind::kDirectory) {
    fail("the given path does not refer to an existing directory");
  }
  return static_cast<Directory*>(entry);
}

const TestFilesystem::File* TestFilesystem::GetFile(std::filesystem::path path) const {
  const Entry* entry = GetEntry(path);
  if (entry == nullptr || entry->kind() != Entry::Kind::kFile) {
    fail("the given path does not refer to an existing file");
  }
  return static_cast<const File*>(entry);
}

TestFilesystem::File* TestFilesystem::GetFile(std::filesystem::path path) {
  Entry* entry = GetEntry(path);
  if (entry == nullptr || entry->kind() != Entry::Kind::kFile) {
    fail("the given path does not refer to an existing file");
  }
  return static_cast<File*>(entry);
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

bool TestFilesystem::Exists(std::filesystem::path path) const { return GetEntry(path) != nullptr; }

bool TestFilesystem::Equivalent(std::filesystem::path path_a, std::filesystem::path path_b) const {
  if (path_a.string() == path_b.string() ||
      Absolute(path_a).string() == Absolute(path_b).string()) {
    return true;
  }
  auto entry_a = GetEntry(path_a);
  auto entry_b = GetEntry(path_b);
  if (entry_a != nullptr && entry_b != nullptr) {
    return entry_a == entry_b;
  }
  return false;
}

bool TestFilesystem::IsDirectory(std::filesystem::path path) const {
  const Entry* entry = GetEntry(path);
  return entry != nullptr && entry->kind() == Entry::Kind::kDirectory;
}

void TestFilesystem::ForEntriesInDirectory(std::filesystem::path path,
                                           std::function<void(std::filesystem::path)> func) const {
  for (const auto& [name, entry] : GetDirectory(path)->entries) {
    func(path / name);
  }
}

void TestFilesystem::CreateDirectory(std::filesystem::path path) {
  path = Absolute(path);
  if (path.empty()) {
    return;
  }
  Directory* parent = GetDirectory(path.parent_path());
  std::string name = path.filename();
  Visit(
      GetEntryInDirectory(parent, name),
      [&] {
        parent->entries.insert({name, std::make_unique<Directory>()});
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
          std::unique_ptr<Directory> child = std::make_unique<Directory>();
          Directory* child_ptr = child.get();
          parent->entries.insert({name, std::move(child)});
          parent = child_ptr;
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
  const File* file = GetFile(path);
  std::stringstream ss;
  ss << file->contents;
  reader(&ss);
}

void TestFilesystem::CreateFile(std::filesystem::path path) {
  path = Absolute(path);
  if (path.empty()) {
    return;
  }
  Directory* parent = GetDirectory(path.parent_path());
  std::string name = path.filename();
  Visit(
      GetEntryInDirectory(parent, name),
      [&] {
        parent->entries.insert({name, std::make_unique<File>()});
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
  File* file = GetFile(path);
  std::stringstream ss;
  writer(&ss);
  file->contents = ss.str();
}

void TestFilesystem::Remove(std::filesystem::path path) {
  path = Absolute(path);
  Directory* parent = GetDirectory(path.parent_path());
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
  Directory* parent = GetDirectory(path.parent_path());
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

}  // namespace common::filesystem
