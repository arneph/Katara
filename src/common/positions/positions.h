//
//  positions.h
//  Katara
//
//  Created by Arne Philipeit on 7/4/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef common_positions_h
#define common_positions_h

#include <memory>
#include <string>
#include <vector>

namespace common {

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

class PosFileSet;

class PosFile {
 public:
  std::string name() const { return name_; }
  pos_t start() const { return line_starts_.front(); }
  pos_t end() const { return line_starts_.front() + contents_.length(); }
  std::string contents() const { return contents_; }
  std::string contents(pos_t start, pos_t end) const;
  char at(pos_t pos) const;

  int64_t LineNumberFor(pos_t pos) const;
  std::string LineFor(pos_t pos) const;
  Position PositionFor(pos_t pos) const;

 private:
  PosFile(std::string name, pos_t start, std::string contents);

  std::string name_;
  std::string contents_;
  std::vector<pos_t> line_starts_;

  friend PosFileSet;
};

class PosFileSet {
 public:
  Position PositionFor(pos_t pos) const;
  PosFile* FileAt(pos_t pos) const;

  PosFile* AddFile(std::string name, std::string contents);

 private:
  std::vector<std::unique_ptr<class PosFile>> files_;
};

}  // namespace common

#endif /* common_positions_h */
