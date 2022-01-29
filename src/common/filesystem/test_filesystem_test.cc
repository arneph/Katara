//
//  test_filesystem_test.cc
//  Katara
//
//  Created by Arne Philipeit on 1/28/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/common/filesystem/test_filesystem.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/common/filesystem/filesystem.h"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace common {
namespace {

std::vector<std::filesystem::path> CollectPathsInDirectory(std::filesystem::path dir,
                                                           TestFilesystem& fs) {
  std::vector<std::filesystem::path> entries;
  fs.ForEntriesInDirectory(dir, [&](std::filesystem::path entry) { entries.push_back(entry); });
  std::sort(entries.begin(), entries.end());
  return entries;
}

}  // namespace

TEST(TestFilesystemTest, ConstructAndDestructSucceeds) {
  TestFilesystem fs;

  EXPECT_EQ(fs.CurrentPath(), "/");
}

TEST(TestFilesystemTest, ConvertsToAbsolutePathsCorrectly) {
  TestFilesystem fs("/abc/123");

  EXPECT_EQ(fs.CurrentPath(), "/abc/123");
  EXPECT_EQ(fs.Absolute(""), "");
  EXPECT_EQ(fs.Absolute("/"), "/");
  EXPECT_EQ(fs.Absolute("/xyz"), "/xyz");
  EXPECT_EQ(fs.Absolute("/xyz/789"), "/xyz/789");
  EXPECT_EQ(fs.Absolute("hello"), "/abc/123/hello");
  EXPECT_EQ(fs.Absolute("hey/hey"), "/abc/123/hey/hey");
}

TEST(TestFilesystemTest, DeterminesEquivalentPathsCorrectly) {
  TestFilesystem fs("/current/path");

  EXPECT_TRUE(fs.Equivalent("", ""));
  EXPECT_TRUE(fs.Equivalent("/", "/"));
  EXPECT_TRUE(fs.Equivalent("/abc", "/abc"));
  EXPECT_TRUE(fs.Equivalent("/abc/123", "/abc/123"));
  EXPECT_TRUE(fs.Equivalent("/current/path/hello", "hello"));
  EXPECT_TRUE(fs.Equivalent("hello", "/current/path/hello"));
}

