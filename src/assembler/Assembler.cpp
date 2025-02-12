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
#include "src/assembler/Assembler.h"

/**
* Converts hexadecimal string prefixed with '$' to numeric value
* 
* @param input Hexadecimal number string (e.g., '$FF')
* @return Parsed number or failure result
*/
Assembler::NumParseResult Assembler::parseHex(const QString &input) {
  if (input.length() <= 1) {
    return NumParseResult::failure(Err::parsingEmptyNumber());
  }
  QString trimmedInput = input.sliced(1);
  bool ok;
  int32_t number = trimmedInput.toInt(&ok, 16);
  if (!ok) {
    return NumParseResult::failure(Err::invalidHexOrRange(input));
  } else if (number > 0xFFFF || number < 0) {
    return NumParseResult::failure(Err::numOutOfRange(number, 0xFFFF));
  }
  return NumParseResult::success(number);
}

/**
* Converts binary string prefixed with '%' to numeric value
* 
* @param input Binary number string (e.g., '%10101010')
* @return Parsed number or failure result
*/
Assembler::NumParseResult Assembler::parseBin(const QString &input) {
  if (input.length() <= 1) {
    return NumParseResult::failure(Err::parsingEmptyNumber());
  }
  QString trimmedInput = input.sliced(1);
  bool ok;
  int32_t number = trimmedInput.toInt(&ok, 2);
  if (!ok) {
    return NumParseResult::failure(Err::invalidBinOrRange(input));
  } else if (number > 0xFFFF || number < 0) {
    return NumParseResult::failure(Err::numOutOfRange(number, 0xFFFF));
  }
  return NumParseResult::success(number);
}

/**
* Converts decimal string to numeric value
* 
* @param input decimal number string
* @param allowNeg flag to permit negative numbers (for relative addressing)
* @return Parsed number or failure result
*/
Assembler::NumParseResult Assembler::parseDec(const QString &input, bool allowNeg) {
  bool ok;
  int32_t number = input.toInt(&ok);
  if (allowNeg) {
    if (!ok) {
      return NumParseResult::failure(Err::invalidRelDecOrRange(input));
    } else if (number > 127 || number < -128) {
      return NumParseResult::failure(Err::numOutOfRelRange(number));
    }
  } else {
    if (!ok) {
      return NumParseResult::failure(Err::invalidDecOrRange(input));
    } else if (number > 0xFFFF || number < 0) {
      return NumParseResult::failure(Err::Err::numOutOfRange(number, 0xFFFF));
    }
  }
  return NumParseResult::success(number);
}

/**
* Converts ASCII character literal to numeric value
* 
* @param input Character literal (e.g., 'A')
* @return Parsed ASCII value or failure result
*/
Assembler::NumParseResult Assembler::parseASCII(const QString &input) {
  if (input.length() != 3 || !input.startsWith("'") || !input.endsWith("'")) {
    return NumParseResult::failure(Err::invalidAsciiConversionSyntax());
  }
  char16_t number = input[1].unicode();
  if (number >= 128) {
    return NumParseResult::failure(Err::invalidAsciiCharacter(input));
  }
  return NumParseResult::success(static_cast<uint8_t>(number));
}

/**
* Universal number parsing method
* 
* @param input Number string in various formats
* @return Parsed number or failure result
* 
* Supports: ASCII literal, Hex ($FF), Binary (%10101010), Decimal
*/
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
/**
* Converts an operand to a number for relative addressing
* 
* @param input Operand string in various formats
* @return Parsed relative address or failure result
* 
* Handles numeric and literal inputs, with specific restrictions:
* - Rejects values $FE and $FF for relative jumps
* - Constrains value to -128 to 127 range
* - Adjusts negative values for offset calculation
*/
Assembler::NumParseRelativeResult Assembler::getNumRelative(const QString &input) {
  if (input.startsWith("'") || input.startsWith("$") || input.startsWith("%")) {
    NumParseResult result = parseNumber(input);
    if (!result.ok)
      return NumParseRelativeResult::fromParseResult(result);
    if (result.value > 0xFF) {
      return NumParseRelativeResult::failure(Err::numOutOfRelRange(result.value));
    }
    if (result.value == 0xFE) {
      return NumParseRelativeResult::failure("A relative address of $FE would perform a relative jump to the current instruction, which is not allowed.");
    }
    if (result.value == 0xFF) {
      return NumParseRelativeResult::failure("A relative address of $FF would perform a relative jump to the operand of the instruction, which is not allowed");
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
      return NumParseRelativeResult::failure(Err::numOutOfRelRange(result.value));
    }
    return NumParseRelativeResult::fromParseResult(result);
  }
}
/**
 * Evaluates an expression and returns its value
 * 
 * @param expr Expression to evaluate
 * @param labelValMap Map of label values
 * @param errOnUndefined Controls undefined label handling
 * @return Evaluation result
 * 
 * If errOnUndefined:
 * - Returns unsuccessfully (ok=false, undefined=true) if expression contains undefined label
 * Otherwise:
 * - Returns (ok=true, undefined=true) for undefined labels
 */
