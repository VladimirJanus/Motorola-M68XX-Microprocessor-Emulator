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
#include "datatypes.h"
int DataTypes::getInstructionLength(ProcessorVersion version, uint8_t opCode) {
  if (version == M6800) {
    return M6800InstructionPage[opCode].mode / 10;
  } else if (version == M6803) {
    return M6803InstructionPage[opCode].mode / 10;
  }
  return 0;
}
DataTypes::AddressingMode DataTypes::getInstructionMode(ProcessorVersion version, uint8_t opCode) {
  if (version == M6800) {
    return M6800InstructionPage[opCode].mode;
  } else if (version == M6803) {
    return M6803InstructionPage[opCode].mode;
  }
  return AddressingMode::INVALID;
}
int DataTypes::getInstructionCycleCount(ProcessorVersion version, uint8_t opCode) {
  if (version == M6800) {
    return M6800InstructionPage[opCode].cycleCount;
  } else if (version == M6803) {
    return M6803InstructionPage[opCode].cycleCount;
  }
  return 0;
}
bool DataTypes::getInstructionSupported(ProcessorVersion processorVersion, uint8_t opCode) {
  if (processorVersion == M6800) {
    if (M6800InstructionPage[opCode].mode == AddressingMode::INVALID) {
      return false;
    }
  } else if (processorVersion == M6803) {
    if (M6803InstructionPage[opCode].mode == AddressingMode::INVALID) {
      return false;
    }
  }
  return true;
}
QString DataTypes::getInstructionMnemonic(ProcessorVersion processorVersion, uint8_t opCode) {
  if (processorVersion == M6800) {
    return M6800Mnemonics[opCode];
  } else if (processorVersion == M6803) {
    return M6803Mnemonics[opCode];
  }
  return "INVALID";
}
