//
//  packages.cc
//  Katara
//
//  Created by Arne Philipeit on 9/6/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "packages.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "lang/representation/ast/ast_util.h"
#include "lang/processors/parser/parser.h"
#include "lang/processors/type_checker/type_checker.h"

namespace lang {
namespace packages {

std::string Package::name() const {
    return name_;
}

std::string Package::path() const {
    return path_;
}

const std::vector<pos::File *>& Package::pos_files() const {
    return pos_files_;
}

const std::vector<std::unique_ptr<ast::File>>& Package::ast_files() const {
    return ast_files_;
}

types::Package * Package::types_package() const {
    return types_package_;
}

bool Package::has_errors() const {
    for (auto& issue : issues_) {
        if (issue.severity() == issues::Severity::Error ||
            issue.severity() == issues::Severity::Fatal) {
            return true;
        }
    }
    return false;
}

bool Package::has_fatal_errors() const {
    for (auto& issue : issues_) {
        if (issue.severity() == issues::Severity::Fatal) {
            return true;
        }
    }
    return false;
}

const std::vector<issues::Issue>& Package::issues() const {
    return issues_;
}

PackageManager::PackageManager(std::string stdlib_path) : stdlib_path_(stdlib_path) {
    file_set_ = std::make_unique<pos::FileSet>();
    type_info_ = std::make_unique<types::TypeInfo>();
}

pos::FileSet * PackageManager::file_set() const {
    return file_set_.get();
}

types::TypeInfo * PackageManager::type_info() const {
    return type_info_.get();
}

Package * PackageManager::LoadPackage(std::string import_dir) {
    auto pkg_path = std::filesystem::absolute(import_dir);
    if (!std::filesystem::is_directory(pkg_path)) {
        return nullptr;
    }
    if (auto it = packages_.find(pkg_path); it != packages_.end()) {
        return it->second.get();
    }
    auto it = packages_.insert({pkg_path, std::unique_ptr<Package>(new Package())});
    auto &package = it.first->second;
    package->name_ = pkg_path.filename();
    package->path_ = pkg_path;
    
    auto source_files = FindSourceFiles(pkg_path);
    if (source_files.empty()) {
        package->issues_.push_back(
            issues::Issue(issues::Origin::PackageManager,
                          issues::Severity::Warning,
                          std::vector<pos::pos_t>{},
                          "package directory does not contain source files"));
        return nullptr;
    }

    for (auto& source_file : source_files) {
        std::ifstream in_stream(source_file, std::ios::in);
        std::stringstream str_stream;
        str_stream << in_stream.rdbuf();
        std::string contents = str_stream.str();
        
        package->pos_files_.push_back(file_set_->AddFile(source_file.filename(),
                                                         contents));
    }

    std::vector<ast::File *> ast_files;
    for (auto& pos_file : package->pos_files_) {
        std::unique_ptr<ast::File> ast_file = parser::Parser::ParseFile(pos_file,
                                                                        package->issues_);
        ast_files.push_back(ast_file.get());
        package->ast_files_.push_back(std::move(ast_file));
    }
    if (package->has_fatal_errors()) {
        return package.get();
    }
    
    auto importer = [&](std::string import) -> types::Package * {
        std::filesystem::path import_path = FindPackagePath(import, pkg_path);
        Package *package = LoadPackage(import_path);
        if (package->has_errors()) {
            return nullptr;
        }
        return package->types_package_;
    };
    types::Package *types_package =
        type_checker::TypeChecker::Check(import_dir,
                                         ast_files,
                                         type_info_.get(),
                                         importer,
                                         package->issues_);
    package->types_package_ = types_package;
    if (package->has_fatal_errors()) {
        return package.get();
    }
    
    return package.get();
}

std::vector<Package *> PackageManager::Packages() const {
    std::vector<Package *> packages;
    for (auto& [_, package] : packages_) {
        packages.push_back(package.get());
    }
    return packages;
}

std::filesystem::path PackageManager::FindPackagePath(std::string import,
                                                       std::filesystem::path import_path) {
    if (auto stdlib_pkg_path = stdlib_path_ / import;
        std::filesystem::is_directory(stdlib_pkg_path)) {
        return stdlib_pkg_path;
    }
    
    std::filesystem::path pkg_path(import);
    if (pkg_path.is_absolute()) {
        return pkg_path;
    }
    return import_path / import;
}

std::vector<std::filesystem::path>
PackageManager::FindSourceFiles(std::filesystem::path package_path) {
    std::vector<std::filesystem::path> results;
    for (auto entry : std::filesystem::directory_iterator(package_path)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        auto file_path = entry.path();
        if (file_path.extension() != ".kat") {
            continue;
        }
        results.push_back(file_path);
    }
    return results;
}

}
}