/*
 * Copyright (C) 2024 Vladimir Januš
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <QRegularExpression>
#include <QString>
#include "src/utils/DataTypes.h"
using DataTypes::AddressingMode;
using DataTypes::alliasMap;
using DataTypes::AssemblyError;
using DataTypes::AssemblyResult;
using DataTypes::interruptLocations;
using DataTypes::mnemonics;
using DataTypes::Msg;
using DataTypes::MsgType;
using DataTypes::ProcessorVersion;

class Assembler {
public:
  static AssemblyResult assemble(ProcessorVersion processorVersion, QString &code, std::array<uint8_t, 0x10000> &memory);

private:
  struct NumParseResult {
    bool ok;
    int32_t value;
    QString message;

    static NumParseResult success(const int value) { return {true, value, ""}; }
    static NumParseResult failure(const QString &msg) { return {false, 0, msg}; }
  };
  struct NumParseRelativeResult {
    bool ok;
    uint8_t value;
    QString message;
    static NumParseRelativeResult fromParseResult(const NumParseResult &result) { return {result.ok, static_cast<uint8_t>(result.value), result.message}; }
    static NumParseRelativeResult failure(const QString &msg) { return {false, 0, msg}; }
  };
  enum ExprOperation {
    PLUS,
    MINUS,
    NONE,
  };
  struct ExpressionEvaluationResult {
    bool ok;
    bool undefined;
    int32_t value;
    QString message;

    static ExpressionEvaluationResult success(const int value) { return {true, false, value, ""}; }
    static ExpressionEvaluationResult setUndefined(const QString msg, bool ok) { return {ok, true, 0, msg}; }
    static ExpressionEvaluationResult failure(const QString msg) { return {false, false, 0, msg}; }
  };

  typedef struct voidOperationResult {
    bool ok;
    QString message;
    static voidOperationResult success() { return {true, ""}; }
    static voidOperationResult failure(QString msg) { return {false, msg}; }
  } ValidationResult, AssignmentResult, VoidOpRes;

  struct LineParts {
    QString label;
    QString s_in;
    QString s_op;
  };

  static NumParseResult getNum(const QString &input);
  static NumParseRelativeResult getNumRelative(const QString &input);
  static ExpressionEvaluationResult expressionEvaluator(QString expr, std::unordered_map<QString, int> &labelValMap, bool errOnUndefined);
  static NumParseResult parseNumber(const QString &input);
  static NumParseResult parseASCII(const QString &input);
  static NumParseResult parseDec(const QString &input, bool allowNeg);
  static NumParseResult parseBin(const QString &input);
  static NumParseResult parseHex(const QString &input);

  static bool lineEmpty(QString line);
  static LineParts disectLine(QString line, int assemblerLine);
  static void checkGeneralInstructionSupport(QString &s_in, int assemblerLine, ProcessorVersion processorVersion);

  static inline bool isLabelOrExpression(QString s_op);

  static AssignmentResult assignLabelValue(const QString &label, int value, std::unordered_map<QString, int> &labelValMap);
  static ValidationResult validateInstructionSupport(QString s_in, uint8_t opCode, ProcessorVersion processorVersion);
  static ValidationResult validateMnemonicSupportForIMM(QString s_in);
  static ValidationResult validateMnemonicSupportForIND(QString s_in);
  static ValidationResult errorCheckUnexpectedOperand(QString s_op);
  static ValidationResult errorCheckMissingOperand(QString s_op);
  static ValidationResult errorCheckMissingLabel(QString label);
  static ValidationResult errorCheckOperandContainsIND(QString s_in, QString s_op);
  static ValidationResult errorCheckOperandContainsIMM(QString s_in, QString s_op);
  static ValidationResult errorCheckOperandIMMINDMixed(QString s_op);
  static ValidationResult errorCheckValueAboveFF(int value);
};

#endif // ASSEMBLER_H