Assembler::ExpressionEvaluationResult Assembler::expressionEvaluator(QString expr, std::map<QString, int> &labelValMap, bool errOnUndefined) {
  QList<QString> symbols;

  for (int i = expr.length() - 1; i >= 0; --i) {
    if (expr[i] == '+' || expr[i] == '-') {
      QString part = expr.mid(i).trimmed();
      if (!part.mid(1).isEmpty()) {
        symbols.prepend(part.mid(1));
      }
      symbols.prepend(part.mid(0, 1));
      expr = expr.left(i);
    }
  }

  if (!expr.isEmpty()) {
    symbols.prepend(expr.trimmed());
  }

  int32_t value = 0;
  ExprOperation op = PLUS;

  for (QString symbol : symbols) {
    symbol = symbol.trimmed();

    if (symbol == "+") {
      if (op != NONE) {
        return ExpressionEvaluationResult::failure(Err::exprMissingValue());
      }
      op = PLUS;
    } else if (symbol == "-") {
      if (op != NONE) {
        return ExpressionEvaluationResult::failure(Err::exprMissingValue());
      }
      op = MINUS;
    } else {
      int32_t operand;
      if (symbol[0].isLetter()) {
        if (labelValMap.count(symbol) == 0) {
          return ExpressionEvaluationResult::setUndefined(Err::labelUndefined(symbol), !errOnUndefined);
        }
        operand = labelValMap[symbol];
      } else if (symbol[0].isDigit() || symbol[0] == '$' || symbol[0] == '\'' || symbol[0] == '%') {
        NumParseResult result = parseNumber(symbol);
        if (!result.ok) {
          return ExpressionEvaluationResult::failure(result.message);
        }
        operand = result.value;
      } else {
        return ExpressionEvaluationResult::failure(Err::exprUnexpectedCharacter(symbol[0]));
      }

      if (op == PLUS) {
        if ((operand > 0 && value > INT32_MAX - operand) || (operand < 0 && value < INT32_MIN - operand)) {
          return ExpressionEvaluationResult::failure(Err::exprOverFlow());
        }
        value += operand;
      } else if (op == MINUS) {
        if ((operand > 0 && value < INT32_MIN + operand) || (operand < 0 && value > INT32_MAX + operand)) {
          return ExpressionEvaluationResult::failure(Err::exprOverFlow());
        }
        value -= operand;
      } else {
        return ExpressionEvaluationResult::failure(Err::exprMissingOperation());
      }
      op = NONE;
    }
  }
  if (op != NONE) {
    return ExpressionEvaluationResult::failure(Err::exprMissingValue());
  }
  if (value > 0xFFFF || value < 0) {
    return ExpressionEvaluationResult::failure(Err::exprOutOfRange(value));
  }

  return ExpressionEvaluationResult::success(value);
}

//returns true if a string represents a label or an expression
inline bool Assembler::isLabelOrExpression(QString s_op) {
  return (s_op[0].isLetter() || s_op.contains('+') || s_op.contains('-'));
}

//adds a label and its value to the label-to-value map.
void Assembler::assignLabelValue(const QString &label, int value, std::map<QString, int> &labelValMap, int assemblerLine) {
  if (label.isEmpty()) {
    return;
  }
  if (labelValMap.count(label) != 0) {
    throw AssemblyError::failure(Err::labelDefinedTwice(label), assemblerLine, -1);
  }
  labelValMap[label] = value;
  return;
}
//validations and error checks.
void Assembler::validateInstructionSupport(QString s_in, uint8_t opCode, ProcessorVersion processorVersion, int assemblerLine) {
  if (!getInstructionSupported(processorVersion, opCode)) {
    throw AssemblyError::failure(Err::instructionDoesNotSupportProcessor(s_in), assemblerLine, -1);
  }
  return;
}
void Assembler::validateMnemonicSupportForAddressingMode(QString s_in, AddressingMode mode, int assemblerLine) {
  if (mnemonics[s_in].opCodes[addressingModes[mode].id] == 0) {
    throw AssemblyError::failure(Err::mnemonicDoesNotSupportAddressingMode(s_in, mode), assemblerLine, -1);
  }
  return;
}

