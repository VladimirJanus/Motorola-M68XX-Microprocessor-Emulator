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

using Core::AddressingMode;
using Core::AssemblyError;
using Core::AssemblyMap;
using Core::AssemblyResult;
using Core::MnemonicInfo;
using Core::Msg;
using Core::MsgType;
using Core::ProcessorVersion;

/**
 * @brief Parses decimal string with optional negative values.
 * 
 * This function takes a decimal string and converts it to an integer value.
 * It can optionally allow negative numbers for relative addressing.
 * 
 * @param input Decimal string.
 * @param allowNeg Allow negative numbers for relative addressing.
 * @return NumParseResult containing the parsed value or an error message.
 * 
 * @note The function returns an error if the input string is empty, contains invalid characters,
 *       or if the parsed number is out of the corresponding range.
 */
Assembler::NumParseResult Assembler::parseDec(const QString &input) {
  bool ok;
  int32_t number = input.toInt(&ok);

  if (!ok) {
    return NumParseResult::failure(Err::invalidDecOrRange(input));
  } else if (number > 0xFFFF || number < 0) {
    return NumParseResult::failure(Err::numOutOfRange(number, 0xFFFF));
  }

  return NumParseResult::success(static_cast<uint16_t>(number));
}
Assembler::NumParseRelativeResult Assembler::parseDecRelative(const QString &input) {
  bool ok;
  int32_t number = input.toInt(&ok);
    if (!ok) {
      return NumParseRelativeResult::failure(Err::invalidRelDecOrRange(input));
    } else if (number > 127 || number < -128) {
      return NumParseRelativeResult::failure(Err::numOutOfRelRange(number));
    }

    return NumParseRelativeResult::success(static_cast<int8_t>(number));
}
/**
 * @brief Parses a hexadecimal string (format: $FF).
 * 
 * This function takes a hexadecimal string with a $ prefix and converts it to an integer value.
 * It validates that the input string is within the 16-bit range (0x0000-0xFFFF).
 * 
 * @param input Hexadecimal string with $ prefix.
 * @return NumParseResult containing the parsed value or an error message.
 * 
 * @note The function returns an error if the input string is empty, contains invalid characters,
 *       or if the parsed number is out of the 16-bit range.
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
  return NumParseResult::success(static_cast<uint16_t>(number));
}
/**
 * @brief Parses binary string (format: %1010)
 * 
 * This function takes a binary string with a % prefix and converts it to an integer value.
 * It validates that the input string is within the 16-bit range (0x0000-0xFFFF).
 * 
 * @param input Binary string with % prefix.
 * @return NumParseResult containing the parsed value or an error message.
 * 
 * @note The function returns an error if the input string is empty, contains invalid characters,
 *       or if the parsed number is out of the 16-bit range.
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
  return NumParseResult::success(static_cast<uint16_t>(number));
}
/**
 * @brief Parses ASCII character literal (format: 'A').
 * 
 * This function takes an ASCII character literal enclosed in single quotes and converts it to its ASCII value.
 * It enforces the 7-bit ASCII range (0-127).
 * 
 * @param input Character literal.
 * @return NumParseResult containing the ASCII value or an error message.
 * 
 * @note The function returns an error if the input string is not in the correct format or if the character is out of the 7-bit ASCII range.
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
 * @brief Universal number parser with format detection.
 * 
 * This function detects the format of the input string (ASCII, Hex, Binary, Decimal) and parses it accordingly.
 * 
 * @param input Number string in supported formats.
 * @return NumParseResult containing the parsed value or an error message.
 */
Assembler::NumParseResult Assembler::parseNumber(const QString &input) {
  if (input.contains("'")) {
    return parseASCII(input);
  } else if (input.startsWith('$')) {
    return parseHex(input);
  } else if (input.startsWith('%')) {
    return parseBin(input);
  } else {
    return parseDec(input);
  }
}
/**
 * @brief Processes relative address operands.
 * 
 * This function processes relative address expressions and adjusts the offset value.
 * It blocks $FE/$FF values and enforces the -128 to 127 range.
 * 
 * @param input Relative address expression.
 * @return NumParseRelativeResult containing the adjusted offset value or an error message.
 */
