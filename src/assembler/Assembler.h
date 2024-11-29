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
using DataTypes::AssemblyResult;
using DataTypes::mnemonics;
using DataTypes::Msg;
using DataTypes::MsgType;
using DataTypes::ProcessorVersion;

class Assembler {
public:
  static AssemblyResult assemble(ProcessorVersion processorVersion, QString &code, std::array<uint8_t, 0x10000> &memory);
  struct NumParseResult {
    bool ok;
    int32_t value;
    QString message;

    static NumParseResult error(const QString &msg) { return {false, 0, msg}; }
  };
  struct NumParseRelativeResult {
    bool ok;
    uint8_t value;
    QString message;
    static NumParseRelativeResult fromParseResult(const NumParseResult &result) { return {result.ok, static_cast<uint8_t>(result.value), result.message}; }
    static NumParseRelativeResult error(const QString &msg) { return {false, 0, msg}; }
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

    static ExpressionEvaluationResult setUndefined(const QString &msg, bool ok) { return {ok, true, 0, msg}; }
    static ExpressionEvaluationResult error(const QString &msg) { return {false, false, 0, msg}; }
  };

private:
  static NumParseResult getNum(const QString &input);
  static NumParseRelativeResult getNumRelative(const QString &input);
  static Msg setLabel(const QString &label, int value, std::unordered_map<QString, int> &labelValMap);
  static ExpressionEvaluationResult expressionEvaluator(const QString &expr, std::unordered_map<QString, int> &labelValMap, bool errOnUndefined);
  static NumParseResult parseNumber(const QString &input);
  static NumParseResult parseASCII(const QString &input);
  static NumParseResult parseDec(const QString &input, bool allowNeg);
  static NumParseResult parseBin(const QString &input);
  static NumParseResult parseHex(const QString &input);
};

#endif // ASSEMBLER_H
