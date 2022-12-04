//
//  issues.h
//  Katara
//
//  Created by Arne Philipeit on 11/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_issues_h
#define ir_issues_h

#include <string>
#include <vector>

#include "src/common/issues/issues.h"
#include "src/common/positions/positions.h"

namespace ir_issues {

enum class Origin {
  kScanner,
  kParser,
  kChecker,
};

enum class IssueKind {
  kScannerStart = 1000,
  kUnexpectedToken,
  kNumberCannotBeRepresented,
  kAddressCannotBeRepresented,
  kEOFInsteadOfEscapedCharacter,
  kEOFInsteadOfStringEndQuote,
  kScannerEnd,

  kParserStart = 2000,
  kDuplicateFuncNumber,
  kDuplicateBlockNumber,
  kUnknownTypeName,
  kUnexpectedAddress,
  kUnexpectedBoolConstant,
  kUnexpectedFuncConstant,
  kUnknownInstructionName,
  kMovInstrDoesNotHaveOneResult,
  kPhiInstrDoesNotHaveOneResult,
  kPhiInstrHasLessThanTwoResults,
  kConvInstrDoesNotHaveOneResult,
  kBoolNotInstrDoesNotHaveOneResult,
  kBoolBinaryInstrDoesNotHaveOneResult,
  kIntUnaryInstrDoesNotHaveOneResult,
  kIntCompareInstrDoesNotHaveOneResult,
  kIntBinaryInstrDoesNotHaveOneResult,
  kIntShiftInstrDoesNotHaveOneResult,
  kPointerOffsetInstrDoesNotHaveOneResult,
  kNilTestInstrDoesNotHaveOneResult,
  kMallocInstrDoesNotHaveOneResult,
  kLoadInstrDoesNotHaveOneResult,
  kStoreInstrHasResults,
  kFreeInstrHasResults,
  kJumpInstrHasResults,
  kJumpCondInstrHasResults,
  kSyscallInstrDoesNotHaveOneResult,
  kReturnInstrHasResults,
  kPanicInstrHasResults,
  kMakeSharedInstrDoesNotHaveOneResult,
  kCopySharedInstrDoesNotHaveOneResult,
  kDeleteSharedInstrHasResults,
  kMakeUniqueInstrDoesNotHaveOneResult,
  kDeleteUniqueInstrHasResults,
  kStringIndexInstrDoesNotHaveOneResult,
  kStringConcatInstrDoesNotHaveOneResult,
  kUnexpectedType,
  kParserEnd,

  kCheckerStart = 3000,

  kCheckerEnd,
};

class Issue : public common::Issue<IssueKind, Origin> {
 public:
  Issue(IssueKind kind, std::vector<common::pos_t> positions, std::string message)
      : common::Issue<IssueKind, Origin>(kind, positions, message) {}

  Origin origin() const override;
  common::Severity severity() const override;
};

typedef common::IssueTracker<IssueKind, Origin, Issue> IssueTracker;

}  // namespace ir_issues

#endif /* ir_issues_h */
