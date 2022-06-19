//
//  issues.h
//  Katara
//
//  Created by Arne Philipeit on 2/20/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_checker_issues_h
#define ir_checker_issues_h

#include <ostream>
#include <string>
#include <vector>

#include "src/ir/representation/object.h"

namespace ir_checker {

class Issue {
 public:
  enum class Kind {
    // Value issues:
    kValueHasNullptrType,

    // Instr issues:
    kUnknownInstrKind,
    kInstrDefinesNullptrValue,
    kInstrUsesNullptrValue,
    kNonPhiInstrUsesInheritedValue,
    kMovInstrOriginAndResultHaveMismatchedTypes,
    kPhiInstrArgAndResultHaveMismatchedTypes,
    kPhiInstrHasNoArgumentForParentBlock,
    kPhiInstrHasMultipleArgumentsForParentBlock,
    kPhiInstrHasArgumentForNonParentBlock,
    kConversionOperandHasUnsupportedType,
    kConversionResultHasUnsupportedType,
    kBoolNotInstrOperandDoesNotHaveBoolType,
    kBoolNotInstrResultDoesNotHaveBoolType,
    kBoolBinaryInstrOperandDoesNotHaveBoolType,
    kBoolBinaryInstrResultDoesNotHaveBoolType,
    kIntUnaryInstrOperandDoesNotHaveIntType,
    kIntUnaryInstrResultDoesNotHaveIntType,
    kIntUnaryInstrResultAndOperandHaveDifferentTypes,
    kIntCompareInstrOperandDoesNotHaveIntType,
    kIntCompareInstrOperandsHaveDifferentTypes,
    kIntCompareInstrResultDoesNotHaveBoolType,
    kIntBinaryInstrOperandDoesNotHaveIntType,
    kIntBinaryInstrResultDoesNotHaveIntType,
    kIntBinaryInstrOperandsAndResultHaveDifferentTypes,
    kIntShiftInstrOperandDoesNotHaveIntType,
    kIntShiftInstrResultDoesNotHaveIntType,
    kIntShiftInstrShiftedAndResultHaveDifferentTypes,
    kPointerOffsetInstrPointerDoesNotHavePointerType,
    kPointerOffsetInstrOffsetDoesNotHaveI64Type,
    kPointerOffsetInstrResultDoesNotHavePointerType,
    kNilTestInstrTestedDoesNotHavePointerOrFuncType,
    kNilTestInstrResultDoesNotHaveBoolType,
    kMallocInstrSizeDoesNotHaveI64Type,
    kMallocInstrResultDoesNotHavePointerType,
    kLoadInstrAddressDoesNotHavePointerType,
    kStoreInstrAddressDoesNotHavePointerType,
    kFreeInstrAddressDoesNotHavePointerType,
    kJumpInstrDestinationIsNotChildBlock,
    kJumpCondInstrConditionDoesNotHaveBoolType,
    kJumpCondInstrHasDuplicateDestinations,
    kJumpCondInstrDestinationIsNotChildBlock,
    kSyscallInstrResultDoesNotHaveI64Type,
    kSyscallInstrSyscallNumberDoesNotHaveI64Type,
    kSyscallInstrArgDoesNotHaveI64Type,
    kCallInstrCalleeDoesNotHaveFuncType,
    kCallInstrStaticCalleeDoesNotExist,
    kCallInstrDoesNotMatchStaticCalleeSignature,
    kReturnInstrDoesNotMatchFuncSignature,

    // Block issues:
    kEntryBlockHasParents,
    kNonEntryBlockHasNoParents,
    kBlockContainsNoInstrs,
    kPhiInBlockWithoutMultipleParents,
    kPhiAfterRegularInstrInBlock,
    kControlFlowInstrBeforeEndOfBlock,
    kControlFlowInstrMissingAtEndOfBlock,
    kControlFlowInstrMismatchedWithBlockGraph,

    // Func issues:
    kFuncDefinesNullptrArg,
    kFuncHasNullptrResultType,
    kFuncHasNoEntryBlock,
    kComputedValueUsedInMultipleFunctions,
    kComputedValueNumberUsedMultipleTimes,
    kComputedValueHasNoDefinition,
    kComputedValueHasMultipleDefinitions,
    kComputedValueDefinitionDoesNotDominateUse,

    // Lang issues:
    kLangLoadFromSmartPointerHasMismatchedElementType,
    kLangStoreToSmartPointerHasMismatchedElementType,
  };

  Issue(const ir::Object* scope_object, Kind kind, std::string message)
      : Issue(scope_object, /*involved_objects=*/{}, kind, message) {}
  Issue(const ir::Object* scope_object, std::vector<const ir::Object*> involved_objects, Kind kind,
        std::string message)
      : scope_object_(scope_object),
        involved_objects_(involved_objects),
        kind_(kind),
        message_(message) {}

  const ir::Object* scope_object() const { return scope_object_; }
  std::vector<const ir::Object*> involved_objects() const { return involved_objects_; }
  Kind kind() const { return kind_; }
  std::string message() const { return message_; }

  friend std::ostream& operator<<(std::ostream& os, const Issue& issue);

 private:
  const ir::Object* scope_object_;
  std::vector<const ir::Object*> involved_objects_;
  Kind kind_;
  std::string message_;
};

std::ostream& operator<<(std::ostream& os, const Issue& issue);
std::ostream& operator<<(std::ostream& os, const Issue::Kind& kind);

}  // namespace ir_checker

#endif /* ir_checker_issues_h */
