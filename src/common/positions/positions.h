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

namespace common::positions {

typedef uint64_t pos_t;
struct range_t {
  pos_t start;
  pos_t end;

  auto operator<=>(const range_t&) const = default;
};

constexpr pos_t kNoPos = 0;
constexpr range_t kNoRange = range_t{
    .start = kNoPos,
    .end = kNoPos,
};

typedef uint64_t line_number_t;
struct line_number_range_t {
  line_number_t start;
  line_number_t end;
};

constexpr line_number_t kNoLineNumber = 0;
constexpr line_number_range_t kNoLineNumberRange = line_number_range_t{
    .start = kNoLineNumber,
    .end = kNoLineNumber,
};

typedef uint64_t column_t;

constexpr line_number_t kNoColumn = 0;

class Position {
 public:
  Position() : Position("", kNoLineNumber, kNoColumn) {}
  Position(line_number_t line) : Position("", line, kNoColumn) {}
  Position(line_number_t line, column_t column) : Position("", line, column) {}
  Position(std::string filename) : Position(filename, kNoLineNumber, kNoColumn) {}
  Position(std::string filename, line_number_t line) : Position(filename, line, kNoColumn) {}
  Position(std::string filename, line_number_t line, column_t column)
      : filename_(filename), line_(line), column_(column) {}

  std::string filename() const { return filename_; }
  line_number_t line() const { return line_; }
  column_t column() const { return column_; }

  bool IsValid() const;

  std::string ToString() const;

 private:
  std::string filename_;
  line_number_t line_;
  column_t column_;
};

class FileSet;

class File {
 public:
  std::string name() const { return name_; }

  pos_t start() const { return line_starts_.front(); }
  pos_t end() const { return line_starts_.front() + contents_.length() - 1; }
  range_t range() const { return range_t{.start = start(), .end = end()}; }

  std::string contents() const { return contents_; }
  std::string contents(range_t range) const;
  char at(pos_t pos) const;

  line_number_t LineNumberOfPosition(pos_t position) const;
  line_number_range_t LineNumbersOfRange(range_t range) const;

  range_t RangeOfLineWithNumber(line_number_t line_number) const;
  range_t RangeOfLinesWithNumbers(line_number_range_t line_number) const;

  std::string LineWithNumber(line_number_t line_number) const;
  std::vector<std::string> LinesWithNumbers(line_number_range_t line_numbers) const;

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

  pos_t NextFileStart() const;
  File* AddFile(std::string name, std::string contents);

 private:
  std::vector<std::unique_ptr<File>> files_;
};

}  // namespace common::positions

#endif /* common_positions_h */
