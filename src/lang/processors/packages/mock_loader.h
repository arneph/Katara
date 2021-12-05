//
//  mock_loader.h
//  Katara
//
//  Created by Arne Philipeit on 6/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_packages_mock_loader_h
#define lang_packages_mock_loader_h

#include <memory>
#include <unordered_map>

#include "src/lang/processors/packages/loader.h"

namespace lang {
namespace packages {

class MockLoader : public Loader {
 public:
  std::string RelativeToAbsoluteDir(std::string dir_path) const override;

  bool CanReadRelativeDir(std::string dir_path) const override;
  std::vector<std::string> SourceFilesInRelativeDir(std::string dir_path) const override;

  bool CanReadAbsoluteDir(std::string dir_path) const override;
  std::vector<std::string> SourceFilesInAbsoluteDir(std::string dir_path) const override;

  bool CanReadSourceFile(std::string file_path) const override;
  std::string ReadSourceFile(std::string file_path) const override;

 private:
  struct Directory {
    std::unordered_map<std::string, std::string> file_contents_;
  };

  std::string current_dir_;
  std::unordered_map<std::string, Directory> dirs_;

  friend class MockLoaderBuilder;
};

class MockLoaderBuilder {
 public:
  MockLoaderBuilder() : loader_(std::make_unique<MockLoader>()) {}

  MockLoaderBuilder& SetCurrentDir(std::string dir);
  MockLoaderBuilder& AddSourceFile(std::string dir, std::string name, std::string contents);

  std::unique_ptr<MockLoader> Build() { return std::move(loader_); }

 private:
  std::unique_ptr<MockLoader> loader_;
};

}  // namespace packages
}  // namespace lang

#endif /* lang_packages_mock_loader_h */
