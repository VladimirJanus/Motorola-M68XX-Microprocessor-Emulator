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
#include "assembler.h"

struct Err {
  inline static const QString invalidHexOrRange(const QString &num) { return "Invalid Hexadecimal number: '" + num + "' or value out of range[0, $FFFF]."; }
  inline static const QString invalidBinOrRange(const QString &num) { return "Invalid Binary number: '" + num + "' or value out of range[0, $FFFF]."; }
  inline static const QString invalidRelDecOrRange(const QString &num) { return "Invalid decimal number: '" + num + "' or value out of range[-128, 127]"; }
  inline static const QString invalidDecOrRange(const QString &num) { return "Invalid decimal number: '" + num + "' or value out of range[0, $FFFF]"; }
  inline static const QString invalidAsciiConversionSyntax() { return "Invalid ASCII conversion syntax. It should be: 'c', where c is a valid ASCII character"; }
  inline static const QString invalidAsciiCharacter(const QString &input) { return "Invalid ASCII character: '" + input + "'"; }

  inline static const QString numOutOfRangeByte(int32_t value) { return "Value out of range for instruction[0, $FF]: " + QString::number(value); }
  inline static const QString numOutOfRangeWord(int32_t value) { return "Value out of range[0, $FFFF]: " + QString::number(value); }
  inline static const QString numOutOfRelRange(int32_t value) { return "Relative address out of range[-128, 127]: " + QString::number(value); }

  inline static const QString exprOverFlow() { return "Expression result out of range[0, $FFFF]"; }
  inline static const QString exprMissingOperation() { return "Missing operation(+/-) in expression"; }
  inline static const QString exprOutOfRange(int32_t value) { return "Expression result out of range[0, $FFFF]: " + QString::number(value); }
  inline static const QString exprUnexpectedCharacter(const QChar &character) { return "Unexpected character in expression: " + QString(character); }

  inline static const QString labelUndefined(const QString &label) { return "Label '" + label + "' is not defined"; }
  inline static const QString labelDefinedTwice(const QString &label) { return "Label already declared: '" + label + "'"; }
  inline static const QString labelReserved(const QString &label) {
    return "'" + label +
           "' is a reserved instruction name and cannot be used as a label. "
           "If you meant to use the instruction, it must be indented with a space or tab:\n"
           "'\tNOP'";
  }
  inline static const QString labelStartsWithDigit(const QChar &character) { return "Label may not start with a digit: '" + QString(character) + "'"; }
  inline static const QString labelStartsWithOther(const QChar &character) { return "Label may not start with character: '" + QString(character) + "'"; }
  inline static const QString labelContainsOther(const QChar &character) { return "Label may not contain character: '" % QString(character) % "'"; }

  inline static const QString parsingEmptyNumber() { return "Missing number."; }
  inline static const QString missingInstruction() { return "Missing instruction."; }
  inline static const QString missingValue() { return "Missing value"; }

  inline static const QString unexpectedChar(const QChar &character) { return "Unexpected character: '" % QString(character) % "'"; }
  inline static const QString unexpectedOperand() { return "Unexpected operand."; }

  inline static const QString instructionUnknown(const QString &s_in) { return "Unknown instruction: '" + s_in + "'"; }
  inline static const QString instructionNotSupported(const QString &s_in) { return "Instruction '" + s_in + "' is not supported on this processor."; }

  inline static const QString invalidSETSyntaxMissingComma(const QString &instruction) {
    return "Invalid " + instruction + " format. Missing comma for address,value separation. Format: ." + instruction + " $FFFF,$FF";
  }
  inline static const QString invalidSETSyntaxExtraComma(const QString &instruction) {
    return "Invalid " + instruction + " format. Too many commas for address,value separation. Format: ." + instruction + " $FFFF,$FF";
  }
  inline static const QString invalidSTRSyntax() { return "Invalid string syntax. Format: .STR \"string\""; }
  inline static const QString invalidINDSyntax() { return "Invalid indexed addressing syntax. Format: LDAA $FF,X"; }
  inline static const QString invalidINDReg(const QString &reg) { return "Invalid index register: '" + reg + "'"; }
  inline static const QString instructionDoesNotSupportDIREXT(const QString &s_in) { return "Instruction '" + s_in + "' does not support direct or extended addressing modes."; }
  inline static const QString instructionDoesNotSupportIND(const QString &s_in) { return "Instruction '" + s_in + "' does not support indexed data"; };
  inline static const QString instructionDoesNotSupportIMM(const QString &s_in) { return "Instruction '" + s_in + "' does not support immediate data"; };
};

