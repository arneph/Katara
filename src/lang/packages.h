//
//  packages.h
//  Katara
//
//  Created by Arne Philipeit on 9/6/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_packages_h
#define lang_packages_h

#include <filesystem>
#include <memory>

#include "lang/positions.h"
#include "lang/ast.h"
#include "lang/ast_util.h"
#include "lang/constant.h"
#include "lang/types.h"
#include "lang/scanner.h"
#include "lang/parser.h"
#include "lang/type_checker.h"

namespace lang {
namespace packages {

class Package {
public:
    std::string name() const;
    std::string path() const;
    
    const std::vector<pos::File *>& pos_files() const;
    const std::vector<std::unique_ptr<ast::File>>& ast_files() const;
    types::Package * types_package() const;
    
    const std::vector<parser::Parser::Error>& parse_errors() const;
    const std::vector<type_checker::TypeChecker::Error>& type_errors() const;
    
private:
    Package() {}
    
    std::string name_;
    std::string path_;
    
    std::vector<pos::File *> pos_files_;
    std::vector<std::unique_ptr<ast::File>> ast_files_;
    types::Package *types_package_;
    
    std::vector<parser::Parser::Error> parse_errors_;
    std::vector<type_checker::TypeChecker::Error> type_errors_;
    
    friend class PackageManager;
};

class PackageManager {
public:
    typedef std::variant<std::string,
                         parser::Parser::Error,
                         type_checker::TypeChecker::Error> Error;
    
    PackageManager(std::string stdlib_path);
    
    pos::FileSet * file_set() const;
    types::TypeInfo * type_info() const;
    
    Package * LoadPackage(std::string pkg_dir);
    
private:
    std::filesystem::path FindPackagePath(std::string import, std::filesystem::path import_path);
    std::vector<std::filesystem::path> FindSourceFiles(std::filesystem::path package_path);
    
    std::filesystem::path stdlib_path_;
    std::unique_ptr<pos::FileSet> file_set_;
    std::unique_ptr<types::TypeInfo> type_info_;
    std::unordered_map<std::string, std::unique_ptr<Package>> packages_;
};

}
}

#endif /* lang_packages_h */
