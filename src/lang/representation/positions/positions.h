//
//  positions.h
//  Katara
//
//  Created by Arne Philipeit on 7/4/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_positions_h
#define lang_positions_h

#include <memory>
#include <string>
#include <vector>

namespace lang {
namespace pos {

typedef int64_t pos_t;

constexpr pos_t kNoPos = 0;

struct Position {
  Position() : Position("", 0, 0) {}
  Position(int64_t line) : Position("", line, 0) {}
  Position(int64_t line, int64_t column) : Position("", line, column) {}
  Position(std::string filename) : Position(filename, 0, 0) {}
  Position(std::string filename, int64_t line) : Position(filename, line, 0) {}
  Position(std::string filename, int64_t line, int64_t column)
      : filename_(filename), line_(line), column_(column) {}

  bool IsValid() const;

  std::string ToString() const;

  const std::string filename_;
  const int64_t line_;
  const int64_t column_;
};

class FileSet;

class File {
 public:
  std::string name() const;
  pos_t start() const;
  pos_t end() const;
  std::string contents() const;
  std::string contents(pos_t start, pos_t end) const;
  char at(pos_t pos) const;

  int64_t LineNumberFor(pos_t pos) const;
  std::string LineFor(pos_t pos) const;
  Position PositionFor(pos_t pos) const;

 private:
  File(std::string name, pos_t start, std::string contents);

  std::string name_;
  std::string contents_;
  std::vector<pos_t> line_starts_;

  friend FileSet;
};

class FileSet {
 public:
  Position PositionFor(pos_t pos) const;
  File* FileAt(pos_t pos) const;

  File* AddFile(std::string name, std::string contents);

 private:
  std::vector<std::unique_ptr<class File>> files_;
};

}  // namespace pos
}  // namespace lang

#endif /* lang_positions_h */
