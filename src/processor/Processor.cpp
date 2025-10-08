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
#include "src/processor/Processor.h"

Processor::Processor(ProcessorVersion version) {
  switchVersion(version);
}

/**
 * @brief Switches the processor to the specified version.
 *
 * Updates the processor version and assigns the corresponding
 * execute function to the function pointer.
 *
 * @param version The processor version to switch to.
 */
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
/**
 * @brief Adds an action to the processor's action queue.
 *
 * The provided action is added to the queue. If the processor is not
 * currently running, it immediately handles the actions.
 *
 * @param action The action to add.
 */
void Processor::addAction(const Action &action) {
  actionQueue.addAction(action);
  if (!running)
    handleActions();
}

/**
 * @brief Processes all pending actions in the queue.
 *
 * Continues retrieving and handling actions until there are no more actions
 * left in the queue.
 */
void Processor::handleActions() {
  while (actionQueue.hasActions()) {
    Action action = actionQueue.getNextAction();
    handleAction(action);
  }
}
QVector<int> newBookmarkedAddresses;
void Processor::queueBookmarkData(QVector<int> data){
    newBookmarkedAddresses = data;
}
/**
 * @brief Handles a single action based on its type.
 *
 * This function inspects the type of the action and updates the processor's
 * state accordingly.
 *
 * @param action The action to handle.
 */
