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

#include "lang/representation/positions/positions.h"
#include "lang/representation/types/info.h"
#include "lang/processors/packages/packages.h"
#include "lang/processors/docs/common.h"
#include "lang/processors/docs/file_doc.h"

namespace lang {
namespace docs {

struct PackageDoc {
    std::string path;
    std::string name;
    std::string html;
    std::vector<FileDoc> docs;
};

PackageDoc GenerateDocumentationForPackage(packages::Package *package,
                                           pos::FileSet *pos_file_set,
                                           types::Info *type_info);

}
}

#endif /* lang_docs_package_doc_h */
