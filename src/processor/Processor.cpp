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
 * @brief Configures the processor to operate as the specified version.
 *
 * Sets the internal processor version field and assigns the appropriate
 * instruction execution function pointer based on the target architecture.
 *
 * @param version The target processor architecture (M6800 or M6803).
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
 * @brief Enqueues an action for deferred processing by the processor.
 *
 * Adds the specified action to the internal action queue. If the processor
 * is idle (not running), all pending actions are processed immediately.
 *
 * @param action The action to enqueue.
 */
void Processor::addAction(const Action &action) {
  actionQueue.addAction(action);
  if (!running)
    handleActions();
}

/**
 * @brief Processes all pending actions from the action queue.
 *
 * Iteratively retrieves and handles each action in the queue until
 * no actions remain.
 */
void Processor::handleActions() {
  while (actionQueue.hasActions()) {
    Action action = actionQueue.getNextAction();
    handleAction(action);
  }
}

QVector<int> newBookmarkedAddresses;

/**
 * @brief Stages bookmark data for the next update cycle.
 *
 * Stores the provided bookmark addresses temporarily. The data is applied
 * to the active bookmarked addresses when an UPDATEBOOKMARKS action is processed.
 *
 * @param data Vector of memory addresses to be bookmarked.
 */
void Processor::queueBookmarkData(QVector<int> data){
    newBookmarkedAddresses = data;
}

/**
 * @brief Dispatches an action to its corresponding handler based on action type.
 *
 * Examines the action type and modifies the processor state accordingly.
 * Handles breakpoint configuration, interrupt requests, memory updates,
 * input events, and various processor settings.
 *
 * @param action The action to process.
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
  case ActionType::SETINCONINVALIDINSTR:
    incrementPCOnMissingInstruction = action.parameter;
    break;
  }
}

/**
 * @brief Evaluates configured breakpoint conditions and halts execution if met.
 *
 * Tests the current processor state against the configured breakpoint criteria
 * (determined by breakWhenIndex, breakAtValue, and breakIsValue). Supported
 * conditions include source line number, program counter, stack pointer,
 * register values, processor flags, and memory contents. Additionally checks
 * bookmark-based breakpoints if enabled.
 *
 * If any condition is satisfied, execution is halted by setting running to false.
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
 * @brief Saves the complete processor context to the memory stack.
 *
 * Pushes the program counter (PC), index register (X), accumulator A,
 * accumulator B, and condition code register (flags) onto the stack
 * in descending order. The stack pointer is decremented after each push.
 * This operation is typically performed during interrupt servicing.
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
 * @brief Modifies a specific bit in the processor's condition code register.
 *
 * Sets or clears the specified flag bit while preserving all other flags.
 *
 * @param flag The flag bit to modify (e.g., Carry, Zero, Negative).
 * @param value True to set the flag, false to clear it.
 */
void Processor::updateFlag(Flag flag, bool value) {
  flags = (flags & ~(1 << flag)) | (value << flag);
}

/**
 * @brief Retrieves the interrupt vector address for the specified interrupt type.
 *
 * Reads the 16-bit interrupt vector from the appropriate location in memory
 * based on the interrupt type. The vector address is constructed from two
 * consecutive bytes in big-endian format.
 *
 * @param interrupt The interrupt type (RST, NMI, or IRQ).
 * @return The 16-bit address of the interrupt service routine.
 * @throws std::invalid_argument If an invalid interrupt type is provided.
 */
inline uint16_t Processor::getInterruptLocation(Interrupt interrupt) {
  if (interrupt != Interrupt::IRQ && interrupt != Interrupt::NMI && interrupt != Interrupt::RST) {
    throw std::invalid_argument("Somehow an invalid interrupt was passed to getInterruptLocation() id: " + std::to_string(static_cast<int>(interrupt)));
  }
  int address = interruptLocations - static_cast<int>(interrupt) * 2;
  return (Memory[address - 1] << 8) + Memory[address];
}

/**
 * @brief Captures the current processor state and emits it to the UI layer.
 *
 * Creates a complete copy of the processor's memory and packages it with
 * register values, flags, cycle counters, and operation count. This data
 * is transmitted via the uiUpdateData signal for display purposes.
 */
void Processor::setUIUpdateData() {
  std::array<uint8_t, 0x10000> memoryCopy;
  std::copy(Memory.begin(), Memory.end(), memoryCopy.begin());
  emit uiUpdateData(memoryCopy, curCycle, flags, PC, SP, aReg, bReg, xReg, useCycles, operationsSinceStart);
}

