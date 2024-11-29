
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
#include "disassembler.h"
#include <QInputDialog>

/*Disassembler::InputNextAddressResult Disassembler::inputNextAddress(int curAdr, QString err) {
  bool ok;
  QString text = QInputDialog::getText(nullptr,
                                       "Input Dialog",
                                       err + ". Missing data will be written with .BYTE. Enter decimal address of next instruction. Current address: " +
                                         QString::number(curAdr, 10) + ".",
                                       QLineEdit::Normal,
                                       QString(),
                                       &ok);
  if (!ok)
    return {false, 0, Msg{MsgType::DEBUG, "Disassembly canceled."}};

  bool iok;
  int number = text.toInt(&iok);
  if (!iok)
    return {false, 0, Msg{MsgType::DEBUG, "Invalid address"}};

  if (number < curAdr)
    return {false, 0, Msg{MsgType::DEBUG, "Next instruction cannot be located before the previous one"}};

  return {true, 0, Msg::none()};
}*/

DisassemblyResult Disassembler::disassemble(ProcessorVersion ver, uint16_t begLoc, uint16_t endLoc, std::array<uint8_t, 0x10000> &Memory) {
  QString code;
  DataTypes::AssemblyMap assemblyMap;
  QList<Msg> messages;

  int line = 0;

  for (int i = 0xFFF0; i < 0xFFFE; i += 2) {
    uint16_t value = (Memory[i] << 8) | Memory[i + 1];
    if (value != 0) {
      code.append("\t.SETW $" + QString::number(i, 16).toUpper() + ",$" + QString::number(value, 16).toUpper() + "\n");
      assemblyMap.addInstruction(i, line++, 0, 0, 0, "SETW", "");
    }
  }

  int lastNonZero = 0xFFEF;
  for (int i = endLoc; i >= 0; --i) {
    if (Memory[i] != 0) {
      lastNonZero = i;
      break;
    }
  }

  int consecutiveZeros = 0;
  for (int address = 0; address < begLoc; ++address) {
    if (Memory[address] == 0) {
      consecutiveZeros++;
    } else {
      if (consecutiveZeros > 0) {
        code.append("\t.RMB " + QString::number(consecutiveZeros) + "\n");
        assemblyMap.addInstruction(address - consecutiveZeros, line++, 0, 0, 0, "RMB", "");
        consecutiveZeros = 0;
      }
      code.append("\t.BYTE $" + QString::number(Memory[address], 16).toUpper() + "\n");
      assemblyMap.addInstruction(address, line++, 0, 0, 0, "BYTE", "");
    }
  }
  if (consecutiveZeros > 0) {
    code.append("\t.RMB " + QString::number(consecutiveZeros) + "\n");
    assemblyMap.addInstruction(begLoc - consecutiveZeros, line++, 0, 0, 0, "RMB", "");
  }

  code.append("\t.ORG $" + QString::number(begLoc, 16).toUpper() + "\n");
  assemblyMap.addInstruction(begLoc, line++, 0, 0, 0, "ORG", "");

  for (int address = begLoc; address <= lastNonZero;) {
    int opCode = Memory[address];
    int inSize = getInstructionLength(ver, opCode);
    AddressingMode inType = getInstructionMode(ver, opCode);
    QString in = getInstructionMnemonic(ver, opCode);

    if (inType == AddressingMode::INVALID) {
      messages.append(Msg{MsgType::WARN, "Unknown/unsupported instruction at address: $" + QString::number(address, 16).toUpper()});
      if (Memory[address] == 0) {
        int zeroCount = 0;
        while (Memory[address + zeroCount] == 0) {
          zeroCount++;
        }
        code.append("\t.RMB " + QString::number(zeroCount) + "\n");
        assemblyMap.addInstruction(address, line++, 0, 0, 0, "RMB", "");
        address += zeroCount + 1;

      } else {
        code.append("\t.BYTE $" + QString::number(Memory[address], 16).toUpper() + " ;UNKOWN INSTRUCTION\n");
        assemblyMap.addInstruction(address, line++, 0, 0, 0, "BYTE", "");
        address++;
      }
      continue;
      /*int zeroCount = 0;
      for (int i = address; i < nextI; ++i) {
        if (Memory[i] == 0) {
          zeroCount++;
        } else {
          if (zeroCount > 0) {
            code.append("\t.RMB " + QString::number(zeroCount) + "\n");
            assemblyMap.addInstruction(i - zeroCount, line++, 0, 0, 0, "RMB", "");
            zeroCount = 0;
          }
          code.append("\t.BYTE $" + QString::number(Memory[i], 16).toUpper() + "\n");
          assemblyMap.addInstruction(i, line++, 0, 0, 0, "BYTE", "");
        }
      }
      if (zeroCount > 0) {
        code.append("\t.RMB " + QString::number(zeroCount) + "\n");
        assemblyMap.addInstruction(nextI - zeroCount, line++, 0, 0, 0, "RMB", "");
      }
      address = nextI;
      continue;*/
    }

    uint8_t operand1 = 0, operand2 = 0;
    switch (inType) {
    case AddressingMode::INH:
      code.append("\t" + in + "\n");
      break;
    case AddressingMode::IMM:
      operand1 = Memory[(address + 1) & 0xFFFF];
      code.append("\t" + in + " #$" + QString::number(operand1, 16).toUpper() + "\n");
      break;
    case AddressingMode::IMMEXT:
      operand1 = Memory[(address + 1) & 0xFFFF];
      operand2 = Memory[(address + 2) & 0xFFFF];
      code.append("\t" + in + " #$" + QString::number((operand1 << 8) | operand2, 16).toUpper() + "\n");
      break;
    case AddressingMode::DIR:
      operand1 = Memory[(address + 1) & 0xFFFF];
      code.append("\t" + in + " $" + QString::number(operand1, 16).toUpper() + "\n");
      break;
    case AddressingMode::IND:
      operand1 = Memory[(address + 1) & 0xFFFF];
      code.append("\t" + in + " $" + QString::number(operand1, 16).toUpper() + ",X\n");
      break;
    case AddressingMode::EXT:
      operand1 = Memory[(address + 1) & 0xFFFF];
      operand2 = Memory[(address + 2) & 0xFFFF];
      code.append("\t" + in + " $" + QString::number((operand1 << 8) | operand2, 16).toUpper() + "\n");
      break;
    case AddressingMode::REL:
      operand1 = Memory[(address + 1) & 0xFFFF];
      if (operand1 == 0xFF || operand1 == 0xFE) {
        code.append("\t" + in + " $00 ;Machine code addresses relative address " + QString::number(operand1, 16).toUpper() + "which is out of bounds\n");
        messages.append(Msg{MsgType::WARN, "Machine code addresses relative address " + QString::number(operand1, 16).toUpper() + "which is out of bounds"});
      } else {
        code.append("\t" + in + " $" + QString::number(operand1, 16).toUpper() + "\n");
      }
      break;
    }

    assemblyMap.addInstruction(address, line++, opCode, operand1, operand2, in, "");
    address += inSize;
  }
  return DisassemblyResult{messages, code, assemblyMap};
}
