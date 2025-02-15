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
#include "Core.h"
uint8_t Core::getInstructionLength(ProcessorVersion version, uint8_t opCode) {
  if (version == M6800) {
    return addressingModes[M6800InstructionPage[opCode].mode].size;
  } else if (version == M6803) {
    return addressingModes[M6803InstructionPage[opCode].mode].size;
  }
  return 0;
}
Core::AddressingMode Core::getInstructionMode(ProcessorVersion version, uint8_t opCode) {
  if (version == M6800) {
    return M6800InstructionPage[opCode].mode;
  } else if (version == M6803) {
    return M6803InstructionPage[opCode].mode;
  }
  return AddressingMode::INVALID;
}
uint8_t Core::getInstructionCycleCount(ProcessorVersion version, uint8_t opCode) {
  if (version == M6800) {
    return M6800InstructionPage[opCode].cycleCount;
  } else if (version == M6803) {
    return M6803InstructionPage[opCode].cycleCount;
  }
  return 0;
}
bool Core::getInstructionSupported(ProcessorVersion version, uint8_t opCode) {
  if (version == M6800) {
    if (M6800InstructionPage[opCode].mode == AddressingMode::INVALID) {
      return false;
    }
  } else if (version == M6803) {
    if (M6803InstructionPage[opCode].mode == AddressingMode::INVALID) {
      return false;
    }
  }
  return true;
}

Core::MnemonicInfo Core::getInfoByMnemonic(ProcessorVersion version, QString mnemonic) {
  for (MnemonicInfo info : mnemonics) {
    if (info.mnemonic == mnemonic && (version & info.supportedVersions)) {
      return info;
    }
  }
  return invalidMnemonic;
}

Core::MnemonicInfo Core::getInfoByOpCode(ProcessorVersion version, uint8_t opCode) {
  for (MnemonicInfo info : mnemonics) {
    if (info.opCodes.contains(opCode) && (version & info.supportedVersions)) {
      return info;
    }
  }
  return invalidMnemonic;
}

bool Core::isMnemonic(QString s) {
  return (getInfoByMnemonic(allProcessorVersionsMask, s).mnemonic != "INVALID") || alliasMap.contains(s);
}
