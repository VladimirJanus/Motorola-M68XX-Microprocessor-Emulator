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

#include <QString>
#include "src/core/Core.h"
using Core::AddressingMode;
using Core::addressingModes;
using Core::alliasMap;
using Core::AssemblyError;
using Core::AssemblyMap;
using Core::AssemblyResult;
using Core::directivesWithLocation;
using Core::getInfoByMnemonic;
using Core::getInfoByOpCode;
using Core::interruptLocations;
using Core::isMnemonic;
using Core::MnemonicInfo;
using Core::mnemonics;
using Core::Msg;
using Core::MsgType;
using Core::ProcessorVersion;

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

  struct LineParts {
    QString label;
    QString s_in;
    QString s_op;
  };

  static NumParseResult parseDec(const QString &input, bool allowNeg);
  static NumParseResult parseHex(const QString &input);
  static NumParseResult parseBin(const QString &input);
  static NumParseResult parseASCII(const QString &input);

  static NumParseResult parseNumber(const QString &input);
  static NumParseRelativeResult parseNumberRelative(const QString &input);

  static ExpressionEvaluationResult expressionEvaluator(QString expr, std::map<QString, int> &labelValMap, bool errOnUndefined);

  static inline bool isLabelOrExpression(QString s_op);
  static bool trimLineAndCheckEmpty(QString &line);

  static void assignLabelValue(const QString &label, int value, std::map<QString, int> &labelValMap, int assemblerLine);

  static MnemonicInfo getMnemonicInfo(QString &s_in, ProcessorVersion processorVersion, int assemblerLine);

  static void validateInstructionSupport(QString s_in, uint8_t opCode, ProcessorVersion processorVersion, int assemblerLine);
  static void validateMnemonicSupportForAddressingMode(MnemonicInfo &info, AddressingMode mode, int assemblerLine);

  static void errorCheckUnexpectedOperand(QString s_op, int assemblerLine);
  static void errorCheckMissingOperand(QString s_op, int assemblerLine);
  static void errorCheckMissingLabel(QString label, int assemblerLine);
  static void errorCheckOperandContainsIND(QString s_in, QString s_op, int assemblerLine);
  static void errorCheckOperandContainsIMM(QString s_in, QString s_op, int assemblerLine);
  static void errorCheckOperandIMMINDMixed(QString s_op, int assemblerLine);

  static void validateValueRange(int32_t value, int32_t max, int assemblerLine);

  static LineParts disectLine(QString line, int assemblerLine);

  //struct for defining possible assembly errors
  struct Err {
    // Number Conversion Errors
    inline static const QString invalidHexOrRange(const QString &num) { return "Invalid Hexadecimal number: '" + num + "' or value out of range[0, $FFFF]."; }
    inline static const QString invalidBinOrRange(const QString &num) { return "Invalid Binary number: '" + num + "' or value out of range[0, $FFFF]."; }
    inline static const QString invalidRelDecOrRange(const QString &num) { return "Invalid decimal number: '" + num + "' or value out of range[-128, 127]"; }
    inline static const QString invalidDecOrRange(const QString &num) { return "Invalid decimal number: '" + num + "' or value out of range[0, $FFFF]"; }

    // ASCII Conversion Errors
    inline static const QString invalidAsciiConversionSyntax() { return "Invalid ASCII conversion syntax. It should be: 'c', where c is a valid ASCII character"; }
    inline static const QString invalidAsciiCharacter(const QString &input) { return "Invalid ASCII character: '" + input + "'"; }

    // Range Check Errors
    inline static const QString numOutOfRange(int32_t value, int32_t range) { return "Value out of range for instruction[0, $" + QString::number(range, 16).toUpper() + "]: $" + QString::number(value, 16); }
    inline static const QString numOutOfRelRange(int32_t value) { return "Relative address out of range[-128, 127]: " + QString::number(value); }

    // Expression Parsing Errors
    inline static const QString exprOverFlow() { return "Expression result out of range[0, $FFFF]"; }
    inline static const QString exprMissingOperation() { return "Missing operation(+/-) in expression"; }
    inline static const QString exprMissingValue() { return "Missing value after operation (+/-) in expression"; }
    inline static const QString exprOutOfRange(int32_t value) { return "Expression result out of range[0, $FFFF]: $" + QString::number(value, 16); }
    inline static const QString exprUnexpectedCharacter(const QChar &character) { return "Unexpected character in expression: " + QString(character); }

    // Label Errors
    inline static const QString labelUndefined(const QString &label) { return "Label '" + label + "' is not defined"; }
    inline static const QString labelDefinedTwice(const QString &label) { return "Label already declared: '" + label + "'"; }
    inline static const QString labelReserved(const QString &label) {
      return "'" + label +
             "' is a reserved instruction name and cannot be used as a label. "
             "If you meant to use the instruction, it must be indented with a space or tab:\n"
             "'\tNOP'";
    }
    inline static const QString labelStartsWithIllegalDigit(const QChar &character) { return "Label may not start with a digit: '" + QString(character) + "'"; }
    inline static const QString labelStartsWithIllegalCharacter(const QChar &character) { return "Label may not start with character: '" + QString(character) + "'"; }
    inline static const QString labelContainsIllegalCharacter(const QChar &character) { return "Label may not contain character: '" + QString(character) + "'"; }

    // Parsing Errors
    inline static const QString parsingEmptyNumber() { return "Missing number."; }
    inline static const QString unexpectedChar(const QChar &character) { return "Unexpected character: '" + QString(character) + "'"; }
    inline static const QString missingInstruction() { return "Missing instruction."; }
    inline static const QString missingValue() { return "Missing value"; }

    // Operand and Instruction Errors
    inline static const QString missingOperand() { return "Missing operand. (Instruction requires operand)"; }
    inline static const QString missingLabel() { return "Missing label. (Instruction requires label)"; }
    inline static const QString unexpectedOperand() { return "Unexpected operand."; }
    inline static const QString instructionUnknown(const QString &s_in) { return "Unknown instruction: '" + s_in + "'"; }
    inline static const QString instructionDoesNotSupportProcessor(const QString &s_in) { return "Instruction '" + s_in + "' is not supported on this processor."; }

    // Syntax-Specific Errors
    inline static const QString invalidSETSyntaxMissingComma(const QString &instruction) { return "Invalid " + instruction + " format. Missing comma for address,value separation. Format: ." + instruction + " $FFFF,$FF"; }
    inline static const QString invalidSETSyntaxExtraComma(const QString &instruction) { return "Invalid " + instruction + " format. Too many commas for address,value separation. Format: ." + instruction + " $FFFF,$FF"; }
    inline static const QString invalidSTRSyntax() { return "Invalid string syntax. Format: .STR \"string\""; }
    inline static const QString invalidINDSyntax() { return "Invalid indexed addressing syntax. Format: LDAA $FF,X"; }
    inline static const QString invalidINDReg(const QString &reg) { return "Invalid index register: '" + reg + "'"; }

    // Addressing Mode Errors
    inline static const QString mnemonicDoesNotSupportAddressingMode(const QString &s_in, AddressingMode mode) {
      QString msg = "Instruction '" + s_in + "' does not support ";
      switch (mode) {
      case AddressingMode::INH:
        msg.append("inherited");
        break;
      case AddressingMode::IMM:
      case AddressingMode::IMMEXT:
        msg.append("immediate");
        break;
      case AddressingMode::IND:
        msg.append("indexed");
        break;
      case AddressingMode::DIR:
      case AddressingMode::EXT:
        msg.append("direct or extended");
        break;
      case AddressingMode::REL:
        msg.append("relative");
        break;
      case AddressingMode::INVALID:
        throw std::invalid_argument("INVALID PASSED FOR CHECKING MNEMONIC SUPPORT");
      }
      msg.append(" addressing.");
      return msg;
    }
    inline static const QString mixedIMMandIND() { return "Immediate and indexed data may not be mixed"; }
  };
};

#endif // ASSEMBLER_H
