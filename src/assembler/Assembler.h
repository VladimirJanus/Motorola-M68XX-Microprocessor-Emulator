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

#include "src/core/Core.h"

#include <QString>

#include <map>
#include <stdint.h>

class Assembler {
public:
  static Core::AssemblyResult assemble(Core::ProcessorVersion processorVersion, const QString &code, std::array<uint8_t, 0x10000> &memory);

private:
  enum ExprOperation {
    PLUS,
    MINUS,
    NONE,
  };

  struct NumParseResult {
    bool ok;
    uint16_t value;
    QString message;

    static NumParseResult success(const uint16_t value) { return {true, value, ""}; }
    static NumParseResult failure(const QString &msg) { return {false, 0, msg}; }
  };
  struct NumParseRelativeResult {
    bool ok;
    int8_t value;
    QString message;
    static NumParseRelativeResult success(int8_t value) {
      return {true, value, ""};
    }
    static NumParseRelativeResult fromParseResult(const NumParseResult &result) {
      return {result.ok, static_cast<int8_t>(result.value), result.message};
    }
    static NumParseRelativeResult failure(const QString &msg) { return {false, 0, msg}; }
  };

  struct ExpressionEvaluationResult {
    bool ok;
    bool undefined;
    uint16_t value;
    QString message;

    static ExpressionEvaluationResult success(const uint16_t value) { return {true, false, value, ""}; }
    static ExpressionEvaluationResult setUndefined(const QString msg, bool ok) { return {ok, true, 0, msg}; }
    static ExpressionEvaluationResult failure(const QString msg) { return {false, false, 0, msg}; }
  };

  struct LineParts {
    QString label;
    QString s_in;
    QString s_op;
  };

  static NumParseResult parseDec(const QString &input);
  static NumParseRelativeResult parseDecRelative(const QString &input);
  static NumParseResult parseHex(const QString &input);
  static NumParseResult parseBin(const QString &input);
  static NumParseResult parseASCII(const QString &input);

  static NumParseResult parseNumber(const QString &input);
  static NumParseRelativeResult parseNumberRelative(const QString &input);

  static ExpressionEvaluationResult expressionEvaluator(QString expr, std::map<QString, int> &labelValMap, bool errOnUndefined);

  static inline bool isLabelOrExpression(QString s_op);
  static bool trimLineAndCheckEmpty(QString &line);

  static void assignLabelValue(const QString &label, int value, std::map<QString, int> &labelValMap, int assemblerLine);

  static Core::MnemonicInfo getMnemonicInfo(const QString &s_in, Core::ProcessorVersion processorVersion, int assemblerLine);

  static void validateInstructionSupport(QString s_in, uint8_t opCode, Core::ProcessorVersion processorVersion, int assemblerLine);
  static void validateMnemonicSupportForAddressingMode(Core::MnemonicInfo &info, Core::AddressingMode mode, int assemblerLine);

  static void errorCheckUnexpectedOperand(QString s_op, int assemblerLine);
  static void errorCheckMissingOperand(QString s_op, int assemblerLine);
  static void errorCheckMissingLabel(QString label, int assemblerLine);
  static void errorCheckOperandContainsIND(QString s_in, QString s_op, int assemblerLine);
  static void errorCheckOperandContainsIMM(QString s_in, QString s_op, int assemblerLine);
  static void errorCheckOperandIMMINDMixed(QString s_op, int assemblerLine);

  static void validateValueRange(int32_t value, int32_t max, int assemblerLine);

  static LineParts disectLine(QString line, int assemblerLine);
};

#endif // ASSEMBLER_H
