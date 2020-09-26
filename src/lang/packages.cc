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

const std::vector<parser::Parser::Error>& Package::parse_errors() const {
    return parse_errors_;
}
 
const std::vector<type_checker::TypeChecker::Error>& Package::type_errors() const {
    return type_errors_;
}

PackageManager::PackageManager(std::string stdlib_path) : stdlib_path_(stdlib_path) {
    file_set_ = std::make_unique<pos::FileSet>();
    type_info_ = std::make_unique<types::TypeInfo>();
}

pos::FileSet * PackageManager::file_set() const {
    return file_set_.get();
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
        return package.get();
    }

    for (auto& source_file : source_files) {
        std::ifstream in_stream(source_file, std::ios::in);
        std::stringstream str_stream;
        str_stream << in_stream.rdbuf();
        std::string contents = str_stream.str();
        
        package->pos_files_.push_back(file_set_->AddFile(source_file.filename(),
                                                         contents));
    }
    for (auto& pos_file : package->pos_files_) {
        package->ast_files_.push_back(parser::Parser::ParseFile(pos_file,
                                                                package->parse_errors_));
    }
    if (!package->parse_errors_.empty()) {
        return package.get();
    }
    for (auto& ast_file : package->ast_files_) {
        auto importer = [&](std::string import) -> types::Package * {
            auto import_path = FindPackagePath(import, pkg_path);
            return LoadPackage(import_path)->types_package_;
        };
        type_checker::TypeChecker::Check(ast_file.get(),
                                         type_info_.get(),
                                         importer,
                                         package->type_errors_);
    }
    
    return package.get();
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
