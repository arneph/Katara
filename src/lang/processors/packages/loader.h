//
//  loader.h
//  Katara
//
//  Created by Arne Philipeit on 6/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_packages_loader_h
#define lang_packages_loader_h

#include <string>
#include <vector>

namespace lang {
namespace packages {

class Loader {
 public:
  virtual ~Loader() {}

  virtual bool CanReadRelativeDir(std::string dir_path) const = 0;
  virtual std::vector<std::string> SourceFilesInRelativeDir(std::string dir_path) const = 0;

  virtual bool CanReadAbsoluteDir(std::string dir_path) const = 0;
  virtual std::vector<std::string> SourceFilesInAbsoluteDir(std::string dir_path) const = 0;

  virtual bool CanReadSourceFile(std::string file_path) const = 0;
  virtual std::string ReadSourceFile(std::string file_path) const = 0;
};

}  // namespace packages
}  // namespace lang

#endif /* lang_packages_loader_h */
