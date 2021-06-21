//
//  package_doc.h
//  Katara
//
//  Created by Arne Philipeit on 1/2/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_docs_package_doc_h
#define lang_docs_package_doc_h

#include <string>
#include <vector>

#include "src/lang/processors/docs/common.h"
#include "src/lang/processors/docs/file_doc.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/representation/positions/positions.h"
#include "src/lang/representation/types/info.h"

namespace lang {
namespace docs {

struct PackageDoc {
  std::string path;
  std::string name;
  std::string html;
  std::vector<FileDoc> docs;
};

PackageDoc GenerateDocumentationForPackage(packages::Package* package,
                                           const pos::FileSet* pos_file_set,
                                           types::Info* type_info);

}  // namespace docs
}  // namespace lang

#endif /* lang_docs_package_doc_h */
