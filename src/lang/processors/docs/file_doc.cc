//
//  file_doc.cc
//  Katara
//
//  Created by Arne Philipeit on 1/2/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "file_doc.h"

#include <iomanip>
#include <sstream>

#include "src/lang/processors/scanner/scanner.h"
#include "src/lang/representation/ast/ast_util.h"
#include "src/lang/representation/types/objects.h"
#include "src/lang/representation/types/package.h"
#include "src/lang/representation/types/types.h"

namespace lang {
namespace docs {

using ::common::positions::File;
using ::common::positions::FileSet;
using ::common::positions::pos_t;
using ::common::positions::range_t;

namespace {

std::string InsertLineNumbers(std::string text, int64_t& line_number) {
  std::ostringstream ss;
  if (line_number == 0) {
    line_number++;
    ss << std::setw(5) << line_number << " ";
  }
  for (char c : text) {
    ss << c;
    if (c != '\n') continue;
    line_number++;
    ss << std::setw(5) << line_number << " ";
  }
  return ss.str();
}
};  // namespace

FileDoc GenerateDocumentationForFile(std::string name, ast::File* ast_file,
                                     const FileSet* pos_file_set, types::Info* type_info) {
  File* pos_file = pos_file_set->FileAt(ast_file->start());
  scanner::Scanner scanner(pos_file);
  pos_t last_pos = pos_file->start() - 1;
  std::ostringstream ss;
  ss << "<!DOCTYPE html>\n"
     << "<html>\n"
     << "<head>\n"
     << "<title>" << name << "</title>\n"
     << "</head>\n"
     << "<body>\n"
     << "<div style=\"font-family:'Courier New'\">\n";
  int64_t line_number = 0;
  while (scanner.token() != tokens::kEOF) {
    std::string whitespace =
        pos_file->contents(range_t{.start = last_pos + 1, .end = scanner.token_start() - 1});
    std::string contents =
        pos_file->contents(range_t{.start = scanner.token_start(), .end = scanner.token_end()});
    whitespace = html::Escape(InsertLineNumbers(whitespace, line_number));
    contents = html::Escape(InsertLineNumbers(contents, line_number));
    html::TextFormat format;
    std::string id = "";
    std::string classs = "";
    std::optional<html::GroupLink> link;
    if (scanner.token() == tokens::kIdent) {
      ast::Ident* ident = nullptr;
      ast::WalkFunction f([&scanner, &ident, &f](ast::Node* node) -> ast::WalkFunction {
        if (node == nullptr) return f;
        if (node->node_kind() == ast::NodeKind::kIdent && node->start() == scanner.token_start()) {
          ident = static_cast<ast::Ident*>(node);
          return ast::WalkFunction();
        }
        return f;
      });
      ast::Walk(ast_file, f);
      format = FormatForIdent(ident, type_info);
      id = "p" + std::to_string(scanner.token_start());
      types::Object* obj = type_info->ObjectOf(ident);
      if (obj != nullptr && obj->package() != nullptr) {
        File* obj_file = pos_file_set->FileAt(obj->position());
        classs = "p" + std::to_string(obj->position());
        link = html::GroupLink{
            .link = obj_file->name() + ".html#" + classs,
            .linked_id = classs,
            .group_class = classs,
        };
      }
    } else {
      format = FormatForToken(scanner.token());
    }

    ss << whitespace;
    ss << html::TagsForText(contents, format, id, classs, link);

    last_pos = scanner.token_end();
    scanner.Next();
  }
  ss << "\n</div>\n"
     << "</body>\n"
     << "</html>";
  return FileDoc{
      .name = name,
      .html = ss.str(),
  };
}

}  // namespace docs
}  // namespace lang
