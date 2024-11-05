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
#include "processor.h"
#include "QtConcurrent/qtconcurrentrun.h"
#include <cstdint>
#include <datatypes.h>
#include <thread>

Processor::Processor() {
  switchVersion(ProcessorVersion::M6800);
}

void Processor::addAction(const Action &action) {
  actionQueue.addAction(action);
  if (!running)
    handleActions();
}

void Processor::handleActions() {
  while (actionQueue.hasActions()) {
    Action action = actionQueue.getNextAction();
    handleAction(action);
  }
}

void Processor::handleAction(const Action& action) {
  switch (action.type) {
  case ActionType::SETBREAKWHEN:
    breakWhenIndex = action.parameter;
    break;
  case ActionType::SETBREAKAT:
    breakAtValue = action.parameter;
    break;
  case ActionType::SETBREAKIS:
    breakIsValue = action.parameter;
    break;
  case ActionType::SETRST:
    if (pendingInterrupt == Interrupt::NONE)
      pendingInterrupt = Interrupt::RST;
    break;
  case ActionType::SETNMI:
    if (pendingInterrupt == Interrupt::NONE)
      pendingInterrupt = Interrupt::NMI;
    break;
  case ActionType::SETIRQ:
    if (pendingInterrupt == Interrupt::NONE)
      pendingInterrupt = Interrupt::IRQ;
    break;
  case ActionType::SETMEMORY:
    Memory[action.parameter & 0xFFFF] = action.parameter >> 16;
    break;
  case ActionType::SETKEY:
    Memory[0xFFF0] = action.parameter;
    if (IRQOnKeyPressed || WAIStatus) {
      if (pendingInterrupt == Interrupt::NONE)
        pendingInterrupt = Interrupt::IRQ;
    }
    break;
  case ActionType::SETMOUSECLICK:
    Memory[0xFFF1] = action.parameter;
    break;
  case ActionType::SETMOUSEX:
    break;
  case ActionType::SETMOUSEY:
    break;
  case ActionType::SETUSECYCLES:
    useCycles = action.parameter;
    break;
  case ActionType::SETIRQONKEYPRESS:
    IRQOnKeyPressed = action.parameter;
    break;
  case ActionType::SETINCONUNKNOWN:
    incrementPCOnMissingInstruction = action.parameter;
    break;
  }
}

void Processor::switchVersion(ProcessorVersion version) {
  processorVersion = version;
  switch (version) {
  case ProcessorVersion::M6800:
    executeInstruction = &Processor::executeM6800;
    break;
  case ProcessorVersion::M6803:
    executeInstruction = &Processor::executeM6803;
    break;
  }
}

inline bool bit(int variable, int bitNum) {
  return (variable & (1 << bitNum)) != 0;
}

void Processor::updateFlags(Flag flag, bool value) {
  switch (flag) {
  case Flag::HalfCarry:
    flags = (flags & ~(1 << 5)) | (value << 5);
    break;
  case Flag::InterruptMask:
    flags = (flags & ~(1 << 4)) | (value << 4);
    break;
  case Flag::Negative:
    flags = (flags & ~(1 << 3)) | (value << 3);
    break;
  case Flag::Zero:
    flags = (flags & ~(1 << 2)) | (value << 2);
    break;
  case Flag::Overflow:
    flags = (flags & ~(1 << 1)) | (value << 1);
    break;
  case Flag::Carry:
    flags = (flags & ~(1)) | (value);
    break;
  default:
    throw;
  }
}

void Processor::reset() {
  stopExecution();
  cycleCount = 0;
  curCycle = 1;
  aReg = 0;
  bReg = 0;
  xReg = 0;
  SP = 0x00FF;
  flags = 0xD0;
  WAIStatus = false;
  pendingInterrupt = Interrupt::NONE;
  std::copy(backupMemory.begin(), backupMemory.end(), Memory.begin());
  PC = (Memory[interruptLocations - 1] << 8) + Memory[interruptLocations];
}

