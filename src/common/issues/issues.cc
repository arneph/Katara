//
//  issues.cpp
//  Katara
//
//  Created by Arne Philipeit on 1/22/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#include "issues.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <string_view>

namespace common::issues {

using ::common::positions::column_t;
using ::common::positions::File;
using ::common::positions::FileSet;
using ::common::positions::line_number_range_t;
using ::common::positions::line_number_t;
using ::common::positions::pos_t;
using ::common::positions::Position;
using ::common::positions::range_t;

std::vector<range_t> PositionsToRanges(std::vector<pos_t> positions) {
  std::vector<range_t> ranges;
  ranges.reserve(positions.size());
  for (pos_t p : positions) {
    ranges.push_back(range_t{.start = p, .end = p});
  }
  return ranges;
}

namespace {

struct SingleLineRanges {
  line_number_t line;
  std::vector<range_t> ranges;
};

struct FileRanges {
  File* file;
  std::vector<SingleLineRanges> single_line_ranges;
  std::vector<range_t> multi_line_ranges;
};

bool CompareRanges(range_t a, range_t b) {
  if (a.start != b.start) {
    return a.start < b.start;
  } else {
    return a.end <= b.end;
  }
};

void SortRanges(std::vector<range_t>& ranges) {
  std::sort(ranges.begin(), ranges.end(), CompareRanges);
}

bool IsSingleLineRange(File* file, range_t range) {
  line_number_range_t line_range = file->LineNumbersOfRange(range);
  return line_range.start == line_range.end;
}

void AddSingleLineRange(range_t range, FileRanges& file_range) {
  line_number_t line = file_range.file->LineNumberOfPosition(range.start);
  auto it = std::find_if(
      file_range.single_line_ranges.begin(), file_range.single_line_ranges.end(),
      [line](const SingleLineRanges& single_line_range) { return single_line_range.line == line; });
  if (it == file_range.single_line_ranges.end()) {
    it = file_range.single_line_ranges.insert(it, SingleLineRanges{.line = line});
  }
  it->ranges.push_back(range);
}

void AddMultiLineRange(range_t range, FileRanges& file_range) {
  file_range.multi_line_ranges.push_back(range);
}

std::vector<FileRanges> GenerateFileRanges(const positions::FileSet* file_set,
                                           const std::vector<range_t>& ranges) {
  std::vector<FileRanges> file_ranges;
  std::for_each(ranges.begin(), ranges.end(), [&file_set, &file_ranges](range_t range) {
    File* file = file_set->FileAt(range.start);
    auto it =
        std::find_if(file_ranges.begin(), file_ranges.end(),
                     [file](const FileRanges& file_range) { return file_range.file == file; });
    if (it == file_ranges.end()) {
      it = file_ranges.insert(it, FileRanges{.file = file});
    }
    if (IsSingleLineRange(file, range)) {
      AddSingleLineRange(range, *it);
    } else {
      AddMultiLineRange(range, *it);
    }
  });
  return file_ranges;
}

void SortSingleLineRanges(std::vector<SingleLineRanges>& single_line_ranges) {
  std::sort(single_line_ranges.begin(), single_line_ranges.end(),
            [](const SingleLineRanges& a, const SingleLineRanges& b) { return a.line < b.line; });
  std::for_each(single_line_ranges.begin(), single_line_ranges.end(),
                [](SingleLineRanges& single_line_range) { SortRanges(single_line_range.ranges); });
}

void SortFileRanges(std::vector<FileRanges>& file_ranges) {
  std::for_each(file_ranges.begin(), file_ranges.end(), [](FileRanges& file_range) {
    SortSingleLineRanges(file_range.single_line_ranges);
    SortRanges(file_range.multi_line_ranges);
  });
}

std::vector<FileRanges> GenerateSortedFileRanges(const positions::FileSet* file_set,
                                                 const std::vector<range_t>& ranges) {
  std::vector<FileRanges> file_ranges = GenerateFileRanges(file_set, ranges);
  SortFileRanges(file_ranges);
  return file_ranges;
}

column_t DetermineLeadingWhitespace(std::string_view line) {
  column_t leading_whitespace = 0;
  for (; leading_whitespace < line.length(); leading_whitespace++) {
    if (line.at(leading_whitespace) != ' ' && line.at(leading_whitespace) != '\t') {
      break;
    }
  }
  return leading_whitespace;
}

column_t DetermineLeadingWhitespace(const std::vector<std::string>& lines) {
  column_t min_leading_whitespace = std::numeric_limits<column_t>::max();
  std::for_each(lines.begin(), lines.end(), [&min_leading_whitespace](std::string_view line) {
    column_t leading_whitespace = DetermineLeadingWhitespace(line);
    if (min_leading_whitespace > leading_whitespace) {
      min_leading_whitespace = leading_whitespace;
    }
  });
  return min_leading_whitespace;
}

std::vector<std::vector<range_t>> DetermineRangeLines(
    const std::vector<range_t>& single_line_ranges) {
  std::vector<std::vector<range_t>> range_lines;
  std::for_each(single_line_ranges.begin(), single_line_ranges.end(),
                [&range_lines](range_t range) {
                  auto it = std::find_if(range_lines.begin(), range_lines.end(),
                                         [range](const std::vector<range_t>& range_line) {
                                           return range_line.back().end < range.start;
                                         });
                  if (it != range_lines.end()) {
                    it->push_back(range);
                  } else {
                    range_lines.push_back(std::vector<range_t>{range});
                  }
                });
  return range_lines;
}

void PrintRepeatedCharacter(char character, column_t count, std::ostream* out) {
  for (column_t c = 0; c < count; c++) {
    *out << character;
  }
}

void PrintSingeLineRanges(File* file, const SingleLineRanges& single_line_ranges,
                          std::ostream* out) {
  Position position;
  if (single_line_ranges.ranges.size() == 1) {
    position = file->PositionFor(single_line_ranges.ranges.front().start);
  } else {
    position = Position(file->name(), single_line_ranges.line);
  }
  std::string line = file->LineWithNumber(single_line_ranges.line);
  range_t line_range = file->RangeOfLineWithNumber(single_line_ranges.line);
  column_t label_space = 4 + position.ToString().length();
  column_t leading_whitespace = DetermineLeadingWhitespace(line);

  *out << "  " << position.ToString() << ": " << line.substr(leading_whitespace) << "\n";

  std::vector<std::vector<range_t>> range_lines = DetermineRangeLines(single_line_ranges.ranges);
  std::for_each(
      range_lines.begin(), range_lines.end(),
      [line_range, label_space, leading_whitespace, out](const std::vector<range_t>& range_line) {
        PrintRepeatedCharacter(' ', label_space, out);
        pos_t current_position = line_range.start + leading_whitespace;
        std::for_each(range_line.begin(), range_line.end(),
                      [&current_position, out](range_t range) {
                        PrintRepeatedCharacter(' ', range.start - current_position, out);
                        if (range.start == range.end) {
                          *out << '^';
                        } else {
                          PrintRepeatedCharacter('~', range.end - range.start + 1, out);
                        }
                        current_position = range.end + 1;
                      });
        *out << '\n';
      });
}

void PrintMultiLineRanges(File* file, range_t multi_line_range, std::ostream* out) {
  Position position = Position(file->name());
  *out << "  " << position.ToString() << ":\n";

  line_number_range_t line_range = file->LineNumbersOfRange(multi_line_range);
  range_t first_line_range = file->RangeOfLineWithNumber(line_range.start);
  range_t last_line_range = file->RangeOfLineWithNumber(line_range.end);
  std::vector<std::string> lines = file->LinesWithNumbers(line_range);

  column_t line_number_identation = 4;
  column_t line_number_space =
      std::max(column_t{2}, static_cast<column_t>(1 + std::log10(line_range.end)));
  column_t vertical_bar_indentation = line_number_identation + line_number_space + 1;
  column_t leading_whitespace = DetermineLeadingWhitespace(lines);

  PrintRepeatedCharacter(' ', vertical_bar_indentation, out);
  *out << "| ";
  PrintRepeatedCharacter(' ', multi_line_range.start - first_line_range.start - leading_whitespace,
                         out);
  *out << 'v';
  PrintRepeatedCharacter('~', first_line_range.end - multi_line_range.start, out);
  *out << '\n';

  for (line_number_t l = line_range.start; l <= line_range.end; l++) {
    PrintRepeatedCharacter(' ', line_number_identation, out);
    *out << std::setw(int(line_number_space)) << std::setfill('0') << l << " | "
         << lines.at(l - line_range.start) << "\n";
  }

  PrintRepeatedCharacter(' ', vertical_bar_indentation, out);
  *out << "| ";
  PrintRepeatedCharacter('~', multi_line_range.end - last_line_range.start - leading_whitespace,
                         out);
  *out << "^\n";
}

void PrintSortedFileRanges(const std::vector<FileRanges>& file_ranges, std::ostream* out) {
  std::for_each(file_ranges.begin(), file_ranges.end(), [out](const FileRanges& file_range) {
    std::for_each(file_range.single_line_ranges.begin(), file_range.single_line_ranges.end(),
                  [file_range, out](const SingleLineRanges& single_line_ranges) {
                    PrintSingeLineRanges(file_range.file, single_line_ranges, out);
                  });
    std::for_each(file_range.multi_line_ranges.begin(), file_range.multi_line_ranges.end(),
                  [file_range, out](range_t multi_line_range) {
                    PrintMultiLineRanges(file_range.file, multi_line_range, out);
                  });
  });
}

}  // namespace

void PrintIssueRanges(const FileSet* file_set, const std::vector<range_t>& ranges,
                      std::ostream* out) {
  std::vector<FileRanges> file_ranges = GenerateSortedFileRanges(file_set, ranges);
  PrintSortedFileRanges(file_ranges, out);
}

}  // namespace common::issues
