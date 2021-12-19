//
//  doc.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "doc.h"

#include "src/cmd/load.h"
#include "src/cmd/util.h"
#include "src/lang/processors/docs/file_doc.h"
#include "src/lang/processors/docs/package_doc.h"

namespace cmd {

int Doc(const std::vector<std::string> args, std::ostream& err) {
  auto [pkg_manager, arg_pkgs, generate_debug_info, exit_code] = Load(args, err);
  if (exit_code) {
    return exit_code;
  }

  for (lang::packages::Package* pkg : arg_pkgs) {
    std::filesystem::path pkg_dir{pkg->dir()};
    std::filesystem::path docs_dir = pkg_dir / "doc";
    std::filesystem::create_directory(docs_dir);

    lang::docs::PackageDoc pkg_doc = lang::docs::GenerateDocumentationForPackage(
        pkg, pkg_manager->file_set(), pkg_manager->type_info());

    WriteToFile(pkg_doc.html, docs_dir / (pkg_doc.name + ".html"));
    for (auto file_doc : pkg_doc.docs) {
      WriteToFile(file_doc.html, docs_dir / (file_doc.name + ".html"));
    }
  }
  return 0;
}

}  // namespace cmd