Assembler::NumParseResult Assembler::parseHex(const QString &input) {
  if (input.length() <= 1) {
    return NumParseResult::error(Err::parsingEmptyNumber());
  }
  QString trimmedInput = input.sliced(1);
  bool ok;
  int32_t number = trimmedInput.toInt(&ok, 16);
  if (!ok) {
    return NumParseResult::error(Err::invalidHexOrRange(input));
  } else if (number > 0xFFFF || number < 0) {
    return NumParseResult::error(Err::numOutOfRangeWord(number));
  }
  return NumParseResult{true, number, ""};
}
Assembler::NumParseResult Assembler::parseBin(const QString &input) {
  if (input.length() <= 1) {
    return NumParseResult::error(Err::parsingEmptyNumber());
  }
  QString trimmedInput = input.sliced(1);
  bool ok;
  int32_t number = trimmedInput.toInt(&ok, 2);
  if (!ok) {
    return NumParseResult::error(Err::invalidBinOrRange(input));
  } else if (number > 0xFFFF || number < 0) {
    return NumParseResult::error(Err::numOutOfRangeWord(number));
  }
  return {true, number, ""};
}
Assembler::NumParseResult Assembler::parseDec(const QString &input, bool allowNeg) {
  bool ok;
  int32_t number;
  if (allowNeg) {
    number = input.toInt(&ok);
    if (!ok) {
      return NumParseResult::error(Err::invalidRelDecOrRange(input));
    } else if (number > 127 || number < -128) {
      return NumParseResult::error(Err::numOutOfRelRange(number));
    }
  } else {
    number = input.toInt(&ok);
    if (!ok) {
      return NumParseResult::error(Err::invalidDecOrRange(input));
    } else if (number > 0xFFFF || number < 0) {
      return NumParseResult::error(Err::numOutOfRangeWord(number));
    }
  }
  return {true, number, ""};
}
Assembler::NumParseResult Assembler::parseASCII(const QString &input) {
  if (input.length() != 3 || !input.startsWith("'") || !input.endsWith("'")) {
    return NumParseResult::error(Err::invalidAsciiConversionSyntax());
  }
  char16_t number = input[1].unicode();
  if (number >= 128) {
    return NumParseResult::error(Err::invalidAsciiCharacter(input));
  }
  return {true, static_cast<uint8_t>(number), ""};
}
Assembler::NumParseResult Assembler::parseNumber(const QString &input) {
  if (input.contains("'")) {
    return parseASCII(input);
  } else if (input.startsWith('$')) {
    return parseHex(input);
  } else if (input.startsWith('%')) {
    return parseBin(input);
  } else {
    return parseDec(input, false);
  }
}

Assembler::NumParseResult Assembler::getNum(const QString &input) {
  NumParseResult result = parseNumber(input);
  return result;
}

Assembler::NumParseRelativeResult Assembler::getNumRelative(const QString &input) {
  if (input.startsWith("'") || input.startsWith("$") || input.startsWith("%")) {
    NumParseResult result = parseNumber(input);
    if (!result.ok)
      return NumParseRelativeResult::fromParseResult(result);

    if (result.value > 0xFF) {
      return NumParseRelativeResult::error(Err::numOutOfRelRange(result.value));
    }
    if (result.value == 0xFE) {
      return NumParseRelativeResult::error("A relative address of $FE would perform a relative jump to the current instruction, which is not allowed.");
    }
    if (result.value == 0xFF) {
      return NumParseRelativeResult::error("A relative address of $FF would perform a relative jump to the operand of the instruction, which is not allowed");
    }
    return NumParseRelativeResult::fromParseResult(result);
  } else {
    NumParseResult result = parseDec(input, true);
    if (!result.ok) {
      return NumParseRelativeResult::fromParseResult(result);
    }
    if (result.value < 0)
      result.value -= 2;
    if (result.value > 127 || result.value < -128) {
      return NumParseRelativeResult::error(Err::numOutOfRelRange(result.value));
    }
    return NumParseRelativeResult::fromParseResult(result);
  }
}

Assembler::ExpressionEvaluationResult Assembler::expressionEvaluator(const QString &expr, std::unordered_map<QString, int> &labelValMap, bool errOnUndefined) {
  QList<QString> symbols;
  static QRegularExpression exp = QRegularExpression("(?=[+\\-])|(?<=[+\\-])");
  symbols = expr.split(exp, Qt::SplitBehaviorFlags::SkipEmptyParts);

  int32_t value = 0;
  ExprOperation op = PLUS;

  for (QString symbol : symbols) {
    symbol = symbol.trimmed();

    if (symbol == "+") {
      op = PLUS;
    } else if (symbol == "-") {
      op = MINUS;
    } else {
      int32_t operand;
      if (symbol[0].isLetter()) {
        if (labelValMap.count(symbol) == 0) {
          return ExpressionEvaluationResult::setUndefined(Err::labelUndefined(symbol), !errOnUndefined);
        }
        operand = labelValMap[symbol];
      } else if (symbol[0].isDigit() || symbol[0] == '$' || symbol[0] == '\'' || symbol[0] == '%') {
        NumParseResult result = getNum(symbol);
        if (!result.ok) {
          return ExpressionEvaluationResult::error(result.message);
        }
        operand = result.value;
      } else {
        return ExpressionEvaluationResult::error(Err::exprUnexpectedCharacter(symbol[0]));
      }

      if (op == PLUS) {
        if ((operand > 0 && value > INT32_MAX - operand) || (operand < 0 && value < INT32_MIN - operand)) {
          return ExpressionEvaluationResult::error(Err::exprOverFlow());
        }
        value += operand;
      } else if (op == MINUS) {
        if ((operand > 0 && value < INT32_MIN + operand) || (operand < 0 && value > INT32_MAX + operand)) {
          return ExpressionEvaluationResult::error(Err::exprOverFlow());
        }
        value -= operand;
      } else {
        return ExpressionEvaluationResult::error(Err::exprMissingOperation());
      }
      op = NONE;
    }
  }

  if (value > 0xFFFF || value < 0) {
    return ExpressionEvaluationResult::error(Err::exprOutOfRange(value));
  }

  return {true, false, value, ""};
}

Msg Assembler::setLabel(const QString &label, int value, std::unordered_map<QString, int> &labelValMap) {
  if (labelValMap.count(label) != 0) {
    return DataTypes::Msg{MsgType::ERROR, Err::labelDefinedTwice(label)};
  }
  labelValMap[label] = value;
  return DataTypes::Msg{MsgType::DEBUG, "Value '" + QString::number(value) + "' assigned to '" + label + "'"};
}

inline bool checkIsLabelOrExpr(QString s_op) {
  return (s_op[0].isLetter() || s_op.contains('+') || s_op.contains('-'));
}

