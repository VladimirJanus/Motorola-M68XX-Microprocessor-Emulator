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
#include <QColor>
#include <QMap>
#include <QString>

namespace Core {
  QString softwareVersion;
  QString programName;
  QString envName;

  QColor memoryCellDefaultColor;
  QColor SMMemoryCellColor;
  QColor SMMemoryCellColor2;
  void initialize() {
    static bool initialized = false;
    if (initialized) {
      throw std::logic_error("Core already initialized");
    }
    initialized = true;

#ifdef __linux__
    envName = QStringLiteral("Linux");
#else
    envName = "Windows 10";
#endif
    softwareVersion = "1.11.1";
    programName = "Motorola M68XX Microprocessor Emulator-" + softwareVersion;

    memoryCellDefaultColor = QColor(230, 230, 255);
    SMMemoryCellColor = QColor(150, 150, 150);
    SMMemoryCellColor2 = QColor(204, 204, 204);
  }

  uint8_t getInstructionLength(ProcessorVersion version, uint8_t opCode) {
    if (version == M6800) {
      return addressingModes[M6800InstructionPage[opCode].mode].size;
    } else if (version == M6803) {
      return addressingModes[M6803InstructionPage[opCode].mode].size;
    }
    return 0;
  }

  AddressingMode getInstructionMode(ProcessorVersion version, uint8_t opCode) {
    if (version == M6800) {
      return M6800InstructionPage[opCode].mode;
    } else if (version == M6803) {
      return M6803InstructionPage[opCode].mode;
    }
    return AddressingMode::INVALID;
  }

  uint8_t getInstructionCycleCount(ProcessorVersion version, uint8_t opCode) {
    if (version == M6800) {
      return M6800InstructionPage[opCode].cycleCount;
    } else if (version == M6803) {
      return M6803InstructionPage[opCode].cycleCount;
    }
    return 0;
  }

  bool getInstructionSupported(ProcessorVersion version, uint8_t opCode) {
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

  MnemonicInfo getInfoByMnemonic(ProcessorVersion version, QString mnemonic) {
    for (MnemonicInfo info : mnemonics) {
      if (info.mnemonic == mnemonic && (version & info.supportedVersions)) {
        return info;
      }
    }
    return invalidMnemonic;
  }

  MnemonicInfo getInfoByOpCode(ProcessorVersion version, uint8_t opCode) {
    for (MnemonicInfo info : mnemonics) {
      if (info.opCodes.contains(opCode) && (version & info.supportedVersions)) {
        return info;
      }
    }
    return invalidMnemonic;
  }

  bool isMnemonic(QString s) {
    return (getInfoByMnemonic(ALL, s).mnemonic != "INVALID") || alliasMap.contains(s);
  }

  const QMap<ActionType, ActionTypeInfo> actionTypeInfo = {
      {ActionType::SETBREAKWHEN, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::SETBREAKAT, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::SETBREAKIS, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::SETBOOKMARKBREAKPOINTS, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::UPDATEBOOKMARKS, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::SETRST, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::SETNMI, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::SETIRQ, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::SETKEY, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::SETMOUSECLICK, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::SETMOUSEX, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::SETMOUSEY, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::SETMEMORY, {ActionExecutionTiming::WHEN_READY, false}},
      {ActionType::SETMEMORYBULK, {ActionExecutionTiming::WHEN_READY, true}},
      {ActionType::SETIRQONKEYPRESS, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::SETINCONINVALIDINSTR, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
      {ActionType::UPDATEPROCESSORSPEED, {ActionExecutionTiming::BEFORE_INSTRUCTION, true}},
  };
  const QMap<AddressingMode, AddressingModeInfo> addressingModes = {
      {AdrMode::INVALID, AddressingModeInfo{0, -1, "invalid"}},
      {AdrMode::INH, AddressingModeInfo{1, 0, "inherited"}},
      {AdrMode::IMM, AddressingModeInfo{2, 1, "immediate"}},
      {AdrMode::IMMEXT, AddressingModeInfo{3, 1, "immediate"}},
      {AdrMode::DIR, AddressingModeInfo{2, 2, "direct"}},
      {AdrMode::IND, AddressingModeInfo{2, 3, "indexed"}},
      {AdrMode::EXT, AddressingModeInfo{3, 4, "extended"}},
      {AdrMode::REL, AddressingModeInfo{2, 5, "relative"}},
  };

} // namespace Core