void Processor::executeM6800() {
  count++;
  (this->*M6800Table[Memory[PC]])();
}
void Processor::executeM6803() {
  (this->*M6803Table[Memory[PC]])();
}

void Processor::executeStep() {
  interruptCheckIPS();
}

void Processor::setUIUpdateData() {
  std::array<uint8_t, 0x10000> memoryCopy;
  std::copy(Memory.begin(), Memory.end(), memoryCopy.begin());
  emit uiUpdateData(memoryCopy, curCycle, flags, PC, SP, aReg, bReg, xReg, useCycles);
}
void Processor::checkBreak() {
  switch (breakWhenIndex) {
  case 0:
    return;
  case 1:
    if (instructionList.getObjectByAddress(PC).lineNumber == breakIsValue)
      running = false;
    break;
  case 2:
    if (PC == breakIsValue)
      running = false;
    break;
  case 3:
    if (SP == breakIsValue)
      running = false;
    break;
  case 4:
    if (xReg == breakIsValue)
      running = false;
    break;
  case 5:
    if (aReg == breakIsValue)
      running = false;
    break;
  case 6:
    if (bReg == breakIsValue)
      running = false;
    break;
  case 7:
    if (bit(flags, 5) == breakIsValue)
      running = false;
    break;
  case 8:
    if (bit(flags, 4) == breakIsValue)
      running = false;
    break;
  case 9:
    if (bit(flags, 3) == breakIsValue)
      running = false;
    break;
  case 10:
    if (bit(flags, 2) == breakIsValue)
      running = false;
    break;
  case 11:
    if (bit(flags, 1) == breakIsValue)
      running = false;
    break;
  case 12:
    if (bit(flags, 0) == breakIsValue)
      running = false;
    break;
  case 13:
    if (Memory[breakAtValue] == breakIsValue)
      running = false;
    break;
  }
}
void Processor::pushStateToMemory() {
  Memory[SP--] = PC & 0xFF;
  Memory[SP--] = (PC >> 8);
  Memory[SP--] = xReg & 0xFF;
  Memory[SP--] = (xReg >> 8);
  Memory[SP--] = aReg;
  Memory[SP--] = bReg;
  Memory[SP--] = flags;
}
uint16_t Processor::getInterruptLocation(Interrupt interrupt) {
  return (Memory[(interruptLocations - static_cast<int>(interrupt) * 2 - 1)] << 8) + Memory[(interruptLocations - static_cast<int>(interrupt) * 2)];
}

