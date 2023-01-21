//
//  issues.h
//  Katara
//
//  Created by Arne Philipeit on 10/24/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_issues_h
#define lang_issues_h

#include <string>
#include <vector>

#include "src/common/issues/issues.h"
#include "src/common/positions/positions.h"

namespace lang {
namespace issues {

enum class Origin {
  kParser,
  kIdentifierResolver,
  kTypeResolver,
  kPackageManager,
};

enum IssueKind {
  kParserStart = 1000,

  kParserFatalStart,
  kMissingSemicolonOrNewLine,
  kMissingColon,
  kMissingLParen,
  kMissingRParen,
  kMissingLAngleBrack,
  kMissingRAngleBrack,
  kMissingLBrack,
  kMissingRBrack,
  kMissingLBrace,
  kMissingRBrace,
  kMissingPackageDeclaration,
  kUnexpectedDeclStart,
  kUnexpectedImportAfterNonImportDecl,
  kMissingImportPackagePath,
  kForbiddenLabelExpr,
  kMissingReturn,
  kMissingIf,
  kMissingIfOrLBrace,
  kMissingSwitch,
  kMissingCaseOrDefault,
  kMissingFor,
  kUnexpectedVariableDefinitionInForLoopPostStmt,
  kMissingFallthroughContinueOrBreak,
  kUnexpectedExprAsStmt,
  kMissingAssignmentOp,
  kMissingIncOrDecOp,
  kMissingExpr,
  kMissingSelectionOrAssertedType,
  kMissingCommaOrRBrace,
  kMissingType,
  kMissingFunc,
  kMissingInterface,
  kMissingEmbeddedInterfaceOrMethodSpec,
  kMissingTypeOrInstanceMethodStart,
  kMissingStruct,
  kMissingPointerType,
  kMissingReceiverPointerTypeOrIdentifier,
  kMissingReceiverTypeParameter,
  kForbiddenMixingOfNamedAndUnnamedArguments,
  kMissingLiteral,
  kMissingIdent,
  kParserFatalEnd,

  kParserEnd,
  kIdentifierResolverStart = 2000,

  kIdentifierResolverErrorStart,
  kRedefinitionOfPredeclaredIdent,
  kRedefinitionOfIdent,
  kPackageImportedTwice,
  kPackageCouldNotBeImported,
  kForbiddenBlankTypeName,
  kForbiddenBlankFuncName,
  kForbiddenBlankTypeParameterName,
  kUnresolvedBranchStmtLabel,
  kForbiddenBlankSelectionName,
  kForbiddenEmbeddedFieldType,
  kUnresolvedIdentifier,
  kIdentifierResolverErrorEnd,

  kIdentiferResolverEnd,
  kTypeResolverStart = 3000,