TEST(TestFilesystemTest, CreatingAndRemovingDirectoriesWorks) {
  TestFilesystem fs;

  EXPECT_TRUE(fs.Exists("/"));
  EXPECT_TRUE(fs.IsDirectory("/"));
  EXPECT_FALSE(fs.Exists("/abc"));
  EXPECT_FALSE(fs.IsDirectory("/abc"));
  EXPECT_FALSE(fs.Exists("/abc/hello"));
  EXPECT_FALSE(fs.IsDirectory("/abc/hello"));
  EXPECT_FALSE(fs.Exists("/xyz"));
  EXPECT_FALSE(fs.IsDirectory("/xyz"));
  EXPECT_FALSE(fs.Exists("/xyz/123"));
  EXPECT_FALSE(fs.IsDirectory("/xyz/123"));
  EXPECT_FALSE(fs.Exists("/xyz/789"));
  EXPECT_FALSE(fs.IsDirectory("/xyz/789"));
  EXPECT_THAT(CollectPathsInDirectory("/", fs), IsEmpty());

  fs.CreateDirectory("abc");
  fs.CreateDirectory("abc/hello");

  EXPECT_TRUE(fs.Exists("/"));
  EXPECT_TRUE(fs.IsDirectory("/"));
  EXPECT_TRUE(fs.Exists("/abc"));
  EXPECT_TRUE(fs.Exists("/abc"));
  EXPECT_TRUE(fs.Exists("/abc/hello"));
  EXPECT_TRUE(fs.IsDirectory("/abc/hello"));
  EXPECT_FALSE(fs.Exists("/xyz"));
  EXPECT_FALSE(fs.IsDirectory("/xyz"));
  EXPECT_FALSE(fs.Exists("/xyz/123"));
  EXPECT_FALSE(fs.IsDirectory("/xyz/123"));
  EXPECT_FALSE(fs.Exists("/xyz/789"));
  EXPECT_FALSE(fs.IsDirectory("/xyz/789"));
  EXPECT_THAT(CollectPathsInDirectory("/", fs), ElementsAre("/abc"));
  EXPECT_THAT(CollectPathsInDirectory("/abc", fs), ElementsAre("/abc/hello"));
  EXPECT_THAT(CollectPathsInDirectory("/abc/hello", fs), IsEmpty());

  fs.CreateDirectories("xyz/123");
  fs.CreateDirectories("/xyz/789");

  EXPECT_TRUE(fs.Exists("/"));
  EXPECT_TRUE(fs.IsDirectory("/"));
  EXPECT_TRUE(fs.Exists("/abc"));
  EXPECT_TRUE(fs.Exists("/abc"));
  EXPECT_TRUE(fs.Exists("/abc/hello"));
  EXPECT_TRUE(fs.IsDirectory("/abc/hello"));
  EXPECT_TRUE(fs.Exists("/xyz"));
  EXPECT_TRUE(fs.IsDirectory("/xyz"));
  EXPECT_TRUE(fs.Exists("/xyz/123"));
  EXPECT_TRUE(fs.IsDirectory("/xyz/123"));
  EXPECT_TRUE(fs.Exists("/xyz/789"));
  EXPECT_TRUE(fs.IsDirectory("/xyz/789"));
  EXPECT_THAT(CollectPathsInDirectory("/", fs), ElementsAre("/abc", "/xyz"));
  EXPECT_THAT(CollectPathsInDirectory("/abc", fs), ElementsAre("/abc/hello"));
  EXPECT_THAT(CollectPathsInDirectory("/abc/hello", fs), IsEmpty());
  EXPECT_THAT(CollectPathsInDirectory("/xyz", fs), ElementsAre("/xyz/123", "/xyz/789"));
  EXPECT_THAT(CollectPathsInDirectory("/xyz/123", fs), IsEmpty());
  EXPECT_THAT(CollectPathsInDirectory("/xyz/789", fs), IsEmpty());

  fs.Remove("abc/hello");

  EXPECT_TRUE(fs.Exists("/"));
  EXPECT_TRUE(fs.IsDirectory("/"));
  EXPECT_TRUE(fs.Exists("/abc"));
  EXPECT_TRUE(fs.Exists("/abc"));
  EXPECT_FALSE(fs.Exists("/abc/hello"));
  EXPECT_FALSE(fs.IsDirectory("/abc/hello"));
  EXPECT_TRUE(fs.Exists("/xyz"));
  EXPECT_TRUE(fs.IsDirectory("/xyz"));
  EXPECT_TRUE(fs.Exists("/xyz/123"));
  EXPECT_TRUE(fs.IsDirectory("/xyz/123"));
  EXPECT_TRUE(fs.Exists("/xyz/789"));
  EXPECT_TRUE(fs.IsDirectory("/xyz/789"));
  EXPECT_THAT(CollectPathsInDirectory("/", fs), ElementsAre("/abc", "/xyz"));
  EXPECT_THAT(CollectPathsInDirectory("/abc", fs), IsEmpty());
  EXPECT_THAT(CollectPathsInDirectory("/xyz", fs), ElementsAre("/xyz/123", "/xyz/789"));
  EXPECT_THAT(CollectPathsInDirectory("/xyz/123", fs), IsEmpty());
  EXPECT_THAT(CollectPathsInDirectory("/xyz/789", fs), IsEmpty());

  fs.RemoveAll("/abc");

  EXPECT_TRUE(fs.Exists("/"));
  EXPECT_TRUE(fs.IsDirectory("/"));
  EXPECT_FALSE(fs.Exists("/abc"));
  EXPECT_FALSE(fs.Exists("/abc"));
  EXPECT_FALSE(fs.Exists("/abc/hello"));
  EXPECT_FALSE(fs.IsDirectory("/abc/hello"));
  EXPECT_TRUE(fs.Exists("/xyz"));
  EXPECT_TRUE(fs.IsDirectory("/xyz"));
  EXPECT_TRUE(fs.Exists("/xyz/123"));
  EXPECT_TRUE(fs.IsDirectory("/xyz/123"));
  EXPECT_TRUE(fs.Exists("/xyz/789"));
  EXPECT_TRUE(fs.IsDirectory("/xyz/789"));
  EXPECT_THAT(CollectPathsInDirectory("/", fs), ElementsAre("/xyz"));
  EXPECT_THAT(CollectPathsInDirectory("/xyz", fs), ElementsAre("/xyz/123", "/xyz/789"));
  EXPECT_THAT(CollectPathsInDirectory("/xyz/123", fs), IsEmpty());
  EXPECT_THAT(CollectPathsInDirectory("/xyz/789", fs), IsEmpty());

  fs.RemoveAll("xyz");

  EXPECT_TRUE(fs.Exists("/"));
  EXPECT_TRUE(fs.IsDirectory("/"));
  EXPECT_FALSE(fs.Exists("/abc"));
  EXPECT_FALSE(fs.IsDirectory("/abc"));
  EXPECT_FALSE(fs.Exists("/abc/hello"));
  EXPECT_FALSE(fs.IsDirectory("/abc/hello"));
  EXPECT_FALSE(fs.Exists("/xyz"));
  EXPECT_FALSE(fs.IsDirectory("/xyz"));
  EXPECT_FALSE(fs.Exists("/xyz/123"));
  EXPECT_FALSE(fs.IsDirectory("/xyz/123"));
  EXPECT_FALSE(fs.Exists("/xyz/789"));
  EXPECT_FALSE(fs.IsDirectory("/xyz/789"));
  EXPECT_THAT(CollectPathsInDirectory("/", fs), IsEmpty());
}

