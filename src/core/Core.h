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
#ifndef CORE_H
#define CORE_H

#include <QColor>
#include <QList>
#include <QMap>
#include <QString>
namespace Core {
  inline const QString softwareVersion = QStringLiteral("1.10.2");
  inline const QString programName = "Motorola M68XX Microprocessor Emulator-" + softwareVersion;

#ifdef __linux__
  inline const QString envName = QStringLiteral("Linux");
#else
  inline const QString envName = QStringLiteral("Windows 10");
#endif

  inline const QColor memoryCellDefaultColor(230, 230, 255);
  inline const QColor SMMemoryCellColor(150, 150, 150);
  inline const QColor SMMemoryCellColor2(204, 204, 204);

  inline const uint16_t interruptLocations = 0xFFFF;

  class AssemblyMap {
  public:
    struct MappedInstr {
      int address;
      int lineNumber;
      uint8_t byte1;
      uint8_t byte2;
      uint8_t byte3;
      QString IN;
      QString OP;
    };

    void clear() { instructions.clear(); }
    bool isEmpty() const { return instructions.empty(); }

    void addInstruction(
      int address, int lineNumber, uint8_t byte1, uint8_t byte2, uint8_t byte3, QString IN, QString OP) {
      instructions.push_back(MappedInstr{address, lineNumber, byte1, byte2, byte3, IN, OP});
    }

    MappedInstr& getObjectByAddress(
      int address) {
      for (MappedInstr& instruction : instructions) {
        if (instruction.address == address) {
          return instruction;
        }
      }
      static MappedInstr defaultInstruction = {address, -1, 0, 0, 0, "", ""};
      return defaultInstruction;
    }

    MappedInstr& getObjectByLine(
      int lineNumber) {
      for (MappedInstr& instruction : instructions) {
        if (instruction.lineNumber == lineNumber) {
          return instruction;
        }
      }
      static MappedInstr defaultInstruction = {-1, lineNumber, 0, 0, 0, "", ""};
      return defaultInstruction;
    }

  private:
    std::vector<MappedInstr> instructions;
  };

  inline QChar numToChar(uint8_t val){
    if (val < 32 || val >= 127) {
      return '\0';
    } else {
      return QChar(val);
    }
  }
  inline uint8_t charToVal(QChar c){
    return (c.unicode() >= 127 || c.unicode() < 32) ? 0 : c.unicode();
  }

  enum class ColorType {
    HIGHLIGHT,
    RUNTIME,
    HIGHLIGHT_RUNTIME,
    ERROR,
    ERRORCHAR,
    NONE,
  };

  enum class ActionType {
    SETBREAKWHEN,
    SETBREAKAT,
    SETBREAKIS,
    SETBOOKMARKBREAKPOINTS,
    UPDATEBOOKMARKS,
    SETRST,
    SETNMI,
    SETIRQ,
    SETKEY,
    SETMOUSECLICK,
    SETMOUSEX,
    SETMOUSEY,
    SETMEMORY,
    SETUSECYCLES,
    SETIRQONKEYPRESS,
    SETINCONINVALIDINSTR,
  };
  struct Action {
    ActionType type;
    uint32_t parameter;
  };

  enum class Interrupt : int {
      NONE = -1,
      RST = 0, // Value used as an offset from FFFF to locate the interrupt vector
      NMI = 1, // Value used as an offset from FFFF to locate the interrupt vector
      IRQ = 2, // Value used as an offset from FFFF to locate the interrupt vector
      RSTCYCLESERVICE = 3,   // Value not used as a vector offset
      NMICYCLESERVICE = 4,   // Value not used as a vector offset
      IRQCYCLESERVICE = 5,   // Value not used as a vector offset
      // SWI is handled as a normal instruction
  };


  enum Flag : uint8_t {
    HalfCarry = 5,
    InterruptMask = 4,
    Negative = 3,
    Zero = 2,
    Overflow = 1,
    Carry = 0,
  };

  enum class MsgType {
    NONE,
    DEBUG,
    WARN,
    ERROR,
  };
  struct Msg {
    MsgType type;
    QString message;

    static const Msg none() { return Msg{MsgType::NONE, ""}; }
  };

  enum ProcessorVersion : uint8_t {
    M6800 = 0x1,
    M6803 = 0x2,
  };
  inline const ProcessorVersion allProcessorVersionsMask = ProcessorVersion(M6800 | M6803);

  // Addressing modes
  typedef enum class AddressingMode {
    INVALID,
    INH,
    IMM,
    IMMEXT,
    DIR,
    EXT,
    IND,
    REL,
  } AdrMode;

  struct AddressingModeInfo {
    uint8_t size;
    int8_t id;
    QString name;
  };

  // Assembly results
  struct AssemblyError {
    bool ok;
    QString message;
    int errorLineNum;
    int errorCharNum;

    static const AssemblyError failure(QString message, int errorLineNum, int errorCharNum) { return {false, message, errorLineNum, errorCharNum}; }
    static const AssemblyError none() { return AssemblyError{true, "", -1, -1}; }
  };
  struct AssemblyResult {
    QList<Msg> messages;
    AssemblyError error;
    AssemblyMap assemblyMap;
  };
  struct DisassemblyResult {
    QList<Msg> messages;
    QString code;
    AssemblyMap assemblyMap;
  };

  // Instruction and mnemonic data
  struct InstructionInfo {
    AddressingMode mode;
    uint8_t cycleCount;
  };
  struct MnemonicInfo {
    QString mnemonic;
    QList<uint8_t> opCodes;
    QString flags;
    QString shortDescription;
    QString longDescription;
    uint8_t supportedVersions;
  };
  struct Allias {
    QString mnemonic;
    QString shortDescription;
    uint8_t supportedVersions;
  };

