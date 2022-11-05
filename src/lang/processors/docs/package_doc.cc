//
//  package_doc.cc
//  Katara
//
//  Created by Arne Philipeit on 1/2/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "package_doc.h"

#include <sstream>

namespace lang {
namespace docs {

namespace {

void GenerateIssueDescription(std::ostringstream& ss, packages::Package* package,
                              const common::PosFileSet* pos_file_set) {
  ss << "Issues:<br><dl>\n";
  for (issues::Issue issue : package->issue_tracker().issues()) {
    ss << "<dt>";
    switch (issue.severity()) {
      case lang::issues::Severity::kWarning:
        ss << html::TagsForText("Warning: ", formats::kWarning);
        break;
      case lang::issues::Severity::kError:
      case lang::issues::Severity::kFatal:
        ss << html::TagsForText("Error: ", formats::kError);
        break;
    }
    ss << issue.message() << "\n";
    for (common::pos_t pos : issue.positions()) {
      ss << "<dd>\n";
      common::Position position = pos_file_set->PositionFor(pos);
      common::PosFile* pos_file = pos_file_set->FileAt(pos);
      std::string line = pos_file->LineFor(pos);
      size_t whitespace = 0;
      for (; whitespace < line.length(); whitespace++) {
        if (line.at(whitespace) != ' ' && line.at(whitespace) != '\t') {
          break;
        }
      }
      ss << "<a href=\"" << pos_file->name() << ".html#p" << pos << "\">" << position.ToString()
         << "</a>:<br/>";
      ss << "<div style=\"font-family:'Courier New'\">";
      ss << line.substr(whitespace) << "<br/>";
      for (size_t i = 0; i < position.column_ - whitespace; i++) {
        ss << "&nbsp;";
      }
      ss << "^</div>\n";
      ss << "</dd>\n";
    }
    ss << "<br/></dt>\n";
  }
  ss << "</dl><br/>\n";
}

void GeneratePackageDescription(std::ostringstream& ss, packages::Package* package,
                                const common::PosFileSet* pos_file_set) {
  ss << "Path: " << package->path() << "<br>\n"
     << "Package files:<dl>\n";
  for (common::PosFile* pos_file : package->pos_files()) {
    ss << "<dt>"
       << "<a href=\"" << pos_file->name() << ".html\">" << pos_file->name() << "</a>"
       << "</dt>\n";
  }
  ss << "</dl>\n";
  if (!package->issue_tracker().issues().empty()) {
    GenerateIssueDescription(ss, package, pos_file_set);
  }
}

}  // namespace

PackageDoc GenerateDocumentationForPackage(packages::Package* package,
                                           const common::PosFileSet* pos_file_set,
                                           types::Info* type_info) {
  std::ostringstream ss;
  ss << "<!DOCTYPE html>\n"
     << "<html>\n"
     << "<head>\n"
     << "<title>" << package->name() << "</title>\n"
     << "</head>\n"
     << "<body style=\"font-family:'Arial'\">\n"
     << "<h1>Package " << package->name() << "</h1>\n"
     << "<div>\n";
  GeneratePackageDescription(ss, package, pos_file_set);
  ss << "</div>"
     << "</body>\n"
     << "</html>";
  std::vector<FileDoc> docs;
  for (auto [name, ast_file] : package->ast_package()->files()) {
    docs.push_back(GenerateDocumentationForFile(name, ast_file, pos_file_set, type_info));
  }
  return PackageDoc{
      .path = package->path(),
      .name = package->name(),
      .html = ss.str(),
      .docs = docs,
  };
}

}  // namespace docs
}  // namespace lang