Assembler::NumParseRelativeResult Assembler::parseNumberRelative(const QString &input) {
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
    NumParseRelativeResult result = parseDecRelative(input);
    if (!result.ok) {
      return NumParseRelativeResult::failure(result.message);
    }
    int value = result.value;
    if (value < 0)
      value -= 2;
    if (value > 127 || value < -128) {
      return NumParseRelativeResult::failure(Err::numOutOfRelRange(value));
    }
    return NumParseRelativeResult::success(static_cast<int8_t>(value));
  }
}

/**
 * @brief Evaluates arithmetic expressions with labels.
 * 
 * This function evaluates mathematical expressions that may include labels.
 * It can control the handling of undefined labels based on the errOnUndefined parameter.
 * 
 * @param expr Mathematical expression string.
 * @param labelValMap Current label values.
 * @param errOnUndefined Control undefined label handling.
 * @return ExpressionEvaluationResult containing the evaluated value or an error message.
 * 
 * @note If errOnUndefined is true, the function returns unsuccessfully if the expression contains an undefined label.
 *       If errOnUndefined is false, the function returns successfully but indicates that the label is undefined.
 */
Assembler::ExpressionEvaluationResult Assembler::expressionEvaluator(QString expr, std::map<QString, int> &labelValMap, bool errOnUndefined) {
  QList<QString> symbols;

  for (qsizetype i = expr.length() - 1; i >= 0; --i) {
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
          return ExpressionEvaluationResult::setUndefined(Err::instructionDoesNotSupportLabelForwardDeclaration(symbol), !errOnUndefined);
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
    return ExpressionEvaluationResult::failure(Err::exprOutOfRange(value)); //should be kept because ExpressionEvaluationResult::success converts int to uint16_t and returns that.
  }

  return ExpressionEvaluationResult::success(value);
}

/**
 * @brief Checks if a string represents a label or an expression.
 * 
 * This function returns true if the input string represents a label or an expression.
 * 
 * @param s_op Input string.
 * @return True if the string represents a label or an expression, false otherwise.
 */
inline bool Assembler::isLabelOrExpression(QString s_op) {
  return (s_op[0].isLetter() || s_op.contains('+') || s_op.contains('-'));
}
/**
 * @brief Checks if a line is empty or a comment.
 * 
 * This function determines if a line of assembly code is empty or contains only a comment.
 * It removes any comments and trims whitespace to make this determination.
 * 
 * @param line Reference to the line of assembly code.
 * @return True if the line is empty or a comment, false otherwise.
 */
