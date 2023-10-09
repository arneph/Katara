//
//  positions.cc
//  Katara
//
//  Created by Arne Philipeit on 7/4/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "positions.h"

#include "src/common/logging/logging.h"

namespace common::positions {

bool Position::IsValid() const { return line_ > 0; }

std::string Position::ToString() const {
  std::string s;
  if (!filename_.empty()) {
    s += filename_;
  }
  if (IsValid()) {
    if (!s.empty()) {
      s += ":";
    }
    s += std::to_string(line_);
    if (column_ > 0) {
      s += ":" + std::to_string(column_);
    }
  }
  if (s.empty()) {
    s = "-";
  }
  return s;
}

File::File(std::string name, pos_t start, std::string contents) {
  name_ = name;
  contents_ = contents;
  line_starts_.push_back(start);

  for (pos_t i = 0; i < contents_.length(); i++) {
    if (contents_.at(i) == '\n') {
      line_starts_.push_back(start + i + 1);
    }
  }
}

std::string File::contents(range_t position_range) const {
  if (position_range.start < start() || position_range.end > end() ||
      position_range.end < position_range.start) {
    return "";
  }
  std::size_t contents_start = position_range.start - start();
  std::size_t length = position_range.end - position_range.start + 1;
  return contents_.substr(contents_start, length);
}

char File::at(pos_t position) const {
  if (position < start() || position > end()) {
    return '\0';
  }
  return contents_.at(position - start());
}

line_number_t File::LineNumberOfPosition(pos_t position) const {
  if (position == end() + 1) {
    return line_starts_.size();
  } else if (position < start() || position > end()) {
    return kNoLineNumber;
  }
  for (size_t i = 0; i < line_starts_.size() - 1; i++) {
    if (line_starts_.at(i) <= position && position < line_starts_.at(i + 1)) {
      return line_number_t(i + 1);
    }
  }
  return line_starts_.size();
}

line_number_range_t File::LineNumbersOfRange(range_t range) const {
  line_number_t start = LineNumberOfPosition(range.start);
  line_number_t end = LineNumberOfPosition(range.end);
  if (start == kNoLineNumber || end == kNoLineNumber || start > end) {
    return kNoLineNumberRange;
  }
  return line_number_range_t{.start = start, .end = end};
}

range_t File::RangeOfLineWithNumber(line_number_t line_number) const {
  if (line_number < 1 || line_number > line_number_t(line_starts_.size())) {
    return kNoRange;
  }
  pos_t line_start = line_starts_.at(line_number - 1);
  pos_t line_end =
      (line_number < line_number_t(line_starts_.size())) ? line_starts_.at(line_number) - 2 : end();
  return range_t{.start = line_start, .end = line_end};
}

range_t File::RangeOfLinesWithNumbers(line_number_range_t line_numbers) const {
  pos_t first_line_start = line_starts_.at(line_numbers.start - 1);
  pos_t last_line_end = (line_numbers.end < line_number_t(line_starts_.size()))
                            ? line_starts_.at(line_numbers.end) - 2
                            : end();
  return range_t{.start = first_line_start, .end = last_line_end};
}

std::string File::LineWithNumber(line_number_t line_number) const {
  return contents(RangeOfLineWithNumber(line_number));
}

std::vector<std::string> File::LinesWithNumbers(line_number_range_t line_numbers) const {
  std::vector<std::string> lines;
  lines.reserve(line_numbers.end - line_numbers.start + 1);
  for (line_number_t l = line_numbers.start; l <= line_numbers.end; l++) {
    lines.push_back(LineWithNumber(l));
  }
  return lines;
}

Position File::PositionFor(pos_t pos) const {
  line_number_t line = LineNumberOfPosition(pos);
  if (line == kNoLineNumber) {
    return Position();
  }
  column_t column = pos - line_starts_.at(line - 1);
  return Position(name_, line, column);
}

Position FileSet::PositionFor(pos_t pos) const {
  File* file = FileAt(pos);
  if (file) {
    return file->PositionFor(pos);
  }
  return Position();
}

File* FileSet::FileAt(pos_t pos) const {
  for (auto& file : files_) {
    if (file->start() <= pos && pos <= file->end() + 1) {
      return file.get();
    }
  }
  return nullptr;
}

pos_t FileSet::NextFileStart() const {
  if (files_.empty()) {
    return kNoPos + 1;
  } else {
    return files_.back()->end() + 3;
  }
}

File* FileSet::AddFile(std::string name, std::string contents) {
  files_.push_back(std::unique_ptr<File>(new File(name, NextFileStart(), contents)));
  return files_.back().get();
}

}  // namespace common::positions