/**
 * @brief Executes the current instruction using the M6800 instruction set.
 *
 * Invokes the appropriate instruction handler from the M6800 function pointer
 * table based on the opcode at the current program counter.
 */
void Processor::executeM6800() {
  (this->*M6800Table[Memory[PC]])();
}

/**
 * @brief Executes the current instruction using the M6803 instruction set.
 *
 * Invokes the appropriate instruction handler from the M6803 function pointer
 * table based on the opcode at the current program counter.
 */
void Processor::executeM6803() {
  (this->*M6803Table[Memory[PC]])();
}

/**
 * @brief Manages interrupt processing in cycle-accurate cycle-per-step(CPS) execution mode.
 *
 * Evaluates the pending interrupt state and either executes the next instruction
 * or services the pending interrupt. Handles interrupt latency by transitioning
 * through intermediate servicing states (e.g., RSTCYCLESERVICE, NMICYCLESERVICE,
 * IRQCYCLESERVICE) to accurately simulate cycle timing.
 *
 * Special handling is provided for WAI (Wait for Interrupt) status, which
 * modifies cycle counts and stack operations. The InterruptMask flag determines
 * whether IRQ interrupts are honored.
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
 * @brief Manages interrupt processing in instruction-per-step execution mode.
 *
 * Evaluates pending interrupts and immediately services them or executes the
 * next instruction. Unlike cycle-accurate mode, this function completes interrupt
 * servicing in a single step without intermediate states.
 *
 * RST interrupts are always serviced immediately. NMI interrupts are non-maskable
 * and service immediately. IRQ interrupts respect the InterruptMask flag and are
 * only serviced when interrupts are enabled.
 *
 * @throws std::invalid_argument If a CYCLESERVICE interrupt state is encountered,
 *         which should only exist in cycle-accurate mode.
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
 * @brief Executes a single instruction step in non-cycle-accurate mode.
 *
 * Processes pending interrupts and executes one instruction using the
 * instruction-per-step interrupt handling logic. Used for step-by-step
 * debugging or simplified timing simulation.
 */
void Processor::executeStep() {
  interruptCheckIPS();
}

/**
 * @brief Initiates asynchronous processor execution in a dedicated thread.
 *
 * Configures execution parameters based on the specified operations per second,
 * initializes timing and cycle counters, and launches a concurrent execution loop.
 * The loop processes instructions in batches to balance execution speed with UI
 * responsiveness, updating the interface at approximately 250 Hz regardless of
 * the execution rate.
 *
 * Execution continues until halted by a breakpoint condition or external stop request.
 * Actions are processed both at loop entry and between instruction batches.
 *
 * @param OPS Target operations per second (determines execution speed).
 * @param list Assembly source mapping for line-number-based breakpoints.
 * @param bookmarkedAddresses Memory addresses configured as breakpoints.
 */
void Processor::startExecution(float OPS, AssemblyMap list, QVector<int> bookmarkedAddressesVector) {
  assemblyMap = list;
  this->bookmarkedAddresses = bookmarkedAddressesVector;
  running = true;
  curCycle = 1;
  cycleCount = getInstructionCycleCount(processorVersion, Memory[PC]);
  operationsSinceStart = 0;


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
            operationsSinceStart++;
            if (i + 1 == batchSize) {
              setUIUpdateData();
            }
          } else {
            interruptCheckCPS();
            checkBreak();
            curCycle = 1;
            operationsSinceStart++;
            if (i + 1 == batchSize) {
              setUIUpdateData();
            }
          }
        } else {
          interruptCheckIPS();
          checkBreak();
          operationsSinceStart++;
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
 * @brief Halts processor execution and waits for thread termination.
 *
 * Sets the running flag to false, signaling the execution loop to terminate,
 * then blocks until the execution thread completes its current batch and exits.
 * Resets the cycle counter to its initial state.
 */
void Processor::stopExecution() {
  curCycle = 1;
  running = false;
  futureWatcher.waitForFinished();
}

/**
 * @brief Performs a complete processor reset to initial power-on state.
 *
 * Halts any ongoing execution, restores memory from the backup copy, clears
 * all interrupt states, and reinitializes all registers to their default values.
 * The program counter is set to the reset vector address read from memory,
 * and the condition code register is set to 0xD0 (interrupt mask and reserved
 * bits set).
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
