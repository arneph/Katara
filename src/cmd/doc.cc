//
//  doc.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "doc.h"

#include <variant>

#include "src/cmd/load.h"
#include "src/lang/processors/docs/file_doc.h"
#include "src/lang/processors/docs/package_doc.h"

namespace cmd {

ErrorCode Doc(Context* ctx) {
  std::variant<LoadResult, ErrorCode> load_result_or_error = Load(ctx);
  if (std::holds_alternative<ErrorCode>(load_result_or_error)) {
    return std::get<ErrorCode>(load_result_or_error);
  }
  LoadResult& load_result = std::get<LoadResult>(load_result_or_error);
  std::unique_ptr<lang::packages::PackageManager>& pkg_manager = load_result.pkg_manager;
  std::vector<lang::packages::Package*>& arg_pkgs = load_result.arg_pkgs;

  for (lang::packages::Package* pkg : arg_pkgs) {
    std::filesystem::path pkg_dir{pkg->dir()};
    std::filesystem::path docs_dir = pkg_dir / "doc";
    ctx->filesystem()->CreateDirectory(docs_dir);

    lang::docs::PackageDoc pkg_doc = lang::docs::GenerateDocumentationForPackage(
        pkg, pkg_manager->file_set(), pkg_manager->type_info());

    ctx->filesystem()->WriteContentsOfFile(docs_dir / (pkg_doc.name + ".html"), pkg_doc.html);
    for (auto file_doc : pkg_doc.docs) {
      ctx->filesystem()->WriteContentsOfFile(docs_dir / (file_doc.name + ".html"), file_doc.html);
    }
  }
  return kNoError;
}

}  // namespace cmd