#define CHECK_INSTRUCTION_SUPPORT(s_in, opCode) \
  if (!getInstructionSupported(processorVersion, opCode)) { \
    messages.append(DataTypes::Msg{MsgType::ERROR, Err::instructionNotSupported(s_in)}); \
    goto end; \
  }
#define CHECK_INSTRUCTION_IMM_SUPPORT(s_in) \
  if (mnemonics[s_in].opCodes[1] == 0) { \
    messages.append(DataTypes::Msg{MsgType::ERROR, Err::instructionDoesNotSupportIMM(s_in)}); \
    goto end; \
  }
#define CHECK_INSTRUCTION_IND_SUPPORT(s_in) \
  if (mnemonics[s_in].opCodes[3] == 0) { \
    messages.append(DataTypes::Msg{MsgType::ERROR, Err::instructionDoesNotSupportIND(s_in)}); \
    goto end; \
  }

#define CHECK_UNEXPECTED_OPERAND(s_op) \
  if (!s_op.isEmpty()) { \
    messages.append(DataTypes::Msg{MsgType::ERROR, Err::unexpectedOperand()}); \
    goto end; \
  }
#define CHECK_OPERAND_MISSING(s_op) \
  if (s_op.isEmpty()) { \
    messages.append(DataTypes::Msg{MsgType::ERROR, "Missing operand. (Instruction requires operand)"}); \
    goto end; \
  }
#define CHECK_LABEL_MISSING(s_op) \
  if (label.isEmpty()) { \
    messages.append(DataTypes::Msg{MsgType::ERROR, "Missing label. (Instruction requires label)"}); \
    goto end; \
  }

#define CHECK_OPERAND_CONTAINS_IND(s_in, s_op) \
  if (s_op.contains(",")) { \
    messages.append(DataTypes::Msg{MsgType::ERROR, Err::instructionDoesNotSupportIND(s_in)}); \
    goto end; \
  }
#define CHECK_OPERAND_CONTAINS_IMM(s_in, s_op) \
  if (s_op.contains("#")) { \
    messages.append(DataTypes::Msg{MsgType::ERROR, Err::instructionDoesNotSupportIMM(s_in)}); \
    goto end; \
  }
#define CHECK_OPERAND_IMM_IND_MIXING(s_op) \
  if (s_op.contains('#') && s_op.contains(',')) { \
    messages.append(DataTypes::Msg{MsgType::ERROR, "Immediate and indexed data cannot be mixed"}); \
    goto end; \
  }

#define CHECK_VALUE_ABOVE_FF(value) \
  if (value > 0xFF) { \
    messages.append(DataTypes::Msg{MsgType::ERROR, Err::numOutOfRangeByte(value)}); \
    goto end; \
  }
/*#define CHECK_VALUE_ABOVE_FFFF(value) \
  if (value > 0xFFFF) { \
    messages.append(DataTypes::Msg{MsgType::ERROR, Err::numOutOfRangeWord(value)}); \
    goto end; \
  }*/

#define SET_LABEL(label, value) \
  if (!label.isEmpty()) { \
    messages.append(setLabel(label, value, labelValMap)); \
    if (messages.last().type == MsgType::ERROR) \
      goto end; \
  }
