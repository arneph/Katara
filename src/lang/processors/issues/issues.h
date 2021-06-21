//
//  issues.h
//  Katara
//
//  Created by Arne Philipeit on 10/24/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_issues_h
#define lang_issues_h

#include <iostream>
#include <string>
#include <vector>

#include "src/lang/representation/positions/positions.h"

namespace lang {
namespace issues {

enum class Origin {
  kParser,
  kIdentifierResolver,
  kTypeResolver,
  kPackageManager,
};

enum class Severity {
  kWarning,  // compilation can still complete
  kError,    // compilation can partially continue but not complete
  kFatal,    // compilation can not continue
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

Origin OriginOf(IssueKind issue_kind);
Severity SeverityOf(IssueKind issue_kind);

class Issue {
 public:
  Issue(IssueKind kind, std::vector<pos::pos_t> positions, std::string message)
      : kind_(kind), positions_(positions), message_(message) {}

  int64_t kind_id() const { return static_cast<int64_t>(kind_); }
  IssueKind kind() { return kind_; }
  Origin origin() const { return OriginOf(kind_); }
  Severity severity() const { return SeverityOf(kind_); }
  const std::vector<pos::pos_t>& positions() const { return positions_; }
  std::string message() const { return message_; }

 private:
  IssueKind kind_;
  std::vector<pos::pos_t> positions_;
  std::string message_;
};

class IssueTracker {
 public:
  enum class PrintFormat {
    kPlain,
    kTerminal,
  };

  IssueTracker(const pos::FileSet* file_set) : file_set_(file_set) {}

  bool has_warnings() const;
  bool has_errors() const;
  bool has_fatal_errors() const;

  const std::vector<Issue>& issues() const { return issues_; }

  void Add(IssueKind kind, pos::pos_t position, std::string message);
  void Add(IssueKind kind, std::vector<pos::pos_t> positions, std::string message);

  void PrintIssues(PrintFormat format, std::ostream& out) const;

 private:
  const pos::FileSet* file_set_;
  std::vector<Issue> issues_;
};

}  // namespace issues
}  // namespace lang

#endif /* lang_issues_h */
