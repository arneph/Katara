//
//  file_doc.h
//  Katara
//
//  Created by Arne Philipeit on 1/2/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_docs_file_doc_h
#define lang_docs_file_doc_h

#include <string>

#include "src/lang/processors/docs/common.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/positions/positions.h"
#include "src/lang/representation/types/info.h"

namespace lang {
namespace docs {

struct FileDoc {
  std::string name;
  std::string html;
};

FileDoc GenerateDocumentationForFile(std::string name, ast::File* ast_file,
                                     pos::FileSet* pos_file_set, types::Info* type_info);

}  // namespace docs
}  // namespace lang

#endif /* lang_docs_file_doc_h */