  // Methods
  uint8_t getInstructionLength(ProcessorVersion version, uint8_t opCode);
  AddressingMode getInstructionMode(ProcessorVersion version, uint8_t opCode);
  uint8_t getInstructionCycleCount(ProcessorVersion version, uint8_t opCode);
  bool getInstructionSupported(ProcessorVersion version, uint8_t opCode);
  bool isMnemonic(QString s);

  MnemonicInfo getInfoByMnemonic(ProcessorVersion version, QString mnemonic);
  MnemonicInfo getInfoByOpCode(ProcessorVersion version, uint8_t opCode);

  inline bool bit(uint32_t variable, uint8_t bitNum) {
    return (variable & (1 << bitNum)) != 0;
  }

  // Instruction data
  inline const QMap<AddressingMode, AddressingModeInfo> addressingModes = {
    {AdrMode::INVALID, AddressingModeInfo{0, -1, "invalid"}},
    {AdrMode::INH, AddressingModeInfo{1, 0, "inherited"}},
    {AdrMode::IMM, AddressingModeInfo{2, 1, "immediate"}},
    {AdrMode::IMMEXT, AddressingModeInfo{3, 1, "immediate"}},
    {AdrMode::DIR, AddressingModeInfo{2, 2, "direct"}},
    {AdrMode::IND, AddressingModeInfo{2, 3, "indexed"}},
    {AdrMode::EXT, AddressingModeInfo{3, 4, "extended"}},
    {AdrMode::REL, AddressingModeInfo{2, 5, "relative"}},
  };