AssemblyStatus Assembler::assemble(ProcessorVersion processorVersion, QString &code, std::array<uint8_t, 0x10000> &Memory) {
  int assemblerLine = 0;
  uint16_t assemblerAddress = 0;

  const uint16_t interruptLocations = 0xFFFF;

  std::unordered_map<QString, int> labelValMap;
  std::unordered_map<int, QString> callLabelMap;
  std::unordered_map<int, QString> callLabelRelMap;
  std::unordered_map<int, QString> callLabelExtMap;
  InstructionList instructionList;

  const QStringList lines = code.split('\n');
  const int totalLines = lines.count();
  int charNum = 0;
  bool HCFwarn = false;

  QList<Msg> messages;
  int errorCharNum = -1;

  setLabel(QString("IRQ_PTR"), 0xFFF8, labelValMap);
  setLabel(QString("SWI_PTR"), 0xFFFA, labelValMap);
  setLabel(QString("NMI_PTR"), 0xFFFC, labelValMap);
  setLabel(QString("RST_PTR"), 0xFFFE, labelValMap);
  for (; assemblerLine < totalLines; assemblerLine++) {
    QString line = lines[assemblerLine];
    uint16_t instructionAddress = assemblerAddress;
    QString label = "";
    QString s_in = "";
    QString s_op = "";
    charNum = 0;
    uint8_t opCode = 0;
    uint8_t operand1 = 0;
    uint8_t operand2 = 0;
    if (line.isEmpty()) {
      goto skipLine;
    }
    if (line.contains(';')) {
      line = line.split(';')[0];
    }
    if (line.isEmpty()) {
      goto skipLine;
    }
    line = QString('-' + line).trimmed().mid(1);
    if (line.isEmpty()) {
      goto skipLine;
    }
    // labelExtraction
    if (line[0].isLetter()) {
      // extract label
      for (; charNum < line.size(); ++charNum) {
        if (line[charNum].isLetterOrNumber() || line[charNum] == '_') {
          if (charNum == line.size() - 1) {
            messages.append(DataTypes::Msg{MsgType::ERROR, Err::missingInstruction()});
            goto end;
          }
        } else if (line[charNum] == '\t' || line[charNum] == ' ') {
          label = line.sliced(0, charNum).toUpper();
          if (mnemonics.contains(label)) {
            messages.append(DataTypes::Msg{MsgType::ERROR, Err::labelReserved(label)});
            goto end;
          }

          for (; charNum < line.size(); ++charNum) {
            if (line[charNum] == '\t' || line[charNum] == ' ') {
              if (charNum == line.size() - 1) {
                messages.append(DataTypes::Msg{MsgType::ERROR, Err::missingInstruction()});
                goto end;
              }
            } else {
              goto lineDisection;
            }
          }
        } else {
          messages.append(DataTypes::Msg{MsgType::ERROR, Err::labelContainsOther(line[charNum])});
          errorCharNum = charNum;
          goto end;
        }
      }
    } else if (line[0] == '\t' || line[0] == ' ') {
      charNum++;
      for (; charNum < line.size(); ++charNum) {
        if (line[charNum] == '\t' || line[charNum] == ' ') {
          if (charNum == line.size() - 1) {
            messages.append(DataTypes::Msg{MsgType::ERROR, Err::missingInstruction()});
            goto end;
          }
        } else {
          break;
        }
      }
      // no label
    } else if (line[0].isDigit()) {
      messages.append(DataTypes::Msg{MsgType::ERROR, Err::labelStartsWithDigit(line[charNum])});
      errorCharNum = charNum;
      goto end;
    } else {
      messages.append(DataTypes::Msg{MsgType::ERROR, Err::labelStartsWithOther(line[charNum])});
      errorCharNum = charNum;
      goto end;
    }
  lineDisection:
    if (line.sliced(charNum).isEmpty()) {
      messages.append(DataTypes::Msg{MsgType::ERROR, Err::missingInstruction()});
      goto end;
    }
    if (!line[charNum].isLetter() && line[charNum] != '.') {
      messages.append(DataTypes::Msg{MsgType::ERROR, Err::unexpectedChar(line[charNum])});
      errorCharNum = charNum;
      goto end;
    }
    charNum++;
    if (line.sliced(charNum).isEmpty()) {
      messages.append(DataTypes::Msg{MsgType::ERROR, Err::missingInstruction()});
      goto end;
    }
    for (int start = charNum - 1; charNum < line.size(); ++charNum) {
      if (line[charNum].isLetter()) {
        if (charNum == line.size() - 1) {
          s_in = line.sliced(start).toUpper();
          goto operationIdentification;
        }
      } else if (line[charNum] == ' ' || line[charNum] == '\t') {
        s_in = (line.sliced(start, charNum - start)).toUpper();
        charNum++;
        break;
      } else {
        messages.append(DataTypes::Msg{MsgType::ERROR, Err::unexpectedChar(line[charNum])});
        errorCharNum = charNum;
        goto end;
      }
    }

    s_op = line.sliced(charNum);
    if (!s_op.contains('\'')) {
      s_op = s_op.toUpper();
    }
    s_op = s_op.trimmed();
    /*
    immAdr = line[charNum] == '#';
    if (immAdr) {
      charNum++;
      if (line.sliced(charNum).isEmpty() || line[charNum] == ' ' || line[charNum] == '\t') {
        messages.append(DataTypes::Msg{MsgType::ERROR, "Missing operand."});
        goto end;
      }
    }
    if (line[charNum].isDigit()) {
      charNum++;
      while (charNum < line.length()) {
        if (line[charNum] == '\t' || line[charNum] == ' ') { //check end
          while (charNum < line.length()) {
            if (line[charNum] != '\t' && line[charNum] != ' ') {
              errorCharNum = charNum;
              messages.append(DataTypes::Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
              goto end;
            }
            charNum++;
          }
        } else if (!line[charNum].isDigit()) {
          errorCharNum = charNum;
          messages.append(DataTypes::Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
          goto end;
        }

        charNum++;
      }
    } else if (line[charNum] == '$') {
      charNum++;
      while (charNum < line.length()) {
        if (line[charNum] == '\t' || line[charNum] == ' ') { //check end
          while (charNum < line.length()) {
            if (line[charNum] != '\t' && line[charNum] != ' ') {
              errorCharNum = charNum;
              messages.append(DataTypes::Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
              goto end;
            }
            charNum++;
          }
        } else if (!line[charNum].isDigit() && !(line[charNum].toUpper() >= 'A' && line[charNum].toUpper() <= 'F')) {
          errorCharNum = charNum;
          messages.append(DataTypes::Msg{MsgType::ERROR, "Hexadecimal numbers (prefixed by $) may only contain the numbers from 0 to 9 and letters from A to F. Unexpected character: '" % line[charNum] % "'"});
          goto end;
        }

        charNum++;
      }
    } else if (line[charNum] == '%') {
      charNum++;
      while (charNum < line.length()) {
        if (line[charNum] == '\t' || line[charNum] == ' ') { //check end
          while (charNum < line.length()) {
            if (line[charNum] != '\t' && line[charNum] != ' ') {
              errorCharNum = charNum;
              messages.append(DataTypes::Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
              goto end;
            }
            charNum++;
          }
        } else if (line[charNum].isDigit() && line[charNum] != '0' && line[charNum] != '1') {
          errorCharNum = charNum;
          messages.append(DataTypes::Msg{MsgType::ERROR, "Binary numbers (prefixed by %) may only contain the numbers 1 and 0."});
          goto end;
        } else if (line[charNum] != '0' && line[charNum] != '1') {
          errorCharNum = charNum;
          messages.append(DataTypes::Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
          goto end;
        }

        charNum++;
      }
    } else if (line[charNum].isLetter()) { // LABEL
      charNum++;
      while (charNum < line.length()) {
        if (line[charNum] == '\t' || line[charNum] == ' ') { //check end
          while (charNum < line.length()) {
            if (line[charNum] != '\t' && line[charNum] != ' ') {
              errorCharNum = charNum;
              messages.append(DataTypes::Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
              goto end;
            }
            charNum++;
          }
        } else if (!line[charNum].isLetterOrNumber() && line[charNum] != '_') {
          errorCharNum = charNum;
          messages.append(DataTypes::Msg{MsgType::ERROR, "Negative numbers may only be written in decimal."});
          goto end;
        }

        charNum++;
      }
    } else if (line[charNum] == '\'') { //char
      if (line.sliced(charNum).length() < 3 || line[charNum + 2] != '\'') {
        messages.append(DataTypes::Msg{MsgType::ERROR, "Invalid ASCII conversion syntax. It should be: 'c', where c is a valid ASCII character"});
        goto end;
      }
      charNum += 3;
      while (charNum < line.length()) {
        if (line[charNum] != '\t' && line[charNum] != ' ') {
          errorCharNum = charNum;
          messages.append(DataTypes::Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
          goto end;
        }
        charNum++;
      }
    } else if (line[charNum] == '-') { //neg rel decimal
      if (immAdr) {
        errorCharNum = charNum;
        messages.append(DataTypes::Msg{MsgType::ERROR, "Negative numbers may only be used with relative addressing."});
        goto end;
      }
      charNum++;
      if (line.sliced(charNum).isEmpty()) {
        messages.append(DataTypes::Msg{MsgType::ERROR, "Missing operand."});
        goto end;
      }
      if (line[charNum] == '$' || line[charNum] == '%') {
        errorCharNum = charNum;
        messages.append(DataTypes::Msg{MsgType::ERROR, "Negative numbers may only be written in decimal."});
        goto end;
      }
      while (charNum < line.length()) {
        if (line[charNum] == '\t' || line[charNum] == ' ') { //check end
          while (charNum < line.length()) {
            if (line[charNum] != '\t' && line[charNum] != ' ') {
              errorCharNum = charNum;
              messages.append(DataTypes::Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
              goto end;
            }
            charNum++;
          }
        } else if ((line[charNum].toUpper() >= 'A' && line[charNum].toUpper() <= 'F')) {
          errorCharNum = charNum;
          messages.append(DataTypes::Msg{MsgType::ERROR, "Negative numbers may only be written in decimal."});
          goto end;
        } else if (!line[charNum].isDigit()) {
          errorCharNum = charNum;
          messages.append(DataTypes::Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
          goto end;
        }
        charNum++;
      }
    } else if (line[charNum] == '"') { //string
      if (s_in != ".STR" && s_in != "STR") {
        errorCharNum = charNum;
        messages.append(DataTypes::Msg{MsgType::ERROR, "Strings may not be used outside the .STR directive."});
        goto end;
      }
      if (immAdr) {
        errorCharNum = charNum - 1;
        messages.append(DataTypes::Msg{MsgType::ERROR, "Assembler directives do not support addressing modes like immediate (prefix: #)"});
        goto end;
      }
      while (charNum < line.length()) {
        if (line[charNum] == '\t' || line[charNum] == ' ') {
          break;
        }
        charNum++;
      }
      if (line[charNum - 1] != '"') {
        errorCharNum = charNum - 1;
        messages.append(DataTypes::Msg{MsgType::ERROR, "Strings must begin and end with a quotes."});
        goto end;
      }
      while (charNum < line.length()) {
        if (line[charNum] != '\t' && line[charNum] != ' ') {
          errorCharNum = charNum;
          messages.append(DataTypes::Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
          goto end;
        }
        charNum++;
      }
    } else {
      errorCharNum = charNum;
      messages.append(DataTypes::Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
      goto end;
    }
*/
  operationIdentification:

    if (alliasMap.contains(s_in)) {
      if (!(alliasMap[s_in].supportedVersions & processorVersion)) {
        messages.append(DataTypes::Msg{MsgType::ERROR, Err::instructionNotSupported(s_in)});
        goto end;
      }
      s_in = alliasMap[s_in].mnemonic;
    }
    if (!mnemonics.contains(s_in)) {
      messages.append(DataTypes::Msg{MsgType::ERROR, Err::instructionUnknown(s_in)});
      goto end;
    }
    if (!(mnemonics[s_in].supportedVersions & processorVersion)) {
      messages.append(DataTypes::Msg{MsgType::ERROR, Err::instructionNotSupported(s_in)});
      goto end;
    }

    if (s_in[0] == '.') {
      if (s_in == ".BYTE") {
        CHECK_OPERAND_MISSING(s_op);
        SET_LABEL(label, assemblerAddress);

        QStringList opList = s_op.split(",");
        foreach (QString curOp, opList) {
          if (curOp.isEmpty()) {
            messages.append(DataTypes::Msg{MsgType::ERROR, Err::missingValue()});
            goto end;
          }
          int value = 0;
          if (checkIsLabelOrExpr(curOp)) {
            auto result = expressionEvaluator(curOp, labelValMap, true);
            if (!result.ok) {
              messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
              goto end;
            }
            value = result.value;
          } else {
            NumParseResult result = getNum(curOp);
            if (!result.ok) {
              messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
              goto end;
            }
            value = result.value;
          }

          CHECK_VALUE_ABOVE_FF(value);

          Memory[assemblerAddress++] = value;
        }
        goto skipLine;
      } else if (s_in == ".EQU") {
        CHECK_OPERAND_MISSING(s_op);
        CHECK_LABEL_MISSING(s_op);

        int value = 0;
        if (checkIsLabelOrExpr(s_op)) {
          auto result = expressionEvaluator(s_op, labelValMap, true);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          value = result.value;
        } else {
          NumParseResult result = getNum(s_op);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          value = result.value;
        }

        //CHECK_VALUE_ABOVE_FFFF(value);

        SET_LABEL(label, value);

        goto skipLine;
      } else if (s_in == ".ORG") {
        CHECK_OPERAND_MISSING(s_op);

        int value = 0;
        if (checkIsLabelOrExpr(s_op)) {
          auto result = expressionEvaluator(s_op, labelValMap, true);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          value = result.value;
        } else {
          NumParseResult result = getNum(s_op);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          value = result.value;
        }

        //CHECK_VALUE_ABOVE_FFFF(value);

        Memory[interruptLocations - 1] = (value & 0xFF00) >> 8;
        Memory[interruptLocations] = value & 0xFF;
        assemblerAddress = value;

        SET_LABEL(label, assemblerAddress);

        goto skipLine;
      } else if (s_in == ".WORD") {
        CHECK_OPERAND_MISSING(s_op);
        SET_LABEL(label, assemblerAddress);

        QStringList opList = s_op.split(",");
        foreach (QString curOp, opList) {
          if (curOp.isEmpty()) {
            messages.append(DataTypes::Msg{MsgType::ERROR, Err::missingValue()});
            goto end;
          }

          int value = 0;
          if (checkIsLabelOrExpr(curOp)) {
            auto result = expressionEvaluator(curOp, labelValMap, true);
            if (!result.ok) {
              messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
              goto end;
            }
            value = result.value;
          } else {
            NumParseResult result = getNum(curOp);
            if (!result.ok) {
              messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
              goto end;
            }
            value = result.value;
          }

          //CHECK_VALUE_ABOVE_FFFF(value);

          operand1 = (value >> 8) & 0xFF;
          operand2 = value & 0xFF;

          Memory[assemblerAddress++] = operand1;
          Memory[assemblerAddress++] = operand2;
        }
        goto skipLine;
      } else if (s_in == ".RMB") {
        SET_LABEL(label, assemblerAddress);

        CHECK_OPERAND_MISSING(s_op);
        int value = 0;
        if (checkIsLabelOrExpr(s_op)) {
          auto result = expressionEvaluator(s_op, labelValMap, true);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          value = result.value;
        } else {
          NumParseResult result = getNum(s_op);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          value = result.value;
        }

        //CHECK_VALUE_ABOVE_FFFF(value);

        assemblerAddress += value;

        goto skipLine;
      } else if (s_in == ".SETW") {
        int commaCount = s_op.count(",");
        if (commaCount == 0) {
          messages.append(DataTypes::Msg{MsgType::ERROR, Err::invalidSETSyntaxMissingComma("SETW")});
          goto end;
        } else if (s_op.count(",") != 1) {
          messages.append(DataTypes::Msg{MsgType::ERROR, Err::invalidSETSyntaxExtraComma("SETW")});
          goto end;
        }

        QString adrOp = s_op.split(",")[0];
        if (adrOp.isEmpty()) {
          messages.append(DataTypes::Msg{MsgType::ERROR, Err::missingValue()});
          goto end;
        }
        int adr = 0;
        if (checkIsLabelOrExpr(adrOp)) {
          auto result = expressionEvaluator(adrOp, labelValMap, true);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          adr = result.value;
        } else {
          NumParseResult result = getNum(adrOp);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          adr = result.value;
        }

        //CHECK_VALUE_ABOVE_FFFF(adr);

        QString valOp = s_op.split(",")[1];
        if (valOp.isEmpty()) {
          messages.append(DataTypes::Msg{MsgType::ERROR, Err::missingValue()});
          goto end;
        }
        int val = 0;
        if (checkIsLabelOrExpr(valOp)) {
          auto result = expressionEvaluator(valOp, labelValMap, true);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          val = result.value;
        } else {
          NumParseResult result = getNum(valOp);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          val = result.value;
        }

        //CHECK_VALUE_ABOVE_FFFF(val);

        operand1 = (val >> 8) & 0xFF;
        operand2 = val & 0xFF;
        Memory[adr] = operand1;
        Memory[adr + 1] = operand2;

        SET_LABEL(label, adr);

        goto skipLine;
      } else if (s_in == ".SETB") {
        int commaCount = s_op.count(",");
        if (commaCount == 0) {
          messages.append(DataTypes::Msg{MsgType::ERROR, Err::invalidSETSyntaxMissingComma("SETB")});
          goto end;
        } else if (s_op.count(",") != 1) {
          messages.append(DataTypes::Msg{MsgType::ERROR, Err::invalidSETSyntaxExtraComma("SETB")});
          goto end;
        }

        QString adrOp = s_op.split(",")[0];
        if (adrOp.isEmpty()) {
          messages.append(DataTypes::Msg{MsgType::ERROR, Err::missingValue()});
          goto end;
        }

        int adr = 0;
        if (checkIsLabelOrExpr(adrOp)) {
          auto result = expressionEvaluator(adrOp, labelValMap, true);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          adr = result.value;
        } else {
          NumParseResult result = getNum(adrOp);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          adr = result.value;
        }

        //CHECK_VALUE_ABOVE_FFFF(adr);

        QString valOp = s_op.split(",")[1];
        if (valOp.isEmpty()) {
          messages.append(DataTypes::Msg{MsgType::ERROR, Err::missingValue()});
          goto end;
        }
        int val = 0;
        if (checkIsLabelOrExpr(valOp)) {
          auto result = expressionEvaluator(valOp, labelValMap, true);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          val = result.value;
        } else {
          NumParseResult result = getNum(valOp);
          if (!result.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
            goto end;
          }
          val = result.value;
        }

        //CHECK_VALUE_ABOVE_FFFF(val);

        operand1 = (val >> 8) & 0xFF;
        operand2 = val & 0xFF;
        Memory[adr] = operand1;
        Memory[adr + 1] = operand2;

        SET_LABEL(label, adr);

        goto skipLine;
      } else if (s_in == ".STR") {
        CHECK_OPERAND_MISSING(s_op);

        SET_LABEL(label, assemblerAddress);
        if (s_op[0] != '"' || s_op.length() < 3) {
          messages.append(DataTypes::Msg{MsgType::ERROR, Err::invalidSTRSyntax()});
          goto end;
        }
        if (s_op.at(s_op.length() - 1) != '"') {
          messages.append(DataTypes::Msg{MsgType::ERROR, Err::invalidSTRSyntax()});
          goto end;
        }
        s_op = s_op.mid(1, s_op.length() - 2);
        for (int i = 0; i < s_op.length(); i++) {
          char16_t number = s_op.at(i).unicode();
          if (number >= 128) {
            messages.append(DataTypes::Msg{MsgType::ERROR, Err::invalidAsciiCharacter(QString(s_op.at(i)))});
            goto end;
          }
          Memory[assemblerAddress++] = static_cast<uint8_t>(number);
        }

        goto skipLine;
      }
    }

    SET_LABEL(label, assemblerAddress);

    if (mnemonics[s_in].opCodes[0] != 0) { // VSE
      CHECK_UNEXPECTED_OPERAND(s_op);

      opCode = mnemonics[s_in].opCodes[0];
      CHECK_INSTRUCTION_SUPPORT(s_in, opCode);

      Memory[assemblerAddress++] = opCode;
      goto skipLine;
    }

    CHECK_OPERAND_MISSING(s_op);

    if (mnemonics[s_in].opCodes[5] != 0) { // REL
      CHECK_OPERAND_CONTAINS_IMM(s_in, s_op);
      CHECK_OPERAND_CONTAINS_IND(s_in, s_op);

      opCode = mnemonics[s_in].opCodes[5];
      CHECK_INSTRUCTION_SUPPORT(s_in, opCode);

      if (s_op[0].isLetter()) {
        callLabelRelMap[assemblerAddress + 1] = s_op;
        operand1 = 0;
      } else {
        NumParseRelativeResult resultVal = getNumRelative(s_op);
        if (!resultVal.ok) {
          messages.append(DataTypes::Msg{MsgType::ERROR, resultVal.message});
          goto end;
        }
        operand1 = resultVal.value;
      }
      Memory[assemblerAddress++] = opCode;
      Memory[assemblerAddress++] = operand1;
      goto skipLine;
    }
    if (s_op.contains(',')) { // IND
      CHECK_OPERAND_IMM_IND_MIXING(s_op);

      CHECK_INSTRUCTION_IND_SUPPORT(s_in);
      opCode = mnemonics[s_in].opCodes[3];
      CHECK_INSTRUCTION_SUPPORT(s_in, opCode);

      if (s_op.length() < 2 || s_op.count(',') != 1 || s_op[s_op.length() - 2] != ',') {
        messages.append(DataTypes::Msg{MsgType::ERROR, Err::invalidINDSyntax()});
        goto end;
      }
      if (!s_op[s_op.length() - 1].isLetter() || s_op[s_op.length() - 1] != 'X') {
        messages.append(DataTypes::Msg{MsgType::ERROR, Err::invalidINDReg(QString(s_op[s_op.length() - 1]))});
      }
      if (s_op[0] == ',') {
        if (s_op.length() != 2) {
          messages.append(DataTypes::Msg{MsgType::ERROR, Err::invalidINDSyntax()});
          goto end;
        }
        operand1 = 0;
      } else if (checkIsLabelOrExpr(s_op)) {
        s_op.chop(2);
        callLabelMap[(assemblerAddress + 1) % 0x10000] = s_op;
      } else {
        s_op.chop(2);

        NumParseResult resultVal = getNum(s_op);
        if (!resultVal.ok) {
          messages.append(DataTypes::Msg{MsgType::ERROR, resultVal.message});
          goto end;
        }
        int value = resultVal.value;

        CHECK_VALUE_ABOVE_FF(value);
        operand1 = value;
      }

      Memory[assemblerAddress++] = opCode;
      Memory[assemblerAddress++] = operand1;
      goto skipLine;
    }
    if (s_op.startsWith("#")) { // IMM
      s_op = s_op.sliced(1);

      CHECK_INSTRUCTION_IMM_SUPPORT(s_in);
      opCode = mnemonics[s_in].opCodes[1];
      CHECK_INSTRUCTION_SUPPORT(s_in, opCode);

      if (getInstructionMode(processorVersion, opCode) == AddressingMode::IMMEXT) {
        if (checkIsLabelOrExpr(s_op)) {
          callLabelExtMap[(assemblerAddress + 1) % 0x10000] = s_op;
        } else {
          NumParseResult resultVal = getNum(s_op);
          if (!resultVal.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, resultVal.message});
            goto end;
          }
          int value = resultVal.value;

          //CHECK_VALUE_ABOVE_FFFF(value);
          operand1 = (value >> 8) & 0xFF;
          operand2 = value & 0xFF;
        }
        Memory[assemblerAddress++] = opCode;
        Memory[assemblerAddress++] = operand1;
        Memory[assemblerAddress++] = operand2;
      } else {
        if (checkIsLabelOrExpr(s_op)) {
          callLabelMap[(assemblerAddress + 1) % 0x10000] = s_op;
        } else {
          NumParseResult resultVal = getNum(s_op);
          if (!resultVal.ok) {
            messages.append(DataTypes::Msg{MsgType::ERROR, resultVal.message});
            goto end;
          }
          int value = resultVal.value;

          CHECK_VALUE_ABOVE_FF(value);
          operand1 = value;
        }
        Memory[assemblerAddress++] = opCode;
        Memory[assemblerAddress++] = operand1;
      }
      goto skipLine;
    }
    if (true) { // DIR/EXT
      bool skipDir = false;
      int value = 0;
      if (checkIsLabelOrExpr(s_op)) {
        auto result = expressionEvaluator(s_op, labelValMap, false);
        if (!result.ok) {
          messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
          goto end;
        }
        if (result.undefined) {
          skipDir = true;
          callLabelExtMap[(assemblerAddress + 1) % 0x10000] = s_op;
        } else {
          value = result.value;
        }
      } else {
        NumParseResult resultVal = getNum(s_op);
        if (!resultVal.ok) {
          messages.append(DataTypes::Msg{MsgType::ERROR, resultVal.message});
          goto end;
        }
        value = resultVal.value;
      }
      //CHECK_VALUE_ABOVE_FFFF(value);
      if (value > 0xFF) {
        skipDir = true;
      }

      if (mnemonics[s_in].opCodes[2] != 0 && !skipDir) {
        opCode = mnemonics[s_in].opCodes[2];
        if (getInstructionSupported(processorVersion, opCode)) {
          operand1 = value;
          Memory[assemblerAddress++] = opCode;
          Memory[assemblerAddress++] = operand1;
          goto skipLine;
        }
      }
      if (mnemonics[s_in].opCodes[4] != 0) {
        opCode = mnemonics[s_in].opCodes[4];
        CHECK_INSTRUCTION_SUPPORT(s_in, opCode);

        operand1 = (value >> 8) & 0xFF;
        operand2 = value & 0xFF;
        Memory[assemblerAddress++] = opCode;
        Memory[assemblerAddress++] = operand1;
        Memory[assemblerAddress++] = operand2;
      } else {
        messages.append(DataTypes::Msg{MsgType::ERROR, Err::instructionDoesNotSupportDIREXT(s_in)});
        goto end;
      }
      goto skipLine;
    }
  skipLine:
    if (opCode == 0x9D || opCode == 0xDD) {
      HCFwarn = true;
    }
    if (instructionAddress > 0xFFF0) {
      messages.append({MsgType::WARN, QString("Instruction on line: %1 overwrites input buffers or interrupt vectors.").arg(assemblerLine)});
    }
    if (assemblerAddress == instructionAddress) {
      instructionList.addInstruction(-1, assemblerLine, opCode, operand1, operand2, s_in, s_op);
    } else {
      instructionList.addInstruction(instructionAddress, assemblerLine, opCode, operand1, operand2, s_in, s_op);
    }
  }
  // naj compiler pokaze na katere lineje je nedifenrane onej
  for (const auto &entry : callLabelMap) {
    int location = entry.first;
    QString expr = entry.second;
    auto &instruction = instructionList.getObjectByAddress(location - 1);
    assemblerLine = instruction.lineNumber;
    auto result = expressionEvaluator(expr, labelValMap, true);
    if (!result.ok) {
      messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
      goto end;
    }
    /*if (labelValMap.count(label) == 0) {
      assemblerLine = instruction.lineNumber;
      messages.append(DataTypes::Msg{MsgType::ERROR, "Use of undeclared label: '" + label + "'"});
      goto end;
    }*/
    //int value = labelValMap[label];

    CHECK_VALUE_ABOVE_FF(result.value);
    result.value = result.value & 0xFF;
    Memory[location] = result.value;
    instruction.byte2 = result.value;
  }
  for (const auto &entry : callLabelExtMap) {
    int location = entry.first;
    QString expr = entry.second;
    auto &instruction = instructionList.getObjectByAddress(location - 1);
    assemblerLine = instruction.lineNumber;
    auto result = expressionEvaluator(expr, labelValMap, true);
    if (!result.ok) {
      messages.append(DataTypes::Msg{MsgType::ERROR, result.message});
      goto end;
    }
    //CHECK_VALUE_ABOVE_FFFF(result.value)

    Memory[location] = (result.value >> 8) & 0xFF;
    Memory[location + 1] = result.value & 0xFF;
    instruction.byte2 = Memory[location];
    instruction.byte3 = Memory[location + 1];
  }
  for (const auto &entry : callLabelRelMap) {
    int location = entry.first;
    QString label = entry.second;
    auto &instruction = instructionList.getObjectByAddress(location - 1);
    assemblerLine = instruction.lineNumber;
    if (labelValMap.count(label) == 0) {
      assemblerLine = instruction.lineNumber;
      if (label.contains('+') || label.contains('-')) {
        messages.append(DataTypes::Msg{MsgType::ERROR, "Cannot use expressions with relative addressing."});
      } else {
        messages.append(DataTypes::Msg{MsgType::ERROR, Err::labelUndefined(label)});
      }

      goto end;
    } else {
      int location2 = labelValMap[label];
      int value;
      value = location2 - location - 1;
      if (value > 127 || value < -128) {
        messages.append(DataTypes::Msg{MsgType::ERROR, Err::numOutOfRelRange(value)});
        goto end;
      }
      qint8 signedValue = static_cast<qint8>(value);
      value = signedValue & 0xFF;
      Memory[location] = value;
      instruction.byte2 = value;
    }
  }

end:
  if (messages.length() > 0 && messages.last().type != MsgType::ERROR && HCFwarn) {
    messages.append(DataTypes::Msg{MsgType::WARN,
                                   "Instructions 0x9D and 0xDD are undefined for the M6800 and would cause processor lockup (Halt and Catch Fire) on real hardware. If you are "
                                   "using a M6803 or similar then this warning is irrelevant."});
  }
  for (int i = messages.size() - 1; i >= 0; --i) {
    if (messages[i].type == MsgType::NONE) {
      messages.removeAt(i);
    }
  }
  return {messages, errorCharNum, assemblerLine, instructionList};
}