void Processor::handleAction(const Action &action) {
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
  case ActionType::SETBOOKMARKBREAKPOINTS:
      bookmarkBreakpointsEnabled = action.parameter;
      break;
  case ActionType::UPDATEBOOKMARKS:
      bookmarkedAddresses = newBookmarkedAddresses;
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
    Memory[action.parameter & 0xFFFF] = (action.parameter >> 16) & 0xFF;
    break;
  case ActionType::SETKEY:
    Memory[0xFFF0] = action.parameter & 0xFF;
    if (IRQOnKeyPressed || WAIStatus) {
      if (pendingInterrupt == Interrupt::NONE)
        pendingInterrupt = Interrupt::IRQ;
    }
    break;
  case ActionType::SETMOUSECLICK:
    Memory[0xFFF1] = action.parameter & 0xFF;
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

/**
 * @brief Checks if a breakpoint condition is met.
 *
 * Depending on the breakWhenIndex and breakIsValue, various processor states
 * (e.g. PC, SP, register values, flag bits, or memory contents) are checked.
 * If the condition is satisfied, the processor execution is halted.
 */
void Processor::checkBreak() {
  switch (breakWhenIndex) {
  case 0:
    break;
  case 1:
    if (assemblyMap.getObjectByAddress(PC).lineNumber == breakIsValue)
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
    if (bit(flags, HalfCarry) == breakIsValue)
      running = false;
    break;
  case 8:
    if (bit(flags, InterruptMask) == breakIsValue)
      running = false;
    break;
  case 9:
    if (bit(flags, Negative) == breakIsValue)
      running = false;
    break;
  case 10:
    if (bit(flags, Zero) == breakIsValue)
      running = false;
    break;
  case 11:
    if (bit(flags, Overflow) == breakIsValue)
      running = false;
    break;
  case 12:
    if (bit(flags, Carry) == breakIsValue)
      running = false;
    break;
  case 13:
    if (Memory[breakAtValue] == breakIsValue)
      running = false;
    break;
  }
  if(bookmarkBreakpointsEnabled)  {
    if(bookmarkedAddresses.contains(PC)){
      running = false;
    }
  }
}
/**
 * @brief Pushes the current processor state onto the memory stack.
 *
 * The state includes the program counter, index register, accumulators, and flags.
 */
void Processor::pushStateToMemory() {
  Memory[SP--] = PC & 0xFF;
  Memory[SP--] = (PC >> 8);
  Memory[SP--] = xReg & 0xFF;
  Memory[SP--] = (xReg >> 8);
  Memory[SP--] = aReg;
  Memory[SP--] = bReg;
  Memory[SP--] = flags;
}
/**
 * @brief Updates a specific flag in the processor's flag register.
 *
 * Sets or clears the given flag based on the provided value.
 *
 * @param flag The flag to update.
 * @param value The boolean value to set for the flag (true for set, false for clear).
 */
void Processor::updateFlag(Flag flag, bool value) {
  flags = (flags & ~(1 << flag)) | (value << flag);
}
/**
 * @brief Retrieves the interrupt vector location for a given interrupt.
 *
 * This inline function computes the interrupt address from memory based on the
 * interrupt type and predefined interrupt locations.
 *
 * @param interrupt The interrupt type for which to get the location.
 * @return The 16-bit interrupt vector address.
 */
inline uint16_t Processor::getInterruptLocation(Interrupt interrupt) {
  if (interrupt != Interrupt::IRQ && interrupt != Interrupt::NMI && interrupt != Interrupt::RST) {
    throw std::invalid_argument("Somehow an invalid interrupt was passed to getInterruptLocation() id: " + std::to_string(static_cast<int>(interrupt)));
  }
  return (Memory[(interruptLocations - static_cast<int>(interrupt) * 2 - 1)] << 8) + Memory[(interruptLocations - static_cast<int>(interrupt) * 2)];
}

/**
 * @brief Prepares and emits UI update data.
 *
 * Copies the entire memory and sends the current processor state (cycle count,
 * flags, registers, etc.) to the UI via the uiUpdateData signal.
 */
void Processor::setUIUpdateData() {
  std::array<uint8_t, 0x10000> memoryCopy;
  std::copy(Memory.begin(), Memory.end(), memoryCopy.begin());
  emit uiUpdateData(memoryCopy, curCycle, flags, PC, SP, aReg, bReg, xReg, useCycles, opertaionsSinceStart);
}

/**
 * @brief Executes the current instruction using the M6800 function pointer table.
 */
void Processor::executeM6800() {
  (this->*M6800Table[Memory[PC]])();
}
/**
 * @brief Executes the current instruction using the M6803 function pointer table.
 */
void Processor::executeM6803() {
  (this->*M6803Table[Memory[PC]])();
}

/**
 * @brief Checks and handles pending interrupts in cycle-per-step (CPS) mode.
 *
 * Depending on the pending interrupt state, this function either executes the
 * instruction normally or handles the interrupt service routine.
 */
void Processor::interruptCheckCPS() {
  switch (pendingInterrupt) {
  case Interrupt::NONE:
    (this->*executeInstruction)();
    if (WAIStatus) {
      cycleCount = 0;
    } else {
      cycleCount = getInstructionCycleCount(processorVersion, Memory[PC]);
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
    if (!bit(flags, InterruptMask)) {
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
        cycleCount = getInstructionCycleCount(processorVersion, Memory[PC]);
      }
      pendingInterrupt = Interrupt::NONE;
    }
    break;
  case Interrupt::RSTCYCLESERVICE:
    updateFlag(Flag::InterruptMask, 1);
    PC = getInterruptLocation(Interrupt::RST);
    cycleCount = getInstructionCycleCount(processorVersion, Memory[PC]);
    pendingInterrupt = Interrupt::NONE;
    WAIStatus = false;
    break;
  case Interrupt::NMICYCLESERVICE:
    if (!WAIStatus) {
      pushStateToMemory();
    } else {
      WAIStatus = false;
    }
    updateFlag(Flag::InterruptMask, 1);
    PC = getInterruptLocation(Interrupt::NMI);
    cycleCount = getInstructionCycleCount(processorVersion, Memory[PC]);
    pendingInterrupt = Interrupt::NONE;
    break;
  case Interrupt::IRQCYCLESERVICE:
    if (!WAIStatus) {
      pushStateToMemory();
    } else {
      WAIStatus = false;
    }
    updateFlag(Flag::InterruptMask, 1);
    PC = getInterruptLocation(Interrupt::IRQ);
    cycleCount = getInstructionCycleCount(processorVersion, Memory[PC]);
    pendingInterrupt = Interrupt::NONE;
    break;
  }
}
/**
 * @brief Checks and handles pending interrupts in instruction-per-step (IPS) mode.
 *
 * This function processes pending interrupts and executes the corresponding
 * interrupt service routines or normal instructions.
 */
void Processor::interruptCheckIPS() {
  switch (pendingInterrupt) {
  case Interrupt::NONE:
    (this->*executeInstruction)();
    break;
  case Interrupt::RST:
    updateFlag(Flag::InterruptMask, 1);
    PC = getInterruptLocation(Interrupt::RST);
    pendingInterrupt = Interrupt::NONE;
    WAIStatus = false;
    break;
  case Interrupt::NMI:
    if (!WAIStatus) {
      pushStateToMemory();
    }
    updateFlag(Flag::InterruptMask, 1);
    PC = getInterruptLocation(Interrupt::NMI);
    pendingInterrupt = Interrupt::NONE;
    WAIStatus = false;
    break;
  case Interrupt::IRQ:
    if (!bit(flags, InterruptMask)) {
      if (!WAIStatus) {
        pushStateToMemory();
      }
      updateFlag(Flag::InterruptMask, 1);
      PC = getInterruptLocation(Interrupt::IRQ);
      WAIStatus = false;
    } else {
      if (WAIStatus == false) {
        (this->*executeInstruction)();
      }
    }
    pendingInterrupt = Interrupt::NONE;
    break;
  case Core::Interrupt::RSTCYCLESERVICE:
  case Core::Interrupt::NMICYCLESERVICE:
  case Core::Interrupt::IRQCYCLESERVICE:
    throw std::invalid_argument("CYCLESERVICE interrupt state passed into interruptCheckIPS.");
    break;
  }
}

/**
 * @brief Executes a single instruction.
 * 
 * This function processes a single step by checking for interrupts and then
 * executing the corresponding instruction.
 */
void Processor::executeStep() {
  interruptCheckIPS();
}
/**
 * @brief Starts the processor execution in a separate thread.
 *
 * This function sets up the execution parameters based on the provided
 * operations per second (OPS) and assembly mapping, then starts a concurrent
 * execution loop that processes instructions and updates the UI.
 *
 * @param OPS The number of operations per second.
 * @param list The assembly mapping used for debugging breakpoints.
 */
void Processor::startExecution(float OPS, AssemblyMap list, QVector<int> bookmarkedAddresses) {
  assemblyMap = list;
  this->bookmarkedAddresses=bookmarkedAddresses;
  running = true;
  curCycle = 1;
  cycleCount = getInstructionCycleCount(processorVersion, Memory[PC]);
  opertaionsSinceStart = 0;


  int nanoDelay = 1000000000 / OPS;
  int uiUpdateSpeed = 250;
  int batchSize;
  if (OPS > uiUpdateSpeed) {
    batchSize = OPS / uiUpdateSpeed;
  } else {
    batchSize = 1;
  }
  startTime = std::chrono::steady_clock::now();
  futureWatcher.setFuture(QtConcurrent::run([this, nanoDelay, batchSize]() {
    auto next = std::chrono::steady_clock::now() + std::chrono::nanoseconds(nanoDelay * batchSize);
    while (running) {
      while (std::chrono::steady_clock::now() < next)
        if (!running) {
          break;
        };

      next = next + std::chrono::nanoseconds(nanoDelay * batchSize);

      handleActions();
      for (int i = 0; i < batchSize; i++) {
        if (!running) {
          break;
        }
        if (useCycles) {
          if (curCycle < cycleCount) {
            curCycle++;
            opertaionsSinceStart++;
            if (i + 1 == batchSize) {
              setUIUpdateData();
            }
          } else {
            interruptCheckCPS();
            checkBreak();
            curCycle = 1;
            opertaionsSinceStart++;
            if (i + 1 == batchSize) {
              setUIUpdateData();
            }
          }
        } else {
          interruptCheckIPS();
          checkBreak();
          opertaionsSinceStart++;
          if (i + 1 == batchSize) {
            setUIUpdateData();
          }
        }
      }
    }
    handleActions();
    emit executionStopped();
  }));
}
/**
 * @brief Stops the processor execution.
 *
 * This function halts the execution loop and waits for the execution thread
 * to finish its current processing.
 */
void Processor::stopExecution() {
  curCycle = 1;
  running = false;
  futureWatcher.waitForFinished();
}
/**
 * @brief Stops execution and resets the processor's internals and registers.
 */
void Processor::reset() {
  stopExecution();

  std::copy(backupMemory.begin(), backupMemory.end(), Memory.begin());

  WAIStatus = false;
  pendingInterrupt = Interrupt::NONE;
  cycleCount = 0;
  curCycle = 1;

  aReg = 0;
  bReg = 0;
  xReg = 0;
  SP = 0x00FF;
  PC = static_cast<uint16_t>(Memory[interruptLocations - 1] << 8) + Memory[interruptLocations];
  flags = 0xD0;
}