  inline const QList<InstructionInfo> M6800InstructionPage = {{AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 4},
                                                              {AdrMode::INH, 4},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              //10
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              //20
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              //30
                                                              {AdrMode::INH, 4},
                                                              {AdrMode::INH, 4},
                                                              {AdrMode::INH, 4},
                                                              {AdrMode::INH, 4},
                                                              {AdrMode::INH, 4},
                                                              {AdrMode::INH, 4},
                                                              {AdrMode::INH, 4},
                                                              {AdrMode::INH, 4},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 5},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 10},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 9},
                                                              {AdrMode::INH, 12},
                                                              //40
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              //50
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              //60
                                                              {AdrMode::IND, 7},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IND, 7},
                                                              {AdrMode::IND, 7},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IND, 7},
                                                              {AdrMode::IND, 7},
                                                              {AdrMode::IND, 7},
                                                              {AdrMode::IND, 7},
                                                              {AdrMode::IND, 7},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IND, 7},
                                                              {AdrMode::IND, 7},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 7},
                                                              //70
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 3},
                                                              {AdrMode::EXT, 6},
                                                              //80
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMMEXT, 3},
                                                              {AdrMode::REL, 8},
                                                              {AdrMode::IMMEXT, 3},
                                                              {AdrMode::INVALID, 0},
                                                              //90
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 4},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 4},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::DIR, 4},
                                                              {AdrMode::DIR, 5},
                                                              //A0
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 8},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 7},
                                                              //B0
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 5},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 5},
                                                              {AdrMode::EXT, 9},
                                                              {AdrMode::EXT, 5},
                                                              {AdrMode::EXT, 6},
                                                              //C0
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IMMEXT, 3},
                                                              {AdrMode::INVALID, 0},
                                                              //D0
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 4},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::DIR, 4},
                                                              {AdrMode::DIR, 5},
                                                              //E0
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 7},
                                                              //F0
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 5},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::EXT, 5},
                                                              {AdrMode::EXT, 6}};
  inline const QList<InstructionInfo> M6803InstructionPage = {{AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 3},
                                                              {AdrMode::INH, 3},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 3},
                                                              {AdrMode::INH, 3},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              //10
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              //20
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              {AdrMode::REL, 3},
                                                              //30
                                                              {AdrMode::INH, 3},
                                                              {AdrMode::INH, 3},
                                                              {AdrMode::INH, 4},
                                                              {AdrMode::INH, 4},
                                                              {AdrMode::INH, 3},
                                                              {AdrMode::INH, 3},
                                                              {AdrMode::INH, 3},
                                                              {AdrMode::INH, 3},
                                                              {AdrMode::INH, 5},
                                                              {AdrMode::INH, 5},
                                                              {AdrMode::INH, 3},
                                                              {AdrMode::INH, 10},
                                                              {AdrMode::INH, 4},
                                                              {AdrMode::INH, 10},
                                                              {AdrMode::INH, 9},
                                                              {AdrMode::INH, 12},
                                                              //40
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              //50
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INH, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INH, 2},
                                                              //60
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 3},
                                                              {AdrMode::IND, 6},
                                                              //70
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 3},
                                                              {AdrMode::EXT, 6},
                                                              //80
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMMEXT, 4},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMMEXT, 4},
                                                              {AdrMode::REL, 6},
                                                              {AdrMode::IMMEXT, 3},
                                                              {AdrMode::INVALID, 0},
                                                              //90
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 5},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 5},
                                                              {AdrMode::DIR, 5},
                                                              {AdrMode::DIR, 4},
                                                              {AdrMode::DIR, 4},
                                                              //A0
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              //B0
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 5},
                                                              {AdrMode::EXT, 5},
                                                              //C0
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMMEXT, 4},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMM, 2},
                                                              {AdrMode::IMMEXT, 3},
                                                              {AdrMode::INVALID, 0},
                                                              {AdrMode::IMMEXT, 3},
                                                              {AdrMode::INVALID, 0},
                                                              //D0
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 5},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 3},
                                                              {AdrMode::DIR, 4},
                                                              {AdrMode::DIR, 4},
                                                              {AdrMode::DIR, 4},
                                                              {AdrMode::DIR, 4},
                                                              //E0
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 6},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 4},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              {AdrMode::IND, 5},
                                                              //F0
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 6},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 4},
                                                              {AdrMode::EXT, 5},
                                                              {AdrMode::EXT, 5},
                                                              {AdrMode::EXT, 5},
                                                              {AdrMode::EXT, 5}};

  inline const QList<QString> directivesWithLocation = {".BYTE", ".WORD", ".STR"};

  inline const QMap<QString, Allias> alliasMap = {{"BYTE", {".BYTE", "Allias for .BYTE", M6800 | M6803}},
                                                  {"EQU", {".EQU", "Allias for .EQU", M6800 | M6803}},
                                                  {"ORG", {".ORG", "Allias for .ORG", M6800 | M6803}},
                                                  {"RMB", {".RMB", "Allias for .RMB", M6800 | M6803}},
                                                  {"SETB", {".SETB", "Allias for .SETB", M6800 | M6803}},
                                                  {"SETW", {".SETW", "Allias for .SETW", M6800 | M6803}},
                                                  {"STR", {".STR", "Allias for .STR", M6800 | M6803}},
                                                  {"WORD", {".WORD", "Allias for .WORD", M6800 | M6803}},
                                                  {"LSL", {"ASL", "Allias for ASL.", M6803}},
                                                  {"LSLA", {"ASLA", "Allias for ASLA.", M6803}},
                                                  {"LSLB", {"ASLB", "Allias for ASLB.", M6803}},
                                                  {"LSLD", {"ASLD", "Allias for ASLD.", M6803}},
                                                  {"BHS", {"BCC", "Alias for BCC. Branch if *unsigned* value higher or same", M6803}},
                                                  {"BLO", {"BCS", "Allias for BCS. Branch if *unsigned* value is lower", M6803}}};
  inline const MnemonicInfo invalidMnemonic{"INVALID", {0, 0, 0, 0, 0, 0}, "", "", "", 0};
  inline const QList<MnemonicInfo> mnemonics = {
    {".BYTE", {}, "", "Set Byte", "Sets a 1-byte value or an array of such values to the current address. Values can also be written in an array separated by commas.", M6800 | M6803},
    {".EQU", {}, "", "Set Constant", "Associates a specified value with a symbol/label.", M6800 | M6803},
    {".ORG", {}, "", "Set Compilation Address", "Sets the current compilation address. Writes the operand to the reset pointer((n-1):n).", M6800 | M6803},
    {".RMB", {}, "", "Reserve Memory", "Reserves a specified amount of memory, subsequently incrementing the current compilation address by that specified amount.", M6800 | M6803},
    {".SETB", {}, "", "Set Byte At Address", "Sets a 1-byte value at a specified address. The value and address are separated by a comma.", M6800 | M6803},
    {".SETW", {}, "", "Set Word At Address", "Sets a 2-byte value at a specified address. The value and address are separated by a comma.", M6800 | M6803},
    {".STR", {}, "", "Set String", "Writes a string encapsulated by quotes to the current compilation address.", M6800 | M6803},
    {".WORD", {}, "", "Set Word", "Sets a 2-byte value or an array of such values to the current compilation address. Values in an array are separated by a comma.", M6800 | M6803},
    {"ABA", {0x1B, 0, 0, 0, 0, 0}, "*-****", "ACCA <- ACCA + ACCB", "Adds the contents of ACCB to the contents of ACCA and places the result in ACCA.", M6800 | M6803},
    {"ABX", {0x3A, 0, 0, 0, 0, 0}, "------", "X <- X + 00:B", "Adds the contents of ACCB to the contents of the X register and places the result in the X register.", M6803},
    {"ADCA", {0, 0x89, 0x99, 0xA9, 0xB9, 0}, "*-****", "ACCA <- ACCA + M + C", "Adds the contents of the C bit to the sum of the contents of ACCA and M, and places the result in ACCA.", M6800 | M6803},
    {"ADCB", {0, 0xC9, 0xD9, 0xE9, 0xF9, 0}, "*-****", "ACCB <- ACCB + M + C", "Adds the contents of the C bit to the sum of the contents of ACCB and M, and places the result in ACCB.", M6800 | M6803},
    {"ADDA", {0, 0x8B, 0x9B, 0xAB, 0xBB, 0}, "*-****", "ACCA <- ACCA + M", "Adds the contents of ACCA and the contents of M and places the result in ACCA.", M6800 | M6803},
    {"ADDB", {0, 0xCB, 0xDB, 0xEB, 0xFB, 0}, "*-****", "ACCB <- ACCB + M", "Adds the contents of ACCB and the contents of M and places the result in ACCB.", M6800 | M6803},
    {"ADDD", {0, 0xC3, 0xD3, 0xE3, 0xF3, 0}, "--****", "ACCA:ACCB <- ACCA:ACCB + M:(M+1)", "Adds the contents of ACCA:ACCB and the contents of M:(M+1) and places the result in ACCA:ACCB.", M6803},
    {"ANDA",
     {0, 0x84, 0x94, 0xA4, 0xB4, 0},
     "--**0-",
     "ACCA <- ACCA · M",
     "Performs logical 'AND' between the contents of ACCA and the contents of M and places the result in ACCA. (Each bit of ACCA after the operation will be the logical 'AND' of the corresponding bits of M and of ACCA before the operation.)",
     M6800 | M6803},
    {"ANDB",
     {0, 0xC4, 0xD4, 0xE4, 0xF4, 0},
     "--**0-",
     "ACCB <- ACCB · M",
     "Performs logical 'AND' between the contents of ACCB and the contents of M and places the result in ACCB. (Each bit of ACCB after the operation will be the logical 'AND' of the corresponding bits of M and of ACCB before the operation.)",
     M6800 | M6803},
    {"ASL", {0, 0, 0, 0x68, 0x78, 0}, "--****", "M arithmetic/logical shift left", "Shifts all bits of the M one place to the left. Bit 0 is loaded with a zero. The C bit is loaded from the most significant bit of M.", M6800 | M6803},
    {"ASLA", {0x48, 0, 0, 0, 0, 0}, "--****", "ACCA arithmetic/logical shift left", "Shifts all bits of the ACCA one place to the left. Bit 0 is loaded with a zero. The C bit is loaded from the most significant bit of ACCA.", M6800 | M6803},
    {"ASLB", {0x58, 0, 0, 0, 0, 0}, "--****", "ACCB arithmetic/logical shift left", "Shifts all bits of the ACCB one place to the left. Bit 0 is loaded with a zero. The C bit is loaded from the most significant bit of ACCB.", M6800 | M6803},
    {"ASLD", {0x05, 0, 0, 0, 0, 0}, "--****", "ACCA:ACCB arithmetic/logical shift left", "Shifts all bits of the ACCA:ACCB one place to the left. Bit 0 is loaded with a zero. The C bit is loaded from the most significant bit of ACCA:ACCB.", M6803},
    {"ASR", {0, 0, 0, 0x67, 0x77, 0}, "--****", "M arithmetic shift right", "Shifts all bits of M one place to the right. Bit 7 is held constant. Bit 0 is loaded into the C bit.", M6800 | M6803},
    {"ASRA", {0x47, 0, 0, 0, 0, 0}, "--****", "ACCA arithmetic shift right", "Shifts all bits of ACCA one place to the right. Bit 7 is held constant. Bit 0 is loaded into the C bit.", M6800 | M6803},
    {"ASRB", {0x57, 0, 0, 0, 0, 0}, "--****", "ACCB arithmetic shift right", "Shifts all bits of ACCB one place to the right. Bit 7 is held constant. Bit 0 is loaded into the C bit.", M6800 | M6803},
    {"BCC", {0, 0, 0, 0, 0, 0x24}, "------", "Branch if carry clear (C=0, unsigned)", "Branches if the carry flag (C) is clear, meaning no carry occurred. Used in unsigned comparisons.", M6800 | M6803},
    {"BCS", {0, 0, 0, 0, 0, 0x25}, "------", "Branch if carry set (C=1, unsigned)", "Branches if the carry flag (C) is set, meaning a carry occurred. Used in unsigned comparisons.", M6800 | M6803},
    {"BEQ", {0, 0, 0, 0, 0, 0x27}, "------", "Branch if equal (Z=1, signed/unsigned)", "Branches if the zero flag (Z) is set, indicating equality. Used in both signed and unsigned comparisons.", M6800 | M6803},
    {"BGE", {0, 0, 0, 0, 0, 0x2C}, "------", "Branch if greater or equal (N=V, signed)", "Branches if the negative flag (N) equals the overflow flag (V). Used for signed comparisons to check if a value is greater than or equal.", M6800 | M6803},
    {"BGT", {0, 0, 0, 0, 0, 0x2E}, "------", "Branch if greater (Z=0 AND N=V, signed)", "Branches if zero (Z) is clear and negative (N) equals overflow (V). Used for signed comparisons to check if a value is greater.", M6800 | M6803},
    {"BHI", {0, 0, 0, 0, 0, 0x22}, "------", "Branch if higher (Z=0 AND C=0, unsigned)", "Branches if carry (C) and zero (Z) flags are clear. Used for unsigned comparisons to check if a value is greater.", M6800 | M6803},
    {"BITA", {0, 0x85, 0x95, 0xA5, 0xB5, 0}, "--**0-", "Test bit (ACCA AND M)", "Performs logical AND between ACCA and memory, updating flags based on the result.", M6800 | M6803},
    {"BITB", {0, 0xC5, 0xD5, 0xE5, 0xF5, 0}, "--**0-", "Test bit (ACCB AND M)", "Performs logical AND between ACCB and memory, updating flags based on the result.", M6800 | M6803},
    {"BLE", {0, 0, 0, 0, 0, 0x2F}, "------", "Branch if less or equal (Z=1 OR N!=V, signed)", "Branches if zero (Z) is set or negative (N) does not equal overflow (V). Used for signed comparisons to check if a value is less than or equal.", M6800 | M6803},
    {"BLS", {0, 0, 0, 0, 0, 0x23}, "------", "Branch if lower or same (Z=1 OR C=1, unsigned)", "Branches if carry (C) or zero (Z) flag is set. Used for unsigned comparisons to check if a value is less than or equal.", M6800 | M6803},
    {"BLT", {0, 0, 0, 0, 0, 0x2D}, "------", "Branch if less (N!=V, signed)", "Branches if negative (N) does not equal overflow (V). Used for signed comparisons to check if a value is less.", M6800 | M6803},
    {"BMI", {0, 0, 0, 0, 0, 0x2B}, "------", "Branch if negative (N=1, signed)", "Branches if negative flag (N) is set, indicating a negative result in signed arithmetic.", M6800 | M6803},
    {"BNE", {0, 0, 0, 0, 0, 0x26}, "------", "Branch if not equal (Z=0, signed/unsigned)", "Branches if zero flag (Z) is clear, indicating inequality. Used in both signed and unsigned comparisons.", M6800 | M6803},
    {"BPL", {0, 0, 0, 0, 0, 0x2A}, "------", "Branch if positive (N=0, signed)", "Branches if negative flag (N) is clear, indicating a positive result in signed arithmetic.", M6800 | M6803},
    {"BRA", {0, 0, 0, 0, 0, 0x20}, "------", "Branch always", "Unconditional branch to a relative address.", M6800 | M6803},
    {"BRN", {0, 0, 0, 0, 0, 0x21}, "------", "Branch never", "This instruction never branches.", M6803},
    {"BSR", {0, 0, 0, 0, 0, 0x8D}, "------", "Branch to subroutine", "Pushes the return address to the stack and branches to a subroutine.", M6800 | M6803},
    {"BVC", {0, 0, 0, 0, 0, 0x28}, "------", "Branch if overflow clear (V=0, signed)", "Branches if overflow flag (V) is clear. Used in signed arithmetic.", M6800 | M6803},
    {"BVS", {0, 0, 0, 0, 0, 0x29}, "------", "Branch if overflow set (V=1, signed)", "Branches if overflow flag (V) is set. Used in signed arithmetic.", M6800 | M6803},
    {"CBA", {0x11, 0, 0, 0, 0, 0}, "--****", "ACCA-ACCB", "Compares the contents of ACCA and the contents of ACCB and sets the condition codes, which may be used for arithmetic and logical conditional branches. Both operands are unaffected.", M6800 | M6803},
    {"CLC", {0x0C, 0, 0, 0, 0, 0}, "-----0", "C <- 0", "Clears the carry bit in the processor condition codes register.", M6800 | M6803},
    {"CLI", {0x0E, 0, 0, 0, 0, 0}, "-0----", "I <- 0", "Clears the interrupt mask bit in the processor condition codes register.", M6800 | M6803},
    {"CLR", {0, 0, 0, 0x6F, 0x7F, 0}, "--0100", "M <- 00", "The contents of M are replaced with zeros.", M6800 | M6803},
    {"CLRA", {0x4F, 0, 0, 0, 0, 0}, "--0100", "ACCA <- 00", "The contents of ACCA are replaced with zeros.", M6800 | M6803},
    {"CLRB", {0x5F, 0, 0, 0, 0, 0}, "--0100", "ACCB <- 00", "The contents of ACCB are replaced with zeros.", M6800 | M6803},
    {"CLV", {0x0A, 0, 0, 0, 0, 0}, "----0-", "V <- 0", "Clears the two's complement overflow bit in the processor condition codes register.", M6800 | M6803},
    {"CMPA", {0, 0x81, 0x91, 0xA1, 0xB1, 0}, "--****", "ACCA - M", "Compares the contents of ACCA and the contents of M and determines the condition codes, which may be used subsequently for controlling conditional branching. Both operands are unaffected.", M6800 | M6803},
    {"CMPB", {0, 0xC1, 0xD1, 0xE1, 0xF1, 0}, "--****", "ACCB - M", "Compares the contents of ACCB and the contents of M and determines the condition codes, which may be used subsequently for controlling conditional branching. Both operands are unaffected.", M6800 | M6803},
    {"COM", {0, 0, 0, 0x63, 0x73, 0}, "--**01", "M <- FF - M", "Replaces the contents of M with its one's complement. (Each bit of the contents of M is replaced with the complement of that bit.)", M6800 | M6803},
    {"COMA", {0x43, 0, 0, 0, 0, 0}, "--**01", "ACCA <- FF - ACCA", "Replaces the contents of ACCA with its one's complement. (Each bit of the contents of ACCA is replaced with the complement of that bit.)", M6800 | M6803},
    {"COMB", {0x53, 0, 0, 0, 0, 0}, "--**01", "ACCB <- FF - ACCB", "Replaces the contents of ACCB with its one's complement. (Each bit of the contents of ACCB is replaced with the complement of that bit.)", M6800 | M6803},
    {"CPX",
     {0, 0x8C, 0x9C, 0xAC, 0xBC, 0},
     "--****",
     "X - M:(M+1)",
     "Compares the high byte of index register with memory at specified address, and low byte with memory at address+1. Sets Z flag based on results for conditional branching. N and V flags are affected but not intended for branching.",
     M6800 | M6803},
    {"DAA",
     {0x19, 0, 0, 0, 0, 0},
     "--****",
     "decimal adjust ACCA",
     "Adjusts the contents of the ACCA register after arithmetic operations with binary-coded-decimal (BCD) operands. It ensures proper representation of BCD sums by adjusting both high and low nibbles, considering the carry bit's state.",
     M6800 | M6803},
    {"DEC", {0, 0, 0, 0x6A, 0x7A, 0}, "--***-", "M <- M - 1", "Subtract one from the contents of M.", M6800 | M6803},
    {"DECA", {0x4A, 0, 0, 0, 0, 0}, "--***-", "ACCA <- ACCA - 1", "Subtract one from the contents of ACCA.", M6800 | M6803},
    {"DECB", {0x5A, 0, 0, 0, 0, 0}, "--***-", "ACCB <- ACCB - 1", "Subtract one from the contents of ACCB.", M6800 | M6803},
    {"DES", {0x34, 0, 0, 0, 0, 0}, "------", "SP <- SP - 1", "Subtract one from the stack pointer.", M6800 | M6803},
    {"DEX", {0x09, 0, 0, 0, 0, 0}, "---*--", "X <- X - 1", "Subtract one from the index register.", M6800 | M6803},
    {"EORA",
     {0, 0x88, 0x98, 0xA8, 0xB8, 0},
     "--**0-",
     "ACCA <- ACCA XOR M",
     "Perform logical 'EXCLUSIVE OR' between the contents of ACCA and the contents of M, and place the result in ACCA. (Each bit of ACCA after the operation will be the logical 'EXCLUSIVE OR' of the corresponding bit of M and ACCA before the operation.)",
     M6800 | M6803},
    {"EORB",
     {0, 0xC8, 0xD8, 0xE8, 0xF8, 0},
     "--**0-",
     "ACCB <- ACCB XOR M",
     "Perform logical 'EXCLUSIVE OR' between the contents of ACCB and the contents of M, and place the result in ACCB. (Each bit of ACCB after the operation will be the logical 'EXCLUSIVE OR' of the corresponding bit of M and ACCB before the operation.)",
     M6800 | M6803},
    {"INC", {0, 0, 0, 0x6C, 0x7C, 0}, "--***-", "M <- M + 1", "Add one to the contents of M.", M6800 | M6803},
    {"INCA", {0x4C, 0, 0, 0, 0, 0}, "--***-", "ACCA <- ACCA + 1", "Add one to the contents of ACCA.", M6800 | M6803},
    {"INCB", {0x5C, 0, 0, 0, 0, 0}, "--***-", "ACCB <- ACCB + 1", "Add one to the contents of ACCB.", M6800 | M6803},
    {"INS", {0x31, 0, 0, 0, 0, 0}, "------", "SP <- SP + 1", "Add one to the stack pointer.", M6800 | M6803},
    {"INX", {0x08, 0, 0, 0, 0, 0}, "---*--", "X <- X + 1", "Add one to the index register.", M6800 | M6803},
    {"JMP", {0, 0, 0, 0x6E, 0x7E, 0}, "------", "Jump to address", "A jump occurs to the instruction stored at the numerical address.", M6800 | M6803},
    {"JSR",
     {0, 0, 0x9D, 0xAD, 0xBD, 0},
     "------",
     "Jump to subroutine",
     "The program counter is incremented by 3 or by 2, depending on the addressing mode, and is then pushed onto the stack, eight bits at a time. The stack pointer points to the next empty location in the stack. A jump occurs to the instruction stored at the numerical address.",
     M6800 | M6803},
    {"LDAA", {0, 0x86, 0x96, 0xA6, 0xB6, 0}, "--**0-", "ACCA <- M", "Loads the contents of memory into the accumulator. The condition codes are set according to the data.", M6800 | M6803},
    {"LDAB", {0, 0xC6, 0xD6, 0xE6, 0xF6, 0}, "--**0-", "ACCB <- M", "Loads the contents of memory into the accumulator. The condition codes are set according to the data.", M6800 | M6803},
    {"LDD", {0, 0xCC, 0xDC, 0xEC, 0xFC, 0}, "--**0-", "ACCA:ACCB <- M:(M+1)", "Loads ACCA with the contents of M and ACCB with the contents of M+1.", M6803},
    {"LDS",
     {0, 0x8E, 0x9E, 0xAE, 0xBE, 0},
     "--**0-",
     "SP <- M:(M+1)",
     "Loads the more significant byte of the stack pointer from the byte of memory at the address specified by the program, and loads the less significant byte of the stack pointer from the next byte of memory, at one plus the address specified by the program.",
     M6800 | M6803},
    {"LDX",
     {0, 0xCE, 0xDE, 0xEE, 0xFE, 0},
     "--**0-",
     "X <- M:(M+1)",
     "Loads the more significant byte of the index register from the byte of memory at the address specified by the program, and loads the less significant byte of the index register from the next byte of memory, at one plus the address specified by the program.",
     M6800 | M6803},
    {"LSR", {0, 0, 0, 0x64, 0x74, 0}, "--0***", "M logical shift right", "Shifts all bits of M one place to the right. Bit 7 is loaded with a zero. The C bit is loaded from the least significant bit of M.", M6800 | M6803},
    {"LSRA", {0x44, 0, 0, 0, 0, 0}, "--0***", "ACCA logical shift right", "Shifts all bits of ACCA one place to the right. Bit 7 is loaded with a zero. The C bit is loaded from the least significant bit of ACCA.", M6800 | M6803},
    {"LSRB", {0x54, 0, 0, 0, 0, 0}, "--0***", "ACCB logical shift right", "Shifts all bits of ACCB one place to the right. Bit 7 is loaded with a zero. The C bit is loaded from the least significant bit of ACCB.", M6800 | M6803},
    {"LSRD", {0x04, 0, 0, 0, 0, 0}, "--0***", "ACCA:ACCB logical shift right", "Shifts all bits of ACCA:ACCB one place to the right. Bit 7 is loaded with a zero. The C bit is loaded from the least significant bit of ACCA:ACCB.", M6803},
    {"MUL", {0x3D, 0, 0, 0, 0, 0}, "-----*", "ACCA:ACCB <- ACCA * ACCB", "Multiplies ACCA and ACCB and stores the result in ACCA:ACCB.", M6803},
    {"NEG", {0, 0, 0, 0x60, 0x70, 0}, "--****", "M <- 00 - M", "Replaces the contents of M with its two's complement. Note that 80 is left unchanged.", M6800 | M6803},
    {"NEGA", {0x40, 0, 0, 0, 0, 0}, "--****", "ACCA <- 00 - ACCA", "Replaces the contents of ACCA with its two's complement. Note that 80 is left unchanged.", M6800 | M6803},
    {"NEGB", {0x50, 0, 0, 0, 0, 0}, "--****", "ACCB <- 00 - ACCB", "Replaces the contents of ACCB with its two's complement. Note that 80 is left unchanged.", M6800 | M6803},
    {"NOP", {0x01, 0, 0, 0, 0, 0}, "------", "No operation", "This is a single-word instruction which causes only the program counter to be incremented. No other registers are affected.", M6800 | M6803},
    {"ORAA",
     {0, 0x8A, 0x9A, 0xAA, 0xBA, 0},
     "--**0-",
     "ACCA <- ACCA ∨ M",
     "Perform logical 'OR' between the contents of ACCA and the contents of M and places the result in ACCA. (Each bit of ACCA after the operation will be the logical 'OR' of the corresponding bits of M and of ACCA before the operation).",
     M6800 | M6803},
    {"ORAB",
     {0, 0xCA, 0xDA, 0xEA, 0xFA, 0},
     "--**0-",
     "ACCB <- ACCB ∨ M",
     "Perform logical 'OR' between the contents of ACCB and the contents of M and places the result in ACCB. (Each bit of ACCB after the operation will be the logical 'OR' of the corresponding bits of M and of ACCB before the operation).",
     M6800 | M6803},
    {"PSHA", {0x36, 0, 0, 0, 0, 0}, "------", "Push ACCA", "The contents of ACCA is stored in the stack at the address contained in the stack pointer. The stack pointer is then decremented.", M6800 | M6803},
    {"PSHB", {0x37, 0, 0, 0, 0, 0}, "------", "Push ACCB", "The contents of ACCB is stored in the stack at the address contained in the stack pointer. The stack pointer is then decremented.", M6800 | M6803},
    {"PSHX", {0x3C, 0, 0, 0, 0, 0}, "------", "Push X", "The contents of the most significant byte of the X register is stored in the stack at the address contained in the stack pointer. The stack pointer is then decremented. The same then happens for the least significant byte of X.", M6803},
    {"PULA", {0x32, 0, 0, 0, 0, 0}, "------", "Pull ACCA", "The stack pointer is incremented. The ACCA is then loaded from the stack, from the address which is contained in the stack pointer.", M6800 | M6803},
    {"PULB", {0x33, 0, 0, 0, 0, 0}, "------", "Pull ACCB", "The stack pointer is incremented. The ACCB is then loaded from the stack, from the address which is contained in the stack pointer.", M6800 | M6803},
    {"PULX",
     {0x38, 0, 0, 0, 0, 0},
     "------",
     "Pull X",
     "SP is incremented. The data at the address which is contained in the SP is then loaded from the stack to the least significant byte of X register. Then SP is incremented again, and the data at the address of SP is stored to the most significant byte of the X register.",
     M6803},
    {"ROL", {0, 0, 0, 0x69, 0x79, 0}, "--****", "M rotate left", "Shifts all bits of M one place to the left. Bit 0 is loaded from the C bit. The C bit is loaded from the most significant bit of M.", M6800 | M6803},
    {"ROLA", {0x49, 0, 0, 0, 0, 0}, "--****", "ACCA rotate left", "Shifts all bits of ACCA one place to the left. Bit 0 is loaded from the C bit. The C bit is loaded from the most significant bit of ACCA.", M6800 | M6803},
    {"ROLB", {0x59, 0, 0, 0, 0, 0}, "--****", "ACCB rotate left", "Shifts all bits of ACCB one place to the left. Bit 0 is loaded from the C bit. The C bit is loaded from the most significant bit of ACCB.", M6800 | M6803},
    {"ROR", {0, 0, 0, 0x66, 0x76, 0}, "--****", "M rotate right", "Shifts all bits of M one place to the right. Bit 7 is loaded from the C bit. The C bit is loaded from the least significant bit of M.", M6800 | M6803},
    {"RORA", {0x46, 0, 0, 0, 0, 0}, "--****", "ACCA rotate right", "Shifts all bits of ACCA one place to the right. Bit 7 is loaded from the C bit. The C bit is loaded from the least significant bit of ACCA.", M6800 | M6803},
    {"RORB", {0x56, 0, 0, 0, 0, 0}, "--****", "ACCB rotate right", "Shifts all bits of ACCB one place to the right. Bit 7 is loaded from the C bit. The C bit is loaded from the least significant bit of ACCB.", M6800 | M6803},
    {"RTI",
     {0x3B, 0, 0, 0, 0, 0},
     "******",
     "Return from interrupt",
     "The condition codes, accumulators B and A, the index register, and the program counter, will be restored to a state pulled from the stack. Note that the interrupt mask bit will be reset if and only if the corresponding bit stored in the stack is zero.",
     M6800 | M6803},
    {"RTS",
     {0x39, 0, 0, 0, 0, 0},
     "------",
     "Return from subroutine",
     "The stack pointer is incremented (by 1). The contents of the byte of memory, at the address now contained in the stack pointer, are loaded into the 8 bits of highest significance in the program counter. The stack pointer is again incremented (by 1). The contents of the byte of memory, at "
     "the address now contained in the stack pointer, are loaded into the 8 bits of lowest significiance in the program counter.",
     M6800 | M6803},
    {"SBA", {0x10, 0, 0, 0, 0, 0}, "--****", "ACCA <- ACCA - ACCB", "Subtracts the contents of ACCB from the contents of ACCA and places the result in ACCA. The contents of ACCB are not affected.", M6800 | M6803},
    {"SBCA", {0, 0x82, 0x92, 0xA2, 0xB2, 0}, "--****", "ACCA <- ACCA - M - C", "Subtracts the contents. of M and C from the contents of ACCA and places the result in ACCA.", M6800 | M6803},
    {"SBCB", {0, 0xC2, 0xD2, 0xE2, 0xF2, 0}, "--****", "ACCB <- ACCB - M - C", "Subtracts the contents. of M and C from the contents of ACCB and places the result in ACCB.", M6800 | M6803},
    {"SEC", {0x0D, 0, 0, 0, 0, 0}, "-----1", "C <- 1", "Sets the carry bit in the processor condition codes register.", M6800 | M6803},
    {"SEI", {0x0F, 0, 0, 0, 0, 0}, "-1----", "I <- 1", "Sets the interrupt mask bit in the processor condition codes register.", M6800 | M6803},
    {"SEV", {0x0B, 0, 0, 0, 0, 0}, "----1-", "V <- 1", "Sets the two's complement overflow bit in the processor condition codes register.", M6800 | M6803},
    {"STAA", {0, 0, 0x97, 0xA7, 0xB7, 0}, "--**0-", "M <- ACCA", "Stores the contents of ACCA in memory. The contents of ACCA remain unchanged.", M6800 | M6803},
    {"STAB", {0, 0, 0xD7, 0xE7, 0xF7, 0}, "--**0-", "M <- ACCB", "Stores the contents of ACCB in memory. The contents of ACCB remain unchanged.", M6800 | M6803},
    {"STD", {0, 0, 0xDD, 0xED, 0xFD, 0}, "--**0-", "M:(M+1) <- ACCA:ACCB", "Stores ACCA at address M and ACCB at address M+1.", M6803},
    {"STS",
     {0, 0, 0x9F, 0xAF, 0xBF, 0},
     "--**0-",
     "M:(M+1) <- SP",
     "Stores the more significant byte of the stack pointer in memory at the address specified by the program, and stores the less significant byte of the stack pointer at the next location in memory, at one plus the address specified by the program.",
     M6800 | M6803},
    {"STX",
     {0, 0, 0xDF, 0xEF, 0xFF, 0},
     "--**0-",
     "M:(M+1) <- X",
     "Stores the more significant byte of the index register in memory at the address specified by the program, and stores the less significant byte of the index register at the next location in memory, at one plus the address specified by the program.",
     M6800 | M6803},
    {"SUBA", {0, 0x80, 0x90, 0xA0, 0xB0, 0}, "--****", "ACCA <- ACCA - M", "Subtracts the contents of M from the contents of ACCA and places the result in ACCA.", M6800 | M6803},
    {"SUBB", {0, 0xC0, 0xD0, 0xE0, 0xF0, 0}, "--****", "ACCB <- ACCB - M", "Subtracts the contents of M from the contents of ACCB and places the result in ACCB.", M6800 | M6803},
    {"SUBD", {0, 0x83, 0x93, 0xA3, 0xB3, 0}, "--****", "ACCA:ACCB <- ACCA:ACCB - M:(M+1)", "Subtracts the contents of M:(M+1) from the contents of ACCA:ACCB and places the result in ACCA:ACCB.", M6803},
    {"SWI",
     {0x3F, 0, 0, 0, 0, 0},
     "-1----",
     "Software Interrupt",
     "The program counter increments by 1. The PC, index register, and accumulators A and B are pushed onto the stack. The condition codes register follows, with H, I, N, Z, V, C in bit positions 5-0, while bits 7-6 are set to 1. The stack pointer decrements after each byte is stored. The "
     "interrupt mask bit is set. Finally, the PC loads the address from the software interrupt pointer at memory locations (n-5) and (n-4), where n is the address $FFFF.",
     M6800 | M6803},
    {"TAB", {0x16, 0, 0, 0, 0, 0}, "--**0-", "ACCB <- ACCA", "Moves the contents of ACCA to ACCB. The former contents of ACCB are lost. The contents of ACCA are not affected.", M6800 | M6803},
    {"TAP", {0x06, 0, 0, 0, 0, 0}, "******", "11HINZVC <- ACCA", "Transfers bits 0-5 of ACCA to the corresponding condition code bits.", M6800 | M6803},
    {"TBA", {0x17, 0, 0, 0, 0, 0}, "--**0-", "ACCA <- ACCB", "Moves the contents of ACCB to ACCA. The former contents of ACCA are lost. The contents of ACCB are not affected.", M6800 | M6803},
    {"TPA", {0x07, 0, 0, 0, 0, 0}, "------", "ACCA <- 11HINZVC", "Transfers condition code bits to bits 0-5 of ACCA, and sets bits 6-7 of ACCA.", M6800 | M6803},
    {"TST", {0, 0, 0, 0x6D, 0x7D, 0}, "--**00", "M - 00", "Set condition codes N and Z according to the contents of M. The V and C flags are cleared.", M6800 | M6803},
    {"TSTA", {0x4D, 0, 0, 0, 0, 0}, "--**00", "ACCA - 00", "Set condition codes N and Z according to the contents of ACCA. The V and C flags are cleared.", M6800 | M6803},
    {"TSTB", {0x5D, 0, 0, 0, 0, 0}, "--**00", "ACCB - 00", "Set condition codes N and Z according to the contents of ACCB. The V and C flags are cleared.", M6800 | M6803},
    {"TSX", {0x30, 0, 0, 0, 0, 0}, "------", "X <- SP + 1", "Loads the index register with one plus the contents of the stack pointer. The contents of the stack pointer remain unchanged.", M6800 | M6803},
    {"TXS", {0x35, 0, 0, 0, 0, 0}, "------", "SP <- X - 1", "Loads the stack pointer with the contents of the index register, minus one. The contents of the index register remain unchanged.", M6800 | M6803},
    {"WAI", {0x3E, 0, 0, 0, 0, 0}, "------", "Wait for interrupt", "Halt program execution and waits for an interrupt.", M6800 | M6803},
  };
} // namespace Core

#endif // CORE_H