TEST(TestFilesystemTest, ReadingAndWritingFileWorks) {
  TestFilesystem fs;

  fs.WriteFile("a", [](std::ostream*) {});

  EXPECT_THAT(CollectPathsInDirectory("/", fs), ElementsAre("/a"));
  EXPECT_TRUE(fs.Exists("/a"));
  EXPECT_FALSE(fs.IsDirectory("/a"));

  std::string a_contents;
  fs.ReadFile("a", [&](std::istream* is) {
    a_contents = std::string(std::istreambuf_iterator<char>(*is), {});
  });
  EXPECT_THAT(a_contents, IsEmpty());

  fs.WriteFile("a", [](std::ostream* os) { *os << "Hello world!"; });

  EXPECT_TRUE(fs.Exists("/a"));
  EXPECT_FALSE(fs.IsDirectory("/a"));

  a_contents.clear();
  fs.ReadFile("a", [&](std::istream* is) {
    a_contents = std::string(std::istreambuf_iterator<char>(*is), {});
  });
  EXPECT_THAT(a_contents, "Hello world!");

  fs.Remove("a");

  EXPECT_THAT(CollectPathsInDirectory("/", fs), IsEmpty());
  EXPECT_FALSE(fs.Exists("/a"));
  EXPECT_FALSE(fs.IsDirectory("/a"));
}

TEST(TestFilesystemTest, ReadingAndWritingFileWithHelperMethodsWorks) {
  TestFilesystem fs;

  fs.WriteContentsOfFile("a", "");

  EXPECT_THAT(CollectPathsInDirectory("/", fs), ElementsAre("/a"));
  EXPECT_TRUE(fs.Exists("/a"));
  EXPECT_FALSE(fs.IsDirectory("/a"));
  EXPECT_THAT(fs.ReadContentsOfFile("a"), IsEmpty());

  fs.WriteContentsOfFile("a", std::string("Hello world!"));

  EXPECT_TRUE(fs.Exists("/a"));
  EXPECT_FALSE(fs.IsDirectory("/a"));
  EXPECT_EQ(fs.ReadContentsOfFile("a"), "Hello world!");

  fs.Remove("a");

  EXPECT_THAT(CollectPathsInDirectory("/", fs), IsEmpty());
  EXPECT_FALSE(fs.Exists("/a"));
  EXPECT_FALSE(fs.IsDirectory("/a"));
}

TEST(TestFilesystemTest, ReadingAndWritingFileInSubdirectoryWorks) {
  TestFilesystem fs;

  fs.CreateDirectories("xyz/123");
  fs.WriteFile("xyz/123/a", [](std::ostream*) {});

  EXPECT_THAT(CollectPathsInDirectory("/xyz/123", fs), ElementsAre("/xyz/123/a"));
  EXPECT_TRUE(fs.Exists("/xyz/123/a"));
  EXPECT_FALSE(fs.IsDirectory("/xyz/123/a"));

  std::string a_contents;
  fs.ReadFile("xyz/123/a", [&](std::istream* is) {
    a_contents = std::string(std::istreambuf_iterator<char>(*is), {});
  });
  EXPECT_THAT(a_contents, IsEmpty());

  fs.WriteFile("xyz/123/a", [](std::ostream* os) { *os << "Hello world!"; });

  EXPECT_TRUE(fs.Exists("/xyz/123/a"));
  EXPECT_FALSE(fs.IsDirectory("/xyz/123/a"));

  a_contents.clear();
  fs.ReadFile("xyz/123/a", [&](std::istream* is) {
    a_contents = std::string(std::istreambuf_iterator<char>(*is), {});
  });
  EXPECT_THAT(a_contents, "Hello world!");

  fs.Remove("xyz/123/a");

  EXPECT_THAT(CollectPathsInDirectory("/", fs), ElementsAre("/xyz"));
  EXPECT_THAT(CollectPathsInDirectory("/xyz", fs), ElementsAre("/xyz/123"));
  EXPECT_THAT(CollectPathsInDirectory("/xyz/123", fs), IsEmpty());
  EXPECT_FALSE(fs.Exists("/xyz/123/a"));
  EXPECT_FALSE(fs.IsDirectory("/xyz/123/a"));
}

}  // namespace common