void Assembler::errorCheckUnexpectedOperand(QString s_op, int assemblerLine) {
  if (!s_op.isEmpty()) {
    throw AssemblyError::failure(Err::unexpectedOperand(), assemblerLine, -1);
  }
  return;
}
void Assembler::errorCheckMissingOperand(QString s_op, int assemblerLine) {
  if (s_op.isEmpty()) {
    throw AssemblyError::failure(Err::missingOperand(), assemblerLine, -1);
  }
  return;
}
void Assembler::errorCheckMissingLabel(QString label, int assemblerLine) {
  if (label.isEmpty()) {
    throw AssemblyError::failure(Err::missingLabel(), assemblerLine, -1);
  }
  return;
}
void Assembler::errorCheckOperandContainsIND(QString s_in, QString s_op, int assemblerLine) {
  if (s_op.contains(',')) {
    throw AssemblyError::failure(Err::mnemonicDoesNotSupportAddressingMode(s_in, AddressingMode::IND), assemblerLine, -1);
  }
  return;
}
void Assembler::errorCheckOperandContainsIMM(QString s_in, QString s_op, int assemblerLine) {
  if (s_op.contains('#')) {
    throw AssemblyError::failure(Err::mnemonicDoesNotSupportAddressingMode(s_in, AddressingMode::IMM), assemblerLine, -1);
  }
  return;
}
void Assembler::errorCheckOperandIMMINDMixed(QString s_op, int assemblerLine) {
  if (s_op.contains('#') && s_op.contains(',')) {
    throw AssemblyError::failure(Err::mixedIMMandIND(), assemblerLine, -1);
  }
  return;
}
void Assembler::validateValueRange(int32_t value, int32_t max, int assemblerLine) {
  if (value > 0xFF) {
    throw AssemblyError::failure(Err::numOutOfRange(value, max), assemblerLine, -1);
  }
  return;
}

void Assembler::checkGeneralInstructionSupport(QString &s_in, int assemblerLine, ProcessorVersion processorVersion) {
  if (alliasMap.contains(s_in)) {
    if (!(alliasMap[s_in].supportedVersions & processorVersion)) {
      throw AssemblyError::failure(Err::instructionDoesNotSupportProcessor(s_in), assemblerLine, -1);
    }
    s_in = alliasMap[s_in].mnemonic;
  }
  if (!mnemonics.contains(s_in)) {
    throw AssemblyError::failure(Err::instructionUnknown(s_in), assemblerLine, -1);
  }
  if (!(mnemonics[s_in].supportedVersions & processorVersion)) {
    throw AssemblyError::failure(Err::instructionDoesNotSupportProcessor(s_in), assemblerLine, -1);
  }
}