void Processor::interruptCheckCPS() {
  switch (pendingInterrupt) {
  case Interrupt::NONE:
    (this->*executeInstruction)();
    if (WAIStatus) {
      cycleCount = 0;
    } else {
      cycleCount = DataTypes::getInstructionCycleCount(processorVersion, Memory[PC]);
    }
    break;
  case Interrupt::RST:
    (this->*executeInstruction)();
    cycleCount = 5;
    pendingInterrupt = Interrupt::RSTCYCLESERVICE;
    break;
  case Interrupt::NMI:
    (this->*executeInstruction)();
    if (WAIStatus) {
      cycleCount = 5;
    } else {
      cycleCount = 13;
    }
    pendingInterrupt = Interrupt::NMICYCLESERVICE;
    break;
  case Interrupt::IRQ:
    (this->*executeInstruction)();
    if (!bit(flags, 4)) {
      if (WAIStatus) {
        cycleCount = 5;
      } else {
        cycleCount = 13;
      }
      pendingInterrupt = Interrupt::IRQCYCLESERVICE;
    } else {
      if (WAIStatus) {
        cycleCount = 0;
      } else {
        cycleCount = DataTypes::getInstructionCycleCount(processorVersion, Memory[PC]);
      }
      pendingInterrupt = Interrupt::NONE;
    }
    break;
  case Interrupt::RSTCYCLESERVICE:
    updateFlags(Flag::InterruptMask, 1);
    PC = getInterruptLocation(Interrupt::RST);
    cycleCount = DataTypes::getInstructionCycleCount(processorVersion, Memory[PC]);
    pendingInterrupt = Interrupt::NONE;
    WAIStatus = false;
    break;
  case Interrupt::NMICYCLESERVICE:
    if (!WAIStatus) {
      pushStateToMemory();
    } else {
      WAIStatus = false;
    }
    updateFlags(Flag::InterruptMask, 1);
    PC = getInterruptLocation(Interrupt::NMI);
    cycleCount = DataTypes::getInstructionCycleCount(processorVersion, Memory[PC]);
    pendingInterrupt = Interrupt::NONE;
    break;
  case Interrupt::IRQCYCLESERVICE:
    if (!WAIStatus) {
      pushStateToMemory();
    } else {
      WAIStatus = false;
    }
    updateFlags(Flag::InterruptMask, 1);
    PC = getInterruptLocation(Interrupt::IRQ);
    cycleCount = DataTypes::getInstructionCycleCount(processorVersion, Memory[PC]);
    pendingInterrupt = Interrupt::NONE;
    break;
  }
}
void Processor::interruptCheckIPS() {
  switch (pendingInterrupt) {
  case Interrupt::NONE:
    (this->*executeInstruction)();
    break;
  case Interrupt::RST:
    updateFlags(Flag::InterruptMask, 1);
    PC = getInterruptLocation(Interrupt::RST);
    pendingInterrupt = Interrupt::NONE;
    WAIStatus = false;
    break;
  case Interrupt::NMI:
    if (!WAIStatus) {
      pushStateToMemory();
    }
    updateFlags(Flag::InterruptMask, 1);
    PC = getInterruptLocation(Interrupt::NMI);
    pendingInterrupt = Interrupt::NONE;
    WAIStatus = false;
    break;
  case Interrupt::IRQ:
    if (!bit(flags, 4)) {
      if (!WAIStatus) {
        pushStateToMemory();
      }
      updateFlags(Flag::InterruptMask, 1);
      PC = getInterruptLocation(Interrupt::IRQ);
      WAIStatus = false;
    } else {
      if (WAIStatus == false) {
        (this->*executeInstruction)();
      }
    }
    pendingInterrupt = Interrupt::NONE;
    break;
  }
}
void Processor::startExecution(float OPS, InstructionList list) {
  instructionList = list;
  running = true;
  curCycle = 1;
  cycleCount = DataTypes::getInstructionCycleCount(processorVersion, Memory[PC]);

  int nanoDelay = 1000000000 / OPS;
  int uiUpdateSpeed = 250;
  int batchSize;
  if (OPS > uiUpdateSpeed) {
    batchSize = OPS / uiUpdateSpeed;
  } else {
    batchSize = 1;
  }
  futureWatcher.setFuture(QtConcurrent::run([this, nanoDelay, batchSize]() {
    auto next = std::chrono::steady_clock::now() + std::chrono::nanoseconds(nanoDelay * batchSize);
    while (running) {
      while (std::chrono::steady_clock::now() < next)
        ;

      next = next + std::chrono::nanoseconds(nanoDelay * batchSize);

      handleActions();
      for (int i = 0; i < batchSize; i++) {
        if (!running) {
          break;
        }
        if (useCycles) {
          if (curCycle < cycleCount) {
            curCycle++;
            if (i + 1 == batchSize) {
              setUIUpdateData();
            }
          } else {
            interruptCheckCPS();
            checkBreak();
            curCycle = 1;
            if (i + 1 == batchSize) {
              setUIUpdateData();
            }
          }
        } else {
          interruptCheckIPS();
          checkBreak();
          if (i + 1 == batchSize) {
            setUIUpdateData();
          }
        }
      }
    }
    emit executionStopped();
  }));
}

void Processor::stopExecution() {
  if (running == true) {
    running = false;
    futureWatcher.waitForFinished();
  }
}