  kTypeResolverErrorStart,
  kUnexpectedTypeDependency,
  kUnexpectedConstantDependency,
  kDependencyLoopForTypeResolver,
  kForbiddenTypeParameterDeclarationForMethod,
  kForbiddenTypeExpression,
  kObjectIsNotTypeName,
  kUnexpectedPointerPrefix,
  kConstantForArraySizeCanNotBeEvaluated,
  kConstantCanNotBeUsedAsArraySize,
  kWrongNumberOfTypeArgumentsForTypeInstance,
  kTypeArgumentCanNotBeUsedForTypeInstanceParameter,
  kTypeParamterConstraintIsNotInterface,
  kReceiverOfNonNamedType,
  kDefinitionOfInterfaceMethodOutsideInterface,
  kRedefinitionOfMethod,
  kWrongNumberOfTypeArgumentsForReceiver,
  kConstantDependsOnNonConstant,
  kMissingTypeOrValueForConstant,
  kConstantWithNonBasicType,
  kConstantValueOfWrongType,
  kConstantExprContainsAddressOp,
  kConstantExprContainsNonPackageSelection,
  kConstantExprContainsTypeAssertion,
  kConstantExprContainsIndexExpr,
  kConstantExprContainsConversionToNonBasicType,
  kConstantExprContainsBuiltinCall,
  kConstantExprContainsFuncCall,
  kConstantExprContainsFuncLit,
  kConstantExprContainsCompositeLit,
  kMissingTypeOrValueForVariable,
  kVariableValueOfWrongType,
  kExprKindIsNotValue,
  kExprTypeIsNotBool,
  kExprTypeIsNotInt,
  kExprTypeIsNotInteger,
  kUnexpectedBasicOperandType,
  kUnexpectedUnaryArithemticOrBitExprOperandType,
  kUnexpectedUnaryLogicExprOperandType,
  kUnexpectedAddressOfExprOperandType,
  kUnexpectedPointerDereferenceExprOperandType,
  kForbiddenWeakDereferenceOfStrongPointer,
  kForbiddenStrongDereferenceOfWeakPointer,
  kUnexpectedAddExprOperandType,
  kMismatchedBinaryExprTypes,
  kUnexpectedBinaryArithmeticOrBitExprOperandType,
  kUnexpectedBinaryShiftExprOperandType,
  kUnexpectedBinaryShiftExprOffsetType,
  kConstantBinaryShiftExprOffsetIsNegative,
  kUnexpectedBinaryLogicExprOperandType,
  kCompareExprOperandTypesNotComparable,
  kCompareExprOperandTypesNotOrderable,
  kUnexpectedSelectionAccessedExprKind,
  kForbiddenSelectionFromPointerToInterfaceOrTypeParameter,
  kUnresolvedSelection,
  kForbiddenBlankTypeAssertionOutsideTypeSwitch,
  kUnexpectedTypeAssertionOperandType,
  kTypeAssertionNeverPossible,
  kUnexpectedIndexOperandType,
  kUnexpectedIndexedOperandType,
  kUnexpectedFuncExprKind,
  kForbiddenTypeArgumentsForTypeConversion,
  kWrongNumberOfArgumentsForTypeConversion,
  kUnexpectedTypeConversionArgumentType,
  kUnexpectedTypeArgumentsForLen,
  kWrongNumberOfArgumentsForLen,
  kUnexpectedLenArgumentType,
  kWrongNumberOfTypeArgumentsForMake,
  kWrongNumberOfArgumentsForMake,
  kUnexpectedTypeArgumentForMake,
  kUnexpectedMakeArgumentType,
  kWrongNumberOfTypeArgumentsForNew,
  kUnexpectedArgumentForNew,
  kUnexpectedFuncCallFuncExprKind,
  kUnexpectedFuncCallFuncType,
  kWrongNumberOfTypeArgumentsForFuncCall,
  kTypeArgumentCanNotBeUsedForFuncTypeParameter,
  kWrongNumberOfArgumentsForFuncCall,
  kUnexpectedFuncCallArgumentType,
  kPackageNameWithoutSelection,
  kForbiddenMultipleStmtLabels,
  kUnexpectedAssignStmtLhsExprKind,
  kMismatchedAssignStmtOperandCountForValueOkRhs,
  kMismatchedAssignStmtValueCount,
  kMismatchedAssignStmtValueType,
  kUnexpectedIncDecStmtOperandType,
  kUnexpectedReturnStmtFuncCallOperandType,
  kMismatchedReturnStmtOperandCount,
  kUnexpectedReturnStmtOperandType,
  kDuplicateDefaultCase,
  kUnexpectedExprCaseValueType,
  kTypeSwitchCaseNeverPossible,
  kUnexpectedBranchStmtBeforeBlockEnd,
  kUnexpectedFallthroughStmt,
  kUnexpectedFallthroughStmtLabel,
  kUnexpectedBreakStmt,
  kUnexpectedBreakStmtLabel,
  kUnexpectedContinueStmt,
  kUnexpectedContinueStmtLabel,
  kTypeResolverErrorEnd,

  kTypeResolverEnd,
  kPackageManagerStart = 4000,

  kPackageManagerWarningStart,
  kPackageDirectoryWithoutSourceFiles,
  kPackageManagerWarningEnd,

  kPackageManagerErrorStart,
  kPackageDirectoryNotFound,
  kMainPackageDirectoryUnreadable,
  kMainPackageFilesInMultipleDirectories,
  kMainPackageFileUnreadable,
  kPackageManagerErrorEnd,

  kPackageManagerEnd,
};

class Issue : public common::issues::Issue<IssueKind, Origin> {
 public:
  Issue(IssueKind kind, std::vector<common::positions::pos_t> positions, std::string message)
      : common::issues::Issue<IssueKind, Origin>(kind, positions, message) {}

  Origin origin() const override;
  common::issues::Severity severity() const override;
};

typedef common::issues::IssueTracker<IssueKind, Origin, Issue> IssueTracker;

}  // namespace issues
}  // namespace lang

#endif /* lang_issues_h */
