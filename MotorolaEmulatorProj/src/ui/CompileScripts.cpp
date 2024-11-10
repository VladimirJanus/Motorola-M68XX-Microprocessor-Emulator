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
#include <QInputDialog>
#include "mainwindow.h"
#include "ui_mainwindow.h"

int MainWindow::inputNextAddress(int curAdr, QString err) {
  bool ok;
  QString text = QInputDialog::getText(this,
                                       "Input Dialog",
                                       err + ". Missing data will be written with .BYTE. Enter decimal address of next instruction. Current address: " +
                                         QString::number(curAdr, 10) + ".",
                                       QLineEdit::Normal,
                                       QString(),
                                       &ok);

  if (ok) {
    bool iok;
    int number = text.toInt(&iok);
    if (iok) {
      if (number < curAdr) {
        PrintConsole("Next instruction cannot be located before the previous one", MsgType::ERROR);
        return -1;
      }
      return number;
    } else {
      PrintConsole("Invalid address", MsgType::ERROR);
      return -1;
    }
  } else {
    PrintConsole("Disassembly canceled.");
    return -1;
  }
}

bool MainWindow::disassemble(ProcessorVersion ver, int begLoc) {
  QString code;
  assemblyMap.clear();
  int line = 0;

  for (int i = 0xFFF0; i < 0xFFFE; i += 2) {
    uint16_t value = (processor.Memory[i] << 8) | processor.Memory[i + 1];
    if (value != 0) {
      code.append("\t.SETW $" + QString::number(i, 16).toUpper() + ",$" + QString::number(value, 16).toUpper() + "\n");
      assemblyMap.addInstruction(i, line++, 0, 0, 0, "SETW", "");
    }
  }

  int lastNonZero = 0xFFEF;
  for (int i = 0xFFEF; i >= 0; --i) {
    if (processor.Memory[i] != 0) {
      lastNonZero = i;
      break;
    }
  }

  int consecutiveZeros = 0;
  for (int address = 0; address < begLoc; ++address) {
    if (processor.Memory[address] == 0) {
      consecutiveZeros++;
    } else {
      if (consecutiveZeros > 0) {
        code.append("\t.RMB " + QString::number(consecutiveZeros) + "\n");
        assemblyMap.addInstruction(address - consecutiveZeros, line++, 0, 0, 0, "RMB", "");
        consecutiveZeros = 0;
      }
      code.append("\t.BYTE $" + QString::number(processor.Memory[address], 16).toUpper() + "\n");
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
    int opCode = processor.Memory[address];
    int inSize = getInstructionLength(ver, opCode);
    AddressingMode inType = getInstructionMode(ver, opCode);
    QString in = getInstructionMnemonic(ver, opCode);

    if (inType == AddressingMode::INVALID) {
      PrintConsole("Unknown/unsupported instruction at address: $" + QString::number(address, 16).toUpper(), MsgType::ERROR);
      int nextI = inputNextAddress(address, "Unknown instruction");
      if (nextI == -1)
        break;
      if (nextI <= address) {
        PrintConsole("Invalid next address. Must be greater than current address.", MsgType::ERROR);
        continue;
      }

      int zeroCount = 0;
      for (int i = address; i < nextI; ++i) {
        if (processor.Memory[i] == 0) {
          zeroCount++;
        } else {
          if (zeroCount > 0) {
            code.append("\t.RMB " + QString::number(zeroCount) + "\n");
            assemblyMap.addInstruction(i - zeroCount, line++, 0, 0, 0, "RMB", "");
            zeroCount = 0;
          }
          code.append("\t.BYTE $" + QString::number(processor.Memory[i], 16).toUpper() + "\n");
          assemblyMap.addInstruction(i, line++, 0, 0, 0, "BYTE", "");
        }
      }
      if (zeroCount > 0) {
        code.append("\t.RMB " + QString::number(zeroCount) + "\n");
        assemblyMap.addInstruction(nextI - zeroCount, line++, 0, 0, 0, "RMB", "");
      }
      address = nextI;
      continue;
    }

    uint8_t operand1 = 0, operand2 = 0;
    switch (inType) {
    case AddressingMode::INH:
      code.append("\t" + in + "\n");
      break;
    case AddressingMode::IMM:
      operand1 = processor.Memory[(address + 1) & 0xFFFF];
      code.append("\t" + in + " #$" + QString::number(operand1, 16).toUpper() + "\n");
      break;
    case AddressingMode::IMMEXT:
      operand1 = processor.Memory[(address + 1) & 0xFFFF];
      operand2 = processor.Memory[(address + 2) & 0xFFFF];
      code.append("\t" + in + " #$" + QString::number((operand1 << 8) | operand2, 16).toUpper() + "\n");
      break;
    case AddressingMode::DIR:
      operand1 = processor.Memory[(address + 1) & 0xFFFF];
      code.append("\t" + in + " $" + QString::number(operand1, 16).toUpper() + "\n");
      break;
    case AddressingMode::IND:
      operand1 = processor.Memory[(address + 1) & 0xFFFF];
      code.append("\t" + in + " $" + QString::number(operand1, 16).toUpper() + ",X\n");
      break;
    case AddressingMode::EXT:
      operand1 = processor.Memory[(address + 1) & 0xFFFF];
      operand2 = processor.Memory[(address + 2) & 0xFFFF];
      code.append("\t" + in + " $" + QString::number((operand1 << 8) | operand2, 16).toUpper() + "\n");
      break;
    case AddressingMode::REL:
      operand1 = processor.Memory[(address + 1) & 0xFFFF];
      if (operand1 == 0xFF || operand1 == 0xFE) {
        code.append("\t" + in + " $00 ;Relative address FF is out of bounds\n");
        PrintConsole("Relative address FF is out of bounds", MsgType::ERROR);
      } else {
        code.append("\t" + in + " $" + QString::number(operand1, 16).toUpper() + "\n");
      }
      break;
    }

    assemblyMap.addInstruction(address, line++, opCode, operand1, operand2, in, "");
    address += inSize;
  }

  ui->plainTextCode->setPlainText(code);
  std::memcpy(processor.backupMemory.data(), processor.Memory.data(), processor.Memory.size() * sizeof(uint8_t));
  setCompileStatus(true);
  return true;
}