//assembles input code and returns a memory image containing the machine code.
AssemblyResult Assembler::assemble(ProcessorVersion processorVersion, QString &code, std::array<uint8_t, 0x10000> &Memory) {
  int assemblerLine = 0;
  uint16_t assemblerAddress = 0;

  std::map<QString, int> labelValMap;
  std::map<int, QString> callLabelMap;
  std::map<int, QString> callLabelRelMap;
  std::map<int, QString> callLabelExtMap;

  QList<Msg> messages;
  AssemblyError assemblyError = AssemblyError::none();
  AssemblyMap assemblyMap;

  bool HCFwarn = false; //should the assembler warn that there are potential Halt-and-Catch-Fire instructions in the machine code

  const QStringList lines = code.split('\n');
  const int totalLines = lines.count();

  //sets predifned labels for interrupt pointers
  assignLabelValue(QString("IRQ_PTR"), 0xFFF8, labelValMap, assemblerLine);
  assignLabelValue(QString("SWI_PTR"), 0xFFFA, labelValMap, assemblerLine);
  assignLabelValue(QString("NMI_PTR"), 0xFFFC, labelValMap, assemblerLine);
  assignLabelValue(QString("RST_PTR"), 0xFFFE, labelValMap, assemblerLine);

  try {
    for (; assemblerLine < totalLines; assemblerLine++) {
      QString line = lines[assemblerLine];
      uint8_t opCode = 0;
      uint8_t operand1 = 0;
      uint8_t operand2 = 0;

      if (assemblerAddress > 0xFFF0) {
        messages.append({MsgType::WARN, QString("Instruction on line: %1 overwrites input buffers or interrupt vectors.").arg(assemblerLine)});
      }

      if (lineEmpty(line)) {
        continue;
      }
      LineParts parts = disectLine(line, assemblerLine);
      QString &label = parts.label;
      QString &s_in = parts.s_in;
      QString &s_op = parts.s_op;

      checkGeneralInstructionSupport(s_in, assemblerLine, processorVersion);

      uint16_t instructionAddress = assemblerAddress;

      if (s_in[0] == '.') {
        if (s_in == ".BYTE") {
          errorCheckMissingOperand(s_op, assemblerLine);

          assignLabelValue(label, assemblerAddress, labelValMap, assemblerLine);

          QStringList opList = s_op.split(",");
          foreach (QString curOp, opList) {
            if (curOp.isEmpty()) {
              throw AssemblyError::failure(Err::missingValue(), assemblerLine, -1);
            }

            auto result = expressionEvaluator(curOp, labelValMap, true);
            if (!result.ok) {
              throw AssemblyError::failure(result.message, assemblerLine, -1);
            }
            int value = result.value;

            validateValueRange(value, 0xFF, assemblerLine);

            Memory[assemblerAddress++] = value;
          }
        } else if (s_in == ".EQU") {
          errorCheckMissingOperand(s_op, assemblerLine);
          errorCheckMissingLabel(s_op, assemblerLine);

          auto result = expressionEvaluator(s_op, labelValMap, true);
          if (!result.ok) {
            throw AssemblyError::failure(result.message, assemblerLine, -1);
          }
          int value = result.value;

          assignLabelValue(label, value, labelValMap, assemblerLine);

        } else if (s_in == ".ORG") {
          errorCheckMissingOperand(s_op, assemblerLine);

          auto result = expressionEvaluator(s_op, labelValMap, true);
          if (!result.ok) {
            throw AssemblyError::failure(result.message, assemblerLine, -1);
          }
          int value = result.value;

          Memory[interruptLocations - 1] = (value & 0xFF00) >> 8;
          Memory[interruptLocations] = value & 0xFF;
          assemblerAddress = value;

          assignLabelValue(label, assemblerAddress, labelValMap, assemblerLine);

        } else if (s_in == ".WORD") {
          errorCheckMissingOperand(s_op, assemblerLine);

          assignLabelValue(label, assemblerAddress, labelValMap, assemblerLine);

          QStringList opList = s_op.split(",");
          foreach (QString curOp, opList) {
            if (curOp.isEmpty()) {
              throw AssemblyError::failure(Err::missingValue(), assemblerLine, -1);
            }

            NumParseResult result = parseNumber(curOp);
            if (!result.ok) {
              throw AssemblyError::failure(result.message, assemblerLine, -1);
            }
            int value = result.value;

            operand1 = (value >> 8) & 0xFF;
            operand2 = value & 0xFF;

            Memory[assemblerAddress++] = operand1;
            Memory[assemblerAddress++] = operand2;
          }

        } else if (s_in == ".RMB") {
          errorCheckMissingOperand(s_op, assemblerLine);

          assignLabelValue(label, assemblerAddress, labelValMap, assemblerLine);

          NumParseResult result = parseNumber(s_op);
          if (!result.ok) {
            throw AssemblyError::failure(result.message, assemblerLine, -1);
          }
          int value = result.value;

          assemblerAddress += value;

        } else if (s_in == ".SETW") {
          if (s_op.count(",") != 1) {
            throw AssemblyError::failure(Err::invalidSETSyntaxMissingComma("SETW"), assemblerLine, -1);
          }

          QString adrOp = s_op.split(",")[0];
          if (adrOp.isEmpty()) {
            throw AssemblyError::failure(Err::missingValue(), assemblerLine, -1);
          }

          NumParseResult result = parseNumber(adrOp);
          if (!result.ok) {
            throw AssemblyError::failure(result.message, assemblerLine, -1);
          }
          int adr = result.value;

          //CHECK_VALUE_ABOVE_FFFF(adr);

          QString valOp = s_op.split(",")[1];
          if (valOp.isEmpty()) {
            throw AssemblyError::failure(Err::missingValue(), assemblerLine, -1);
          }

          result = parseNumber(valOp);
          if (!result.ok) {
            throw AssemblyError::failure(result.message, assemblerLine, -1);
          }
          int val = result.value;

          //CHECK_VALUE_ABOVE_FFFF(val);

          operand1 = (val >> 8) & 0xFF;
          operand2 = val & 0xFF;
          Memory[adr] = operand1;
          Memory[adr + 1] = operand2;

          assignLabelValue(label, adr, labelValMap, assemblerLine);

        } else if (s_in == ".SETB") {
          if (s_op.count(",") != 1) {
            throw AssemblyError::failure(Err::invalidSETSyntaxMissingComma("SETB"), assemblerLine, -1);
          }

          QString adrOp = s_op.split(",")[0];
          if (adrOp.isEmpty()) {
            throw AssemblyError::failure(Err::missingValue(), assemblerLine, -1);
          }

          NumParseResult result = parseNumber(adrOp);
          if (!result.ok) {
            throw AssemblyError::failure(result.message, assemblerLine, -1);
          }
          int adr = result.value;

          QString valOp = s_op.split(",")[1];
          if (valOp.isEmpty()) {
            throw AssemblyError::failure(Err::missingValue(), assemblerLine, -1);
          }

          result = parseNumber(valOp);
          if (!result.ok) {
            throw AssemblyError::failure(result.message, assemblerLine, -1);
          }
          int val = result.value;

          validateValueRange(val, 0xFF, assemblerLine);
          Memory[adr] = val;

          assignLabelValue(label, adr, labelValMap, assemblerLine);

        } else if (s_in == ".STR") {
          errorCheckMissingOperand(s_op, assemblerLine);

          assignLabelValue(label, assemblerAddress, labelValMap, assemblerLine);

          if (s_op[0] != '"' || s_op.length() < 3) {
            throw AssemblyError::failure(Err::invalidSTRSyntax(), assemblerLine, -1);
          }
          if (s_op.at(s_op.length() - 1) != '"') {
            throw AssemblyError::failure(Err::invalidSTRSyntax(), assemblerLine, -1);
          }
          s_op = s_op.mid(1, s_op.length() - 2);
          for (int i = 0; i < s_op.length(); i++) {
            char16_t number = s_op.at(i).unicode();
            if (number >= 128) {
              throw AssemblyError::failure(Err::invalidAsciiCharacter(QString(s_op.at(i))), assemblerLine, -1);
            }
            Memory[assemblerAddress++] = static_cast<uint8_t>(number);
          }
        }

        bool hasLocation = (directivesWithLocation.contains(s_in));
        assemblyMap.addInstruction(hasLocation ? instructionAddress : -1, assemblerLine, opCode, operand1, operand2, s_in, s_op);
      } else {
        assignLabelValue(label, assemblerAddress, labelValMap, assemblerLine);

        if (mnemonics[s_in].opCodes[addressingModes[AddressingMode::INH].id] != 0) { // INH
          errorCheckUnexpectedOperand(s_op, assemblerLine);

          opCode = mnemonics[s_in].opCodes[addressingModes[AddressingMode::INH].id];
          validateInstructionSupport(s_in, opCode, processorVersion, assemblerLine);

          Memory[assemblerAddress++] = opCode;
        } else {
          errorCheckMissingOperand(s_op, assemblerLine);

          if (mnemonics[s_in].opCodes[addressingModes[AddressingMode::REL].id] != 0) { // REL
            errorCheckOperandContainsIMM(s_in, s_op, assemblerLine);
            errorCheckOperandContainsIND(s_in, s_op, assemblerLine);

            opCode = mnemonics[s_in].opCodes[addressingModes[AddressingMode::REL].id];
            validateInstructionSupport(s_in, opCode, processorVersion, assemblerLine);

            if (s_op[0].isLetter()) {
              callLabelRelMap[assemblerAddress + 1] = s_op;
              operand1 = 0;
            } else {
              NumParseRelativeResult resultVal = getNumRelative(s_op);
              if (!resultVal.ok)
                throw AssemblyError::failure(resultVal.message, assemblerLine, -1);

              operand1 = resultVal.value;
            }
            Memory[assemblerAddress++] = opCode;
            Memory[assemblerAddress++] = operand1;
          } else if (s_op.contains(',')) { // IND
            errorCheckOperandIMMINDMixed(s_op, assemblerLine);

            validateMnemonicSupportForAddressingMode(s_in, AddressingMode::IND, assemblerLine);
            opCode = mnemonics[s_in].opCodes[addressingModes[AddressingMode::IND].id];
            validateInstructionSupport(s_in, opCode, processorVersion, assemblerLine);

            if (s_op.length() < 2 || s_op.count(',') != 1 || s_op[s_op.length() - 2] != ',') {
              throw AssemblyError::failure(Err::invalidINDSyntax(), assemblerLine, -1);
            }
            if (!s_op[s_op.length() - 1].isLetter() || s_op[s_op.length() - 1] != 'X') {
              throw AssemblyError::failure(Err::invalidINDReg(QString(s_op[s_op.length() - 1])), assemblerLine, -1);
            }
            if (s_op[0] == ',') {
              if (s_op.length() != 2) {
                throw AssemblyError::failure(Err::invalidINDSyntax(), assemblerLine, -1);
              }
              operand1 = 0;
            } else if (isLabelOrExpression(s_op)) {
              s_op.chop(2);
              callLabelMap[(assemblerAddress + 1) % 0x10000] = s_op;
            } else {
              s_op.chop(2);

              NumParseResult resultVal = parseNumber(s_op);
              if (!resultVal.ok) {
                throw AssemblyError::failure(resultVal.message, assemblerLine, -1);
              }
              int value = resultVal.value;

              validateValueRange(value, 0xFF, assemblerLine);

              operand1 = value;
            }

            Memory[assemblerAddress++] = opCode;
            Memory[assemblerAddress++] = operand1;
          } else if (s_op.startsWith("#")) { // IMM
            s_op = s_op.sliced(1);

            validateMnemonicSupportForAddressingMode(s_in, AddressingMode::IMM, assemblerLine);
            opCode = mnemonics[s_in].opCodes[addressingModes[AddressingMode::IMM].id];
            validateInstructionSupport(s_in, opCode, processorVersion, assemblerLine);

            if (getInstructionMode(processorVersion, opCode) == AddressingMode::IMMEXT) {
              if (isLabelOrExpression(s_op)) {
                callLabelExtMap[(assemblerAddress + 1) % 0x10000] = s_op;
              } else {
                NumParseResult resultVal = parseNumber(s_op);
                if (!resultVal.ok) {
                  throw AssemblyError::failure(resultVal.message, assemblerLine, -1);
                }
                int value = resultVal.value;

                operand1 = (value >> 8) & 0xFF;
                operand2 = value & 0xFF;
              }
              Memory[assemblerAddress++] = opCode;
              Memory[assemblerAddress++] = operand1;
              Memory[assemblerAddress++] = operand2;
            } else {
              if (isLabelOrExpression(s_op)) {
                callLabelMap[(assemblerAddress + 1) % 0x10000] = s_op;
              } else {
                NumParseResult resultVal = parseNumber(s_op);
                if (!resultVal.ok) {
                  throw AssemblyError::failure(resultVal.message, assemblerLine, -1);
                }
                int value = resultVal.value;

                validateValueRange(value, 0xFF, assemblerLine);

                operand1 = value;
              }
              Memory[assemblerAddress++] = opCode;
              Memory[assemblerAddress++] = operand1;
            }
          } else { // DIR/EXT
            bool skipDir = false;
            int value = 0;
            if (isLabelOrExpression(s_op)) {
              auto result = expressionEvaluator(s_op, labelValMap, false);
              if (!result.ok) {
                throw AssemblyError::failure(result.message, assemblerLine, -1);
              }

              if (result.undefined) {
                skipDir = true;
                callLabelExtMap[(assemblerAddress + 1) % 0x10000] = s_op;
              } else {
                value = result.value;
              }
            } else {
              NumParseResult resultVal = parseNumber(s_op);
              if (!resultVal.ok) {
                throw AssemblyError::failure(resultVal.message, assemblerLine, -1);
              }
              value = resultVal.value;
            }

            if (value > 0xFF) {
              skipDir = true;
            }

            if (mnemonics[s_in].opCodes[addressingModes[AddressingMode::DIR].id] != 0 && !skipDir) {
              opCode = mnemonics[s_in].opCodes[addressingModes[AddressingMode::DIR].id];
              if (getInstructionSupported(processorVersion, opCode)) {
                operand1 = value;
                Memory[assemblerAddress++] = opCode;
                Memory[assemblerAddress++] = operand1;
              } else if (mnemonics[s_in].opCodes[addressingModes[AddressingMode::EXT].id] != 0) {
                opCode = mnemonics[s_in].opCodes[addressingModes[AddressingMode::EXT].id];
                validateInstructionSupport(s_in, opCode, processorVersion, assemblerLine);

                operand1 = (value >> 8) & 0xFF;
                operand2 = value & 0xFF;
                Memory[assemblerAddress++] = opCode;
                Memory[assemblerAddress++] = operand1;
                Memory[assemblerAddress++] = operand2;
              } else {
                throw AssemblyError::failure(Err::mnemonicDoesNotSupportAddressingMode(s_in, AddressingMode::EXT), assemblerLine, -1);
              }
            } else if (mnemonics[s_in].opCodes[addressingModes[AddressingMode::EXT].id] != 0) {
              opCode = mnemonics[s_in].opCodes[addressingModes[AddressingMode::EXT].id];
              validateInstructionSupport(s_in, opCode, processorVersion, assemblerLine);

              operand1 = (value >> 8) & 0xFF;
              operand2 = value & 0xFF;
              Memory[assemblerAddress++] = opCode;
              Memory[assemblerAddress++] = operand1;
              Memory[assemblerAddress++] = operand2;
            } else {
              throw AssemblyError::failure(Err::mnemonicDoesNotSupportAddressingMode(s_in, AddressingMode::EXT), assemblerLine, -1);
            }
          }
        }
        if (opCode == 0x9D || opCode == 0xDD) {
          HCFwarn = true;
        }
        assemblyMap.addInstruction(instructionAddress, assemblerLine, opCode, operand1, operand2, s_in, s_op);
      }
    }
    // naj compiler pokaze na katere lineje je nedifenrane onej
    for (const auto &entry : callLabelMap) {
      int location = entry.first;
      QString expr = entry.second;
      auto &instruction = assemblyMap.getObjectByAddress(location - 1);
      assemblerLine = instruction.lineNumber;
      auto result = expressionEvaluator(expr, labelValMap, true);
      if (!result.ok) {
        throw AssemblyError::failure(result.message, assemblerLine, -1);
      }

      validateValueRange(result.value, 0xFF, assemblerLine);

      Memory[location] = result.value;
      instruction.byte2 = result.value;
    }
    for (const auto &entry : callLabelExtMap) {
      int location = entry.first;
      QString expr = entry.second;
      auto &instruction = assemblyMap.getObjectByAddress(location - 1);
      assemblerLine = instruction.lineNumber;
      auto result = expressionEvaluator(expr, labelValMap, true);
      if (!result.ok) {
        throw AssemblyError::failure(result.message, assemblerLine, -1);
      }

      Memory[location] = (result.value >> 8) & 0xFF;
      Memory[location + 1] = result.value & 0xFF;
      instruction.byte2 = Memory[location];
      instruction.byte3 = Memory[location + 1];
    }
    for (const auto &entry : callLabelRelMap) {
      int location = entry.first;
      QString label = entry.second;
      auto &instruction = assemblyMap.getObjectByAddress(location - 1);
      assemblerLine = instruction.lineNumber;
      if (labelValMap.count(label) == 0) {
        if (label.contains('+') || label.contains('-')) {
          throw AssemblyError::failure("Cannot use expressions with relative addressing.", assemblerLine, -1);
        } else {
          throw AssemblyError::failure(Err::labelUndefined(label), assemblerLine, -1);
        }

      } else {
        int location2 = labelValMap[label];
        int value;
        value = location2 - location - 1;
        if (value > 127 || value < -128) {
          throw AssemblyError::failure(Err::numOutOfRelRange(value), assemblerLine, -1);
        }
        qint8 signedValue = static_cast<qint8>(value);
        value = signedValue & 0xFF;
        Memory[location] = value;
        instruction.byte2 = value;
      }
    }
  } catch (AssemblyError e) {
    assemblyError = e;
  }
  if (assemblyError.ok && HCFwarn) {
    messages.append(Msg{MsgType::WARN,
                        "Instructions 0x9D and 0xDD are undefined for the M6800 and would cause processor lockup (Halt and Catch Fire) on real hardware. If you are "
                        "using a M6803 or similar then this warning is irrelevant."});
  }
  for (int i = messages.size() - 1; i >= 0; --i) {
    if (messages[i].type == MsgType::NONE) {
      messages.removeAt(i);
    }
  }
  for (auto it = labelValMap.rbegin(); it != labelValMap.rend(); ++it) {
    messages.prepend(Msg{MsgType::DEBUG, "Value: $" + QString::number(it->second, 16) + " assigned to label '" + it->first + "'"});
  }
  return AssemblyResult{messages, assemblyError, assemblyMap};
}
bool Assembler::lineEmpty(QString &line) {
  if (line.isEmpty()) {
    return true;
  }
  if (line.contains(';')) {
    line = line.split(';')[0];
  }
  if (line.isEmpty()) {
    return true;
  }
  line = QString('-' + line).trimmed().mid(1);
  if (line.isEmpty()) {
    return true;
  }
  return false;
}

