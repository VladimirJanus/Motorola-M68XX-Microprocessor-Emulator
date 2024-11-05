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
#include "InstructionList.h"

void InstructionList::clear() {
    instructions.clear();
}

void InstructionList::addInstruction(int address, int lineNumber, int byte1, int byte2, int byte3, QString IN, QString OP) {
  Instruction instruction;
  instruction.address = address;
  instruction.lineNumber = lineNumber;
  instruction.byte1 = byte1;
  instruction.byte2 = byte2;
  instruction.byte3 = byte3;
  instruction.IN = IN;
  instruction.OP = OP;
  instructions.push_back(instruction);
}

InstructionList::Instruction& InstructionList::getObjectByAddress(int address) {
  for (Instruction& instruction : instructions) {
    if (instruction.address == address) {
      return instruction;
    }
  }
  static Instruction defaultInstruction = {address, -1, 0, 0, 0, "", ""};
  return defaultInstruction;
}

InstructionList::Instruction& InstructionList::getObjectByLine(int lineNumber) {
  for (Instruction& instruction : instructions) {
    if (instruction.lineNumber == lineNumber) {
      return instruction;
    }
  }
  static Instruction defaultInstruction = {-1, lineNumber, 0, 0, 0, "", ""};
  return defaultInstruction;
}

bool InstructionList::isEmpty() const {
    return instructions.empty();
}
