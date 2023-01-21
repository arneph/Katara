//
//  positions.cc
//  Katara
//
//  Created by Arne Philipeit on 7/4/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "positions.h"

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

  for (pos_t i = 0; i < int64_t(contents_.length()); i++) {
    if (contents_.at(i) == '\n') {
      line_starts_.push_back(start + i + 1);
    }
  }
}

std::string File::contents(pos_t ps, pos_t pe) const {
  if (ps < start() || pe > end() || pe < ps) {
    return "";
  }
  if (ps == end()) ps--;
  if (pe == end()) pe--;
  int64_t cs = ps - line_starts_.at(0);
  int64_t ce = pe - line_starts_.at(0);
  return contents_.substr(cs, ce - cs + 1);
}

char File::at(pos_t pos) const {
  if (pos == end()) {
    return '\0';
  }
  return contents_.at(pos - line_starts_.at(0));
}

int64_t File::LineNumberFor(pos_t pos) const {
  if (pos < start() || pos > end()) {
    return 0;
  }
  for (size_t i = 0; i < line_starts_.size() - 1; i++) {
    if (line_starts_.at(i) <= pos && pos < line_starts_.at(i + 1)) {
      return i + 1;
    }
  }
  return line_starts_.size();
}

std::string File::LineFor(pos_t pos) const {
  int64_t line = LineNumberFor(pos);
  if (line == 0) {
    return nullptr;
  } else if (line == int64_t(line_starts_.size())) {
    int64_t s = line_starts_.back() - line_starts_.at(0);
    return contents_.substr(s);
  }
  int64_t s = line_starts_.at(line - 1) - line_starts_.at(0);
  int64_t e = line_starts_.at(line) - line_starts_.at(0);
  return contents_.substr(s, e - s);
}

Position File::PositionFor(pos_t pos) const {
  int64_t line = LineNumberFor(pos);
  if (line == 0) {
    return Position();
  }
  int64_t column = pos - line_starts_.at(line - 1);
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
    if (file->start() <= pos && pos <= file->end()) {
      return file.get();
    }
  }
  return nullptr;
}

File* FileSet::AddFile(std::string name, std::string contents) {
  pos_t p = kNoPos + 1;
  if (!files_.empty()) {
    auto& last_file = files_.back();
    p = last_file->end() + 1;
  }
  files_.push_back(std::unique_ptr<File>(new File(name, p, contents)));
  return files_.back().get();
}

}  // namespace common::positions