Assembler::LineParts Assembler::disectLine(QString line, int assemblerLine) {
  LineParts parts{"", "", ""};
  int charNum = 0;
  if (line[0].isLetter()) {
    // extract label

    for (; charNum < line.size(); ++charNum) {
      if (line[charNum].isLetterOrNumber() || line[charNum] == '_') {
        if (charNum == line.size() - 1) {
          throw AssemblyError::failure(Err::missingInstruction(), assemblerLine, -1);
        }
      } else if (line[charNum] == '\t' || line[charNum] == ' ') {
        parts.label = line.sliced(0, charNum).toUpper();
        if (mnemonics.contains(parts.label)) {
          throw AssemblyError::failure(Err::labelReserved(parts.label), assemblerLine, -1);
        }

        for (; charNum < line.size(); ++charNum) {
          if (line[charNum] == '\t' || line[charNum] == ' ') {
            if (charNum == line.size() - 1) {
              throw AssemblyError::failure(Err::missingInstruction(), assemblerLine, -1);
            }
          } else {
            break;
          }
        }
        break;
      } else {
        throw AssemblyError::failure(Err::labelContainsIllegalCharacter(line[charNum]), assemblerLine, charNum);
      }
    }
  } else if (line[0] == '\t' || line[0] == ' ') {
    charNum++;
    for (; charNum < line.size(); ++charNum) {
      if (line[charNum] == '\t' || line[charNum] == ' ') {
        if (charNum == line.size() - 1) {
          throw AssemblyError::failure(Err::missingInstruction(), assemblerLine, -1);
        }
      } else {
        break;
      }
    }
    parts.label = "";
  } else if (line[0].isDigit()) {
    throw AssemblyError::failure(Err::labelStartsWithIllegalDigit(line[charNum]), assemblerLine, charNum);

  } else {
    throw AssemblyError::failure(Err::labelStartsWithIllegalCharacter(line[charNum]), assemblerLine, charNum);
  }
  //get instruction and operand
  if (line.sliced(charNum).isEmpty()) {
    throw AssemblyError::failure(Err::missingInstruction(), assemblerLine, -1);
  }
  if (!line[charNum].isLetter() && line[charNum] != '.') {
    throw AssemblyError::failure(Err::unexpectedChar(line[charNum]), assemblerLine, charNum);
  }
  charNum++;
  if (line.sliced(charNum).isEmpty()) {
    throw AssemblyError::failure(Err::missingInstruction(), assemblerLine, -1);
  }
  for (int start = charNum - 1; charNum < line.size(); ++charNum) {
    if (line[charNum].isLetter()) {
      if (charNum == line.size() - 1) {
        parts.s_in = line.sliced(start).toUpper();
        return parts;
      }
    } else if (line[charNum] == ' ' || line[charNum] == '\t') {
      parts.s_in = (line.sliced(start, charNum - start)).toUpper();
      charNum++;
      break;
    } else {
      throw AssemblyError::failure(Err::unexpectedChar(line[charNum]), assemblerLine, charNum);
    }
  }

  parts.s_op = line.sliced(charNum);
  if (!parts.s_op.contains('\'')) {
    parts.s_op = parts.s_op.toUpper();
  }
  parts.s_op = parts.s_op.trimmed();
  return parts;
  /*
    immAdr = line[charNum] == '#';
    if (immAdr) {
      charNum++;
      if (line.sliced(charNum).isEmpty() || line[charNum] == ' ' || line[charNum] == '\t') {
        messages.append(Msg{MsgType::ERROR, "Missing operand."});
        
      }
    }
    if (line[charNum].isDigit()) {
      charNum++;
      while (charNum < line.length()) {
        if (line[charNum] == '\t' || line[charNum] == ' ') { //check end
          while (charNum < line.length()) {
            if (line[charNum] != '\t' && line[charNum] != ' ') {
              errorCharNum = charNum;
              messages.append(Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
              
            }
            charNum++;
          }
        } else if (!line[charNum].isDigit()) {
          errorCharNum = charNum;
          messages.append(Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
          
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
              messages.append(Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
              
            }
            charNum++;
          }
        } else if (!line[charNum].isDigit() && !(line[charNum].toUpper() >= 'A' && line[charNum].toUpper() <= 'F')) {
          errorCharNum = charNum;
          messages.append(Msg{MsgType::ERROR, "Hexadecimal numbers (prefixed by $) may only contain the numbers from 0 to 9 and letters from A to F. Unexpected character: '" % line[charNum] % "'"});
          
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
              messages.append(Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
              
            }
            charNum++;
          }
        } else if (line[charNum].isDigit() && line[charNum] != '0' && line[charNum] != '1') {
          errorCharNum = charNum;
          messages.append(Msg{MsgType::ERROR, "Binary numbers (prefixed by %) may only contain the numbers 1 and 0."});
          
        } else if (line[charNum] != '0' && line[charNum] != '1') {
          errorCharNum = charNum;
          messages.append(Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
          
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
              messages.append(Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
              
            }
            charNum++;
          }
        } else if (!line[charNum].isLetterOrNumber() && line[charNum] != '_') {
          errorCharNum = charNum;
          messages.append(Msg{MsgType::ERROR, "Negative numbers may only be written in decimal."});
          
        }

        charNum++;
      }
    } else if (line[charNum] == '\'') { //char
      if (line.sliced(charNum).length() < 3 || line[charNum + 2] != '\'') {
        messages.append(Msg{MsgType::ERROR, "Invalid ASCII conversion syntax. It should be: 'c', where c is a valid ASCII character"});
        
      }
      charNum += 3;
      while (charNum < line.length()) {
        if (line[charNum] != '\t' && line[charNum] != ' ') {
          errorCharNum = charNum;
          messages.append(Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
          
        }
        charNum++;
      }
    } else if (line[charNum] == '-') { //neg rel decimal
      if (immAdr) {
        errorCharNum = charNum;
        messages.append(Msg{MsgType::ERROR, "Negative numbers may only be used with relative addressing."});
        
      }
      charNum++;
      if (line.sliced(charNum).isEmpty()) {
        messages.append(Msg{MsgType::ERROR, "Missing operand."});
        
      }
      if (line[charNum] == '$' || line[charNum] == '%') {
        errorCharNum = charNum;
        messages.append(Msg{MsgType::ERROR, "Negative numbers may only be written in decimal."});
        
      }
      while (charNum < line.length()) {
        if (line[charNum] == '\t' || line[charNum] == ' ') { //check end
          while (charNum < line.length()) {
            if (line[charNum] != '\t' && line[charNum] != ' ') {
              errorCharNum = charNum;
              messages.append(Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
              
            }
            charNum++;
          }
        } else if ((line[charNum].toUpper() >= 'A' && line[charNum].toUpper() <= 'F')) {
          errorCharNum = charNum;
          messages.append(Msg{MsgType::ERROR, "Negative numbers may only be written in decimal."});
          
        } else if (!line[charNum].isDigit()) {
          errorCharNum = charNum;
          messages.append(Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
          
        }
        charNum++;
      }
    } else if (line[charNum] == '"') { //string
      if (s_in != ".STR" && s_in != "STR") {
        errorCharNum = charNum;
        messages.append(Msg{MsgType::ERROR, "Strings may not be used outside the .STR directive."});
        
      }
      if (immAdr) {
        errorCharNum = charNum - 1;
        messages.append(Msg{MsgType::ERROR, "Assembler directives do not support addressing modes like immediate (prefix: #)"});
        
      }
      while (charNum < line.length()) {
        if (line[charNum] == '\t' || line[charNum] == ' ') {
          break;
        }
        charNum++;
      }
      if (line[charNum - 1] != '"') {
        errorCharNum = charNum - 1;
        messages.append(Msg{MsgType::ERROR, "Strings must begin and end with a quotes."});
        
      }
      while (charNum < line.length()) {
        if (line[charNum] != '\t' && line[charNum] != ' ') {
          errorCharNum = charNum;
          messages.append(Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
          
        }
        charNum++;
      }
    } else {
      errorCharNum = charNum;
      messages.append(Msg{MsgType::ERROR, "Unexpected character: '" % line[charNum] % "'"});
      
    }
*/
}