bool Assembler::trimLineAndCheckEmpty(QString &line) {
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

/**
 * @brief Adds a label and its value to the label-to-value map.
 * 
 * This function assigns a value to a label and adds it to the label-to-value map.
 * 
 * @param label Label to assign.
 * @param value Value to assign to the label.
 * @param labelValMap Map of label values.
 * @param assemblerLine Current line number.
 * @throws AssemblyError if the label is already defined.
 */
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

/**
*
* @brief Retrieves mnemonic information and validates processor compatibility.
*
* This function checks if a mnemonic is recognized and supported by the specified processor version.
* If valid, it returns detailed information about the mnemonic. If the mnemonic is invalid or
* incompatible with the processor version, an error is thrown.
*
* @param s_in Mnemonic to validate and retrieve information for.
* @param processorVersion Target processor version to check compatibility against.
* @param assemblerLine Current line number in the assembler source (for error reporting).
* @return MnemonicInfo Structured information about the validated mnemonic.
* @throws AssemblyError Thrown under two conditions:
*
*                  - If the mnemonic is unrecognized (invalid).
*                  - If the mnemonic is not supported by the specified processor version.
*/
MnemonicInfo Assembler::getMnemonicInfo(QString &s_in, ProcessorVersion processorVersion, int assemblerLine) {
  MnemonicInfo info = getInfoByMnemonic(processorVersion, s_in);
  if (info.mnemonic == "INVALID") {
    throw AssemblyError::failure(Err::instructionDoesNotSupportProcessor(s_in), assemblerLine, -1);
  }
  return info;
}

/**
 * @brief Validates instruction support for the specified processor version.
 * 
 * This function checks if an instruction is supported by the specified processor version.
 * 
 * @param s_in Instruction mnemonic.
 * @param opCode Operation code.
 * @param processorVersion Processor version.
 * @param assemblerLine Current line number.
 * @throws AssemblyError if the instruction is not supported by the processor.
 */
void Assembler::validateInstructionSupport(QString s_in, uint8_t opCode, ProcessorVersion processorVersion, int assemblerLine) {
  if (!getInstructionSupported(processorVersion, opCode)) {
    throw AssemblyError::failure(Err::instructionDoesNotSupportProcessor(s_in), assemblerLine, -1);
  }
  return;
}
/**
 * @brief Validates mnemonic support for the specified addressing mode.
 * 
 * This function checks if a mnemonic supports the specified addressing mode.
 * 
 * @param s_in Mnemonic to check.
 * @param mode Addressing mode.
 * @param assemblerLine Current line number.
 * @throws AssemblyError if the mnemonic does not support the addressing mode.
 */
void Assembler::validateMnemonicSupportForAddressingMode(MnemonicInfo &info, AddressingMode mode, int assemblerLine) {
  if (info.opCodes[Core::addressingModes[mode].id] == 0) {
    throw AssemblyError::failure(Err::mnemonicDoesNotSupportAddressingMode(info.mnemonic, mode), assemblerLine, -1);
  }
  return;
}

/**
 * @brief Checks for unexpected operand.
 * 
 * This function throws an error if the operand is not empty.
 * 
 * @param s_op Operand string.
 * @param assemblerLine Current line number.
 * @throws AssemblyError if the operand is not empty.
 */
void Assembler::errorCheckUnexpectedOperand(QString s_op, int assemblerLine) {
  if (!s_op.isEmpty()) {
    throw AssemblyError::failure(Err::unexpectedOperand(), assemblerLine, -1);
  }
  return;
}
/**
 * @brief Checks for missing operand.
 * 
 * This function throws an error if the operand is empty.
 * 
 * @param s_op Operand string.
 * @param assemblerLine Current line number.
 * @throws AssemblyError if the operand is empty.
 */
void Assembler::errorCheckMissingOperand(QString s_op, int assemblerLine) {
  if (s_op.isEmpty()) {
    throw AssemblyError::failure(Err::missingOperand(), assemblerLine, -1);
  }
  return;
}
/**
 * @brief Checks for missing label.
 * 
 * This function throws an error if the label is empty.
 * 
 * @param label Label string.
 * @param assemblerLine Current line number.
 * @throws AssemblyError if the label is empty.
 */
void Assembler::errorCheckMissingLabel(QString label, int assemblerLine) {
  if (label.isEmpty()) {
    throw AssemblyError::failure(Err::missingLabel(), assemblerLine, -1);
  }
  return;
}
/**
 * @brief Checks if operand contains indirect addressing mode.
 * 
 * This function throws an error if the operand contains a comma, indicating indirect addressing mode.
 * 
 * @param s_in Mnemonic.
 * @param s_op Operand string.
 * @param assemblerLine Current line number.
 * @throws AssemblyError if the operand contains a comma.
 */
void Assembler::errorCheckOperandContainsIND(QString s_in, QString s_op, int assemblerLine) {
  if (s_op.contains(',')) {
    throw AssemblyError::failure(Err::mnemonicDoesNotSupportAddressingMode(s_in, AddressingMode::IND), assemblerLine, -1);
  }
  return;
}
/**
 * @brief Checks if operand contains immediate addressing mode.
 * 
 * This function throws an error if the operand contains a hash symbol, indicating immediate addressing mode.
 * 
 * @param s_in Mnemonic.
 * @param s_op Operand string.
 * @param assemblerLine Current line number.
 * @throws AssemblyError if the operand contains a hash symbol.
 */
void Assembler::errorCheckOperandContainsIMM(QString s_in, QString s_op, int assemblerLine) {
  if (s_op.contains('#')) {
    throw AssemblyError::failure(Err::mnemonicDoesNotSupportAddressingMode(s_in, AddressingMode::IMM), assemblerLine, -1);
  }
  return;
}
/**
 * @brief Checks if operand contains mixed immediate and indirect addressing modes.
 * 
 * This function throws an error if the operand contains both a hash symbol and a comma.
 * 
 * @param s_op Operand string.
 * @param assemblerLine Current line number.
 * @throws AssemblyError if the operand contains both a hash symbol and a comma.
 */
void Assembler::errorCheckOperandIMMINDMixed(QString s_op, int assemblerLine) {
  if (s_op.contains('#') && s_op.contains(',')) {
    throw AssemblyError::failure(Err::mixedIMMandIND(), assemblerLine, -1);
  }
  return;
}

/**
 * @brief Validates value range.
 * 
 * This function checks if a value is within the specified range.
 * 
 * @param value Value to check.
 * @param max Maximum allowed value.
 * @param assemblerLine Current line number.
 * @throws AssemblyError if the value is out of range.
 */
void Assembler::validateValueRange(int32_t value, int32_t max, int assemblerLine) {
  if (value > 0xFF) {
    throw AssemblyError::failure(Err::numOutOfRange(value, max), assemblerLine, -1);
  }
  return;
}

/**
 * @brief Splits a line into label, instruction, and operand components.
 *
 * This function parses a line of assembly code and extracts its components: label, instruction, and operand.
 * It performs syntax validation and throws an error for any invalid syntax.
 *
 * @param line Source assembly line.
 * @param assemblerLine Current line number.
 * @return LineParts structure containing the label, instruction, and operand.
 * @throws AssemblyError for invalid syntax, such as missing instruction, illegal characters, or unexpected characters.
 */
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

        if (Core::isMnemonic(parts.label)) {
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
  // get instruction and operand
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
  if (!parts.s_op.contains('\'') && !parts.s_op.contains('\"')) {
    parts.s_op = parts.s_op.toUpper();
  }
  parts.s_op = parts.s_op.trimmed();
  return parts;
}

/**
 * @brief Assembles the input assembly code into machine code.
 *
 * This function processes the input assembly code, validates instructions and operands,
 * and generates the corresponding machine code in the output memory buffer.
 * It also handles label assignments, error checking, and message logging.
 *
 * @param processorVersion The version of the processor for which the code is being assembled.
 * @param code The input assembly code as a QString.
 * @param Memory The output memory buffer where the assembled machine code will be stored.
 * @return AssemblyResult containing messages, errors, and the assembly map.
 *
 * @note The function throws AssemblyError for various syntax and semantic errors in the input code.
 */
AssemblyResult Assembler::assemble(ProcessorVersion processorVersion, QString &code, std::array<uint8_t, 0x10000> &Memory) {
  int assemblerLine = 0;
  uint16_t assemblerAddress = 0;

  std::map<QString, int> labelValMap;
  std::map<uint16_t, QString> callLabelMap;
  std::map<uint16_t, QString> callLabelRelMap;
  std::map<uint16_t, QString> callLabelExtMap;

  QList<Msg> messages;
  AssemblyError assemblyError = AssemblyError::none();
  AssemblyMap assemblyMap;

  bool HCFwarn = false; // should the assembler warn that there are potential Halt-and-Catch-Fire instructions in the machine code

  const QStringList lines = code.split('\n');
  const qsizetype totalLines = lines.count();

  // Predefined interrupt vectors
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

      if (trimLineAndCheckEmpty(line)) {
        continue;
      }

      //parse line
      LineParts parts = disectLine(line, assemblerLine);
      QString &label = parts.label;
      QString &s_in = parts.s_in;
      QString &s_op = parts.s_op;

      //replace with allias mnemonic
      if (Core::alliasMap.contains(s_in)) {
        if (!(Core::alliasMap[s_in].supportedVersions & processorVersion)) {
          throw AssemblyError::failure(Err::instructionDoesNotSupportProcessor(s_in), assemblerLine, -1);
        }
        s_in = Core::alliasMap[s_in].mnemonic;
      }

      MnemonicInfo mnemonicInfo = getMnemonicInfo(s_in, processorVersion, assemblerLine);

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
          uint16_t value = result.value;

          assignLabelValue(label, value, labelValMap, assemblerLine);
        } else if (s_in == ".ORG") {
          errorCheckMissingOperand(s_op, assemblerLine);

          auto result = expressionEvaluator(s_op, labelValMap, true);
          if (!result.ok) {
            throw AssemblyError::failure(result.message, assemblerLine, -1);
          }
          uint16_t value = result.value;

          Memory[Core::interruptLocations - 1] = value >> 8;
          Memory[Core::interruptLocations] = value & 0xFF;
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

            auto result = expressionEvaluator(curOp, labelValMap, true);
            if (!result.ok) {
              throw AssemblyError::failure(result.message, assemblerLine, -1);
            }
            uint16_t value = result.value;

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

          auto result = expressionEvaluator(adrOp, labelValMap, true);
          if (!result.ok) {
              throw AssemblyError::failure(result.message, assemblerLine, -1);
          }
          uint16_t adr = result.value;
          QString valOp = s_op.split(",")[1];
          if (valOp.isEmpty()) {
            throw AssemblyError::failure(Err::missingValue(), assemblerLine, -1);
          }

          result = expressionEvaluator(valOp, labelValMap, true);
          if (!result.ok) {
            throw AssemblyError::failure(result.message, assemblerLine, -1);
          }
          uint16_t val = result.value;

          operand1 = (val >> 8) & 0xFF;
          operand2 = val & 0xFF;
          Memory[adr] = operand1;
          Memory[(adr + 1) & 0xFFFF] = operand2;

          assignLabelValue(label, adr, labelValMap, assemblerLine);
        } else if (s_in == ".SETB") {
          if (s_op.count(",") != 1) {
            throw AssemblyError::failure(Err::invalidSETSyntaxMissingComma("SETB"), assemblerLine, -1);
          }

          QString adrOp = s_op.split(",")[0];
          if (adrOp.isEmpty()) {
            throw AssemblyError::failure(Err::missingValue(), assemblerLine, -1);
          }

          auto result = expressionEvaluator(adrOp, labelValMap, true);
          if (!result.ok) {
              throw AssemblyError::failure(result.message, assemblerLine, -1);
          }
          uint16_t adr = result.value;
          QString valOp = s_op.split(",")[1];
          if (valOp.isEmpty()) {
              throw AssemblyError::failure(Err::missingValue(), assemblerLine, -1);
          }

          result = expressionEvaluator(valOp, labelValMap, true);
          if (!result.ok) {
              throw AssemblyError::failure(result.message, assemblerLine, -1);
          }
          uint16_t val = result.value;

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

        bool hasLocation = (Core::directivesWithLocation.contains(s_in));
        assemblyMap.addInstruction(hasLocation ? instructionAddress : -1, assemblerLine, opCode, operand1, operand2, s_in, s_op);
      } else {
        assignLabelValue(label, assemblerAddress, labelValMap, assemblerLine);

        if (uint8_t tempCodeINH = mnemonicInfo.opCodes[Core::addressingModes[AddressingMode::INH].id]; tempCodeINH != 0) { // INH
          errorCheckUnexpectedOperand(s_op, assemblerLine);

          opCode = tempCodeINH;
          validateInstructionSupport(s_in, opCode, processorVersion, assemblerLine);

          Memory[assemblerAddress++] = opCode;
        } else {
          errorCheckMissingOperand(s_op, assemblerLine);

          if (uint8_t tempCode = mnemonicInfo.opCodes[Core::addressingModes[AddressingMode::REL].id]; tempCode != 0) { // REL
            errorCheckOperandContainsIMM(s_in, s_op, assemblerLine);
            errorCheckOperandContainsIND(s_in, s_op, assemblerLine);

            opCode = tempCode;
            validateInstructionSupport(s_in, opCode, processorVersion, assemblerLine);

            if (s_op[0].isLetter()) {
              callLabelRelMap[(assemblerAddress + 1) & 0xFFFF] = s_op;
              operand1 = 0;
            } else {
              NumParseRelativeResult resultVal = parseNumberRelative(s_op);
              if (!resultVal.ok)
                throw AssemblyError::failure(resultVal.message, assemblerLine, -1);

              operand1 = resultVal.value;
            }
            Memory[assemblerAddress++] = opCode;
            Memory[assemblerAddress++] = operand1;
          } else if (s_op.contains(',')) { // IND
            errorCheckOperandIMMINDMixed(s_op, assemblerLine);

            validateMnemonicSupportForAddressingMode(mnemonicInfo, AddressingMode::IND, assemblerLine);

            opCode = mnemonicInfo.opCodes[Core::addressingModes[AddressingMode::IND].id];
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
              callLabelMap[(assemblerAddress + 1) & 0xFFFF] = s_op;
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

            validateMnemonicSupportForAddressingMode(mnemonicInfo, AddressingMode::IMM, assemblerLine);

            opCode = mnemonicInfo.opCodes[Core::addressingModes[AddressingMode::IMM].id];
            validateInstructionSupport(s_in, opCode, processorVersion, assemblerLine);

            if (getInstructionMode(processorVersion, opCode) == AddressingMode::IMMEXT) {
              if (isLabelOrExpression(s_op)) {
                callLabelExtMap[(assemblerAddress + 1) & 0xFFFF] = s_op;
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
                callLabelMap[(assemblerAddress + 1) & 0xFFFF] = s_op;
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
            uint16_t value = 0;
            if (isLabelOrExpression(s_op)) {
              auto result = expressionEvaluator(s_op, labelValMap, false);
              if (!result.ok) {
                throw AssemblyError::failure(result.message, assemblerLine, -1);
              }

              if (result.undefined) {
                skipDir = true;
                callLabelExtMap[(assemblerAddress + 1) & 0xFFFF] = s_op;
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

            if (mnemonicInfo.opCodes[Core::addressingModes[AddressingMode::DIR].id] != 0 && !skipDir) {
              opCode = mnemonicInfo.opCodes[Core::addressingModes[AddressingMode::DIR].id];

              if (getInstructionSupported(processorVersion, opCode)) {
                operand1 = value;
                Memory[assemblerAddress++] = opCode;
                Memory[assemblerAddress++] = operand1;
              } else if (mnemonicInfo.opCodes[Core::addressingModes[AddressingMode::EXT].id] != 0) {
                opCode = mnemonicInfo.opCodes[Core::addressingModes[AddressingMode::EXT].id];
                validateInstructionSupport(s_in, opCode, processorVersion, assemblerLine);

                operand1 = (value >> 8) & 0xFF;
                operand2 = value & 0xFF;
                Memory[assemblerAddress++] = opCode;
                Memory[assemblerAddress++] = operand1;
                Memory[assemblerAddress++] = operand2;
              } else {
                throw AssemblyError::failure(Err::mnemonicDoesNotSupportAddressingMode(s_in, AddressingMode::EXT), assemblerLine, -1);
              }
            } else if (mnemonicInfo.opCodes[Core::addressingModes[AddressingMode::EXT].id] != 0) {
              opCode = mnemonicInfo.opCodes[Core::addressingModes[AddressingMode::EXT].id];
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
    // second pass/passes to resolve undefined expr or labels
    for (const auto &entry : callLabelMap) {
      uint16_t location = entry.first;
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
      uint16_t location = entry.first;
      QString expr = entry.second;
      auto &instruction = assemblyMap.getObjectByAddress(location - 1);
      assemblerLine = instruction.lineNumber;
      auto result = expressionEvaluator(expr, labelValMap, true);
      if (!result.ok) {
        throw AssemblyError::failure(result.message, assemblerLine, -1);
      }

      Memory[location] = (result.value >> 8) & 0xFF;
      Memory[(location + 1) & 0xFFFF] = result.value & 0xFF;
      instruction.byte2 = Memory[location];
      instruction.byte3 = Memory[(location + 1) & 0xFFFF];
    }
    for (const auto &entry : callLabelRelMap) {
      uint16_t location = entry.first;
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
        uint16_t location2 = labelValMap[label];
        int value;
        value = location2 - location - 1;
        if (value > 127 || value < -128) {
          throw AssemblyError::failure(Err::numOutOfRelRange(value), assemblerLine, -1);
        }
        int8_t signedValue = static_cast<int8_t>(value);
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
  for (qsizetype i = messages.size() - 1; i >= 0; --i) {
    if (messages[i].type == MsgType::NONE) {
      messages.removeAt(i);
    }
  }
  for (auto it = labelValMap.rbegin(); it != labelValMap.rend(); ++it) {
    messages.prepend(Msg{MsgType::DEBUG, "Value: $" + QString::number(it->second, 16) + " assigned to label '" + it->first + "'"});
  }
  return AssemblyResult{messages, assemblyError, assemblyMap};
}
