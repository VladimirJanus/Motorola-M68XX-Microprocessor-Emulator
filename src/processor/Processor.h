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
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <QObject>
#include "src/core/Core.h"
#include "src/utils/ActionQueue.h"
#include <cstdint>
#include <qfuturewatcher.h>

using Core::AssemblyMap;
using Core::bit;
using Core::Flag;
using Core::Flag::Carry;
using Core::Flag::HalfCarry;
using Core::Flag::InterruptMask;
using Core::Flag::Negative;
using Core::Flag::Overflow;
using Core::Flag::Zero;
using Core::Interrupt;
using Core::interruptLocations;
using Core::M6800InstructionPage;
using Core::M6803InstructionPage;
using Core::ProcessorVersion;

class Processor : public QObject {
  Q_OBJECT
private:
  typedef void (Processor::*funcPtr)();
  ActionQueue actionQueue;
  AssemblyMap assemblyMap;
  QFutureWatcher<void> futureWatcher;
  funcPtr executeInstruction;
  ProcessorVersion processorVersion = ProcessorVersion::M6800;

  //internal settings
  bool WAIStatus = false;
  bool IRQOnKeyPressed = false;
  bool incrementPCOnMissingInstruction = false;
  int breakWhenIndex = 0;
  int breakIsValue = 0;
  int breakAtValue = 0;

public:
  Processor();
  //processor internals
  std::array<uint8_t, 0x10000> Memory = {};
  std::array<uint8_t, 0x10000> backupMemory = {};
  uint8_t aReg = 0;
  uint8_t bReg = 0;
  uint16_t PC = 0;
  uint16_t SP = 0x00FF;
  uint16_t xReg = 0;
  uint8_t flags = 0;
  uint16_t *curIndReg = &xReg;
  int curCycle = 1;
  int cycleCount = 0;
  Interrupt pendingInterrupt = Interrupt::NONE;

  //runtime settings
  volatile bool running = false;
  bool useCycles = false;

  //run stop step
  void reset();
  void executeStep();
  void startExecution(float OPS, AssemblyMap list);
  void stopExecution();

  //change settings
  void switchVersion(ProcessorVersion version);
  void addAction(const Action &action);

private:
  //helper funcs
  uint16_t getInterruptLocation(Interrupt interrupt);
  void updateFlags(Flag flag, bool value);
  void pushStateToMemory();
  void checkBreak();

  //instructionExecution
  void interruptCheckCPS();
  void interruptCheckIPS();
  void executeM6803();
  void executeM6800();

  void setUIUpdateData();

  //action handling
  void handleAction(const Action &action);
  void handleActions();

signals:
  void uiUpdateData(std::array<uint8_t, 0x10000>, int curCycle, uint8_t flags, uint16_t PC, uint16_t SP, uint8_t aReg, uint8_t bReg, uint16_t xReg, bool useCycles);
  void executionStopped();

private:
  void ZERO();
  void INVALID();
  void INHNOP();
  void INHLSRD();
  void INHASLD();
  void INHTAP();
  void INHTPA();
  void INHINX();
  void INHDEX();
  void INHCLV();
  void INHSEV();
  void INHCLC();
  void INHSEC();
  void INHCLI();
  void INHSEI();
  void INHSBA();
  void INHCBA();
  void INHTAB();
  void INHTBA();
  void INHDAA();
  void INHABA();
  void RELBRA();
  void RELBRN();
  void RELBHI();
  void RELBLS();
  void RELBCC();
  void RELBCS();
  void RELBNE();
  void RELBEQ();
  void RELBVC();
  void RELBVS();
  void RELBPL();
  void RELBMI();
  void RELBGE();
  void RELBLT();
  void RELBGT();
  void RELBLE();
  void INHTSX();
  void INHINS();
  void INHPULA();
  void INHPULB();
  void INHDES();
  void INHTCS();
  void INHPSHA();
  void INHPSHB();
  void INHPULX();
  void INHRTS();
  void INHABX();
  void INHRTI();
  void INHPSHX();
  void INHMUL();
  void INHWAI();
  void INHSWI();
  void INHNEGA();
  void INHCOMA();
  void INHLSRA();
  void INHRORA();
  void INHASRA();
  void INHASLA();
  void INHROLA();
  void INHDECA();
  void INHINCA();
  void INHTSTA();
  void INHCLRA();
  void INHNEGB();
  void INHCOMB();
  void INHLSRB();
  void INHRORB();
  void INHASRB();
  void INHASLB();
  void INHROLB();
  void INHDECB();
  void INHINCB();
  void INHTSTB();
  void INHCLRB();
  void INDNEG();
  void INDCOM();
  void INDLSR();
  void INDROR();
  void INDASR();
  void INDASL();
  void INDROL();
  void INDDEC();
  void INDINC();
  void INDTST();
  void INDJMP();
  void INDCLR();
  void EXTNEG();
  void EXTCOM();
  void EXTLSR();
  void EXTROR();
  void EXTASR();
  void EXTASL();
  void EXTROL();
  void EXTDEC();
  void EXTINC();
  void EXTTST();
  void EXTJMP();
  void EXTCLR();
  void IMMSUBA();
  void IMMCMPA();
  void IMMSBCA();
  void IMMSUBD();
  void IMMANDA();
  void IMMBITA();
  void IMMLDAA();
  void IMMEORA();
  void IMMADCA();
  void IMMORAA();
  void IMMADDA();
  void IMMCPX();
  void RELBSR();
  void IMMLDS();
  void DIRSUBA();
  void DIRCMPA();
  void DIRSBCA();
  void DIRSUBD();
  void DIRANDA();
  void DIRBITA();
  void DIRLDAA();
  void DIRSTAA();
  void DIREORA();
  void DIRADCA();
  void DIRORAA();
  void DIRADDA();
  void DIRCPX();
  void DIRJSR();
  void DIRLDS();
  void DIRSTS();
  void INDSUBA();
  void INDCMPA();
  void INDSBCA();
  void INDSUBD();
  void INDANDA();
  void INDBITA();
  void INDLDAA();
  void INDSTAA();
  void INDEORA();
  void INDADCA();
  void INDORAA();
  void INDADDA();
  void INDCPX();
  void INDJSR();
  void INDLDS();
  void INDSTS();
  void EXTSUBA();
  void EXTCMPA();
  void EXTSBCA();
  void EXTSUBD();
  void EXTANDA();
  void EXTBITA();
  void EXTLDAA();
  void EXTSTAA();
  void EXTEORA();
  void EXTADCA();
  void EXTORAA();
  void EXTADDA();
  void EXTCPX();
  void EXTJSR();
  void EXTLDS();
  void EXTSTS();
  void IMMSUBB();
  void IMMCMPB();
  void IMMSBCB();
  void IMMADDD();
  void IMMANDB();
  void IMMBITB();
  void IMMLDAB();
  void IMMEORB();
  void IMMADCB();
  void IMMORAB();
  void IMMADDB();
  void IMMLDD();
  void IMMLDX();
  void DIRSUBB();
  void DIRCMPB();
  void DIRSBCB();
  void DIRADDD();
  void DIRANDB();
  void DIRBITB();
  void DIRLDAB();
  void DIRSTAB();
  void DIREORB();
  void DIRADCB();
  void DIRORAB();
  void DIRADDB();
  void DIRLDD();
  void DIRSTD();
  void DIRLDX();
  void DIRSTX();
  void INDSUBB();
  void INDCMPB();
  void INDSBCB();
  void INDADDD();
  void INDANDB();
  void INDBITB();
  void INDLDAB();
  void INDSTAB();
  void INDEORB();
  void INDADCB();
  void INDORAB();
  void INDADDB();
  void INDLDD();
  void INDSTD();
  void INDLDX();
  void INDSTX();
  void EXTSUBB();
  void EXTCMPB();
  void EXTSBCB();
  void EXTADDD();
  void EXTANDB();
  void EXTBITB();
  void EXTLDAB();
  void EXTSTAB();
  void EXTEORB();
  void EXTADCB();
  void EXTORAB();
  void EXTADDB();
  void EXTLDD();
  void EXTSTD();
  void EXTLDX();
  void EXTSTX();

  funcPtr M6800Table[256] = {
    &Processor::ZERO,    &Processor::INHNOP,  &Processor::INVALID, &Processor::INVALID, &Processor::INVALID, &Processor::INVALID, &Processor::INHTAP,  &Processor::INHTPA,
    &Processor::INHINX,  &Processor::INHDEX,  &Processor::INHCLV,  &Processor::INHSEV,  &Processor::INHCLC,  &Processor::INHSEC,  &Processor::INHCLI,  &Processor::INHSEI,
    &Processor::INHSBA,  &Processor::INHCBA,  &Processor::INVALID, &Processor::INVALID, &Processor::INVALID, &Processor::INVALID, &Processor::INHTAB,  &Processor::INHTBA,
    &Processor::INVALID, &Processor::INHDAA,  &Processor::INVALID, &Processor::INHABA,  &Processor::INVALID, &Processor::INVALID, &Processor::INVALID, &Processor::INVALID,
    &Processor::RELBRA,  &Processor::INVALID, &Processor::RELBHI,  &Processor::RELBPL,  &Processor::RELBCC,  &Processor::RELBCS,  &Processor::RELBNE,  &Processor::RELBEQ,
    &Processor::RELBVC,  &Processor::RELBVS,  &Processor::RELBPL,  &Processor::RELBMI,  &Processor::RELBGE,  &Processor::RELBLT,  &Processor::RELBGT,  &Processor::RELBLE,
    &Processor::INHTSX,  &Processor::INHINS,  &Processor::INHPULA, &Processor::INHPULB, &Processor::INHDES,  &Processor::INHTCS,  &Processor::INHPSHA, &Processor::INHPSHB,
    &Processor::INVALID, &Processor::INHRTS,  &Processor::INVALID, &Processor::INHRTI,  &Processor::INVALID, &Processor::INVALID, &Processor::INHWAI,  &Processor::INHSWI,

    &Processor::INHNEGA, &Processor::INVALID, &Processor::INVALID, &Processor::INHCOMA, &Processor::INHLSRA, &Processor::INVALID, &Processor::INHRORA, &Processor::INHASRA,
    &Processor::INHASLA, &Processor::INHROLA, &Processor::INHDECA, &Processor::INVALID, &Processor::INHINCA, &Processor::INHTSTA, &Processor::INVALID, &Processor::INHCLRA,
    &Processor::INHNEGB, &Processor::INVALID, &Processor::INVALID, &Processor::INHCOMB, &Processor::INHLSRB, &Processor::INVALID, &Processor::INHRORB, &Processor::INHASRB,
    &Processor::INHASLB, &Processor::INHROLB, &Processor::INHDECB, &Processor::INVALID, &Processor::INHINCB, &Processor::INHTSTB, &Processor::INVALID, &Processor::INHCLRB,
    &Processor::INDNEG,  &Processor::INVALID, &Processor::INVALID, &Processor::INDCOM,  &Processor::INDLSR,  &Processor::INVALID, &Processor::INDROR,  &Processor::INDASR,
    &Processor::INDASL,  &Processor::INDROL,  &Processor::INDDEC,  &Processor::INVALID, &Processor::INDINC,  &Processor::INDTST,  &Processor::INDJMP,  &Processor::INDCLR,
    &Processor::EXTNEG,  &Processor::INVALID, &Processor::INVALID, &Processor::EXTCOM,  &Processor::EXTLSR,  &Processor::INVALID, &Processor::EXTROR,  &Processor::EXTASR,
    &Processor::EXTASL,  &Processor::EXTROL,  &Processor::EXTDEC,  &Processor::INVALID, &Processor::EXTINC,  &Processor::EXTTST,  &Processor::EXTJMP,  &Processor::EXTCLR,

    &Processor::IMMSUBA, &Processor::IMMCMPA, &Processor::IMMSBCA, &Processor::INVALID, &Processor::IMMANDA, &Processor::IMMBITA, &Processor::IMMLDAA, &Processor::INVALID,
    &Processor::IMMEORA, &Processor::IMMADCA, &Processor::IMMORAA, &Processor::IMMADDA, &Processor::IMMCPX,  &Processor::RELBSR,  &Processor::IMMLDS,  &Processor::INVALID,
    &Processor::DIRSUBA, &Processor::DIRCMPA, &Processor::DIRSBCA, &Processor::INVALID, &Processor::DIRANDA, &Processor::DIRBITA, &Processor::DIRLDAA, &Processor::DIRSTAA,
    &Processor::DIREORA, &Processor::DIRADCA, &Processor::DIRORAA, &Processor::DIRADDA, &Processor::DIRCPX,  &Processor::INVALID, &Processor::DIRLDS,  &Processor::DIRSTS,
    &Processor::INDSUBA, &Processor::INDCMPA, &Processor::INDSBCA, &Processor::INVALID, &Processor::INDANDA, &Processor::INDBITA, &Processor::INDLDAA, &Processor::INDSTAA,
    &Processor::INDEORA, &Processor::INDADCA, &Processor::INDORAA, &Processor::INDADDA, &Processor::INDCPX,  &Processor::INDJSR,  &Processor::INDLDS,  &Processor::INDSTS,
    &Processor::EXTSUBA, &Processor::EXTCMPA, &Processor::EXTSBCA, &Processor::INVALID, &Processor::EXTANDA, &Processor::EXTBITA, &Processor::EXTLDAA, &Processor::EXTSTAA,
    &Processor::EXTEORA, &Processor::EXTADCA, &Processor::EXTORAA, &Processor::EXTADDA, &Processor::EXTCPX,  &Processor::EXTJSR,  &Processor::EXTLDS,  &Processor::EXTSTS,

    &Processor::IMMSUBB, &Processor::IMMCMPB, &Processor::IMMSBCB, &Processor::INVALID, &Processor::IMMANDB, &Processor::IMMBITB, &Processor::IMMLDAB, &Processor::INVALID,
    &Processor::IMMEORB, &Processor::IMMADCB, &Processor::IMMORAB, &Processor::IMMADDB, &Processor::INVALID, &Processor::INVALID, &Processor::IMMLDX,  &Processor::INVALID,
    &Processor::DIRSUBB, &Processor::DIRCMPB, &Processor::DIRSBCB, &Processor::INVALID, &Processor::DIRANDB, &Processor::DIRBITB, &Processor::DIRLDAB, &Processor::DIRSTAB,
    &Processor::DIREORB, &Processor::DIRADCB, &Processor::DIRORAB, &Processor::DIRADDB, &Processor::INVALID, &Processor::INVALID, &Processor::DIRLDX,  &Processor::DIRSTX,
    &Processor::INDSUBB, &Processor::INDCMPB, &Processor::INDSBCB, &Processor::INVALID, &Processor::INDANDB, &Processor::INDBITB, &Processor::INDLDAB, &Processor::INDSTAB,
    &Processor::INDEORB, &Processor::INDADCB, &Processor::INDORAB, &Processor::INDADDB, &Processor::INVALID, &Processor::INVALID, &Processor::INDLDX,  &Processor::INDSTX,
    &Processor::EXTSUBB, &Processor::EXTCMPB, &Processor::EXTSBCB, &Processor::INVALID, &Processor::EXTANDB, &Processor::EXTBITB, &Processor::EXTLDAB, &Processor::EXTSTAB,
    &Processor::EXTEORB, &Processor::EXTADCB, &Processor::EXTORAB, &Processor::EXTADDB, &Processor::INVALID, &Processor::INVALID, &Processor::EXTLDX,  &Processor::EXTSTX,
  };

  funcPtr M6803Table[256] = {
    &Processor::ZERO,    &Processor::INHNOP,  &Processor::INVALID, &Processor::INVALID, &Processor::INHLSRD, &Processor::INHASLD, &Processor::INHTAP,  &Processor::INHTPA,
    &Processor::INHINX,  &Processor::INHDEX,  &Processor::INHCLV,  &Processor::INHSEV,  &Processor::INHCLC,  &Processor::INHSEC,  &Processor::INHCLI,  &Processor::INHSEI,
    &Processor::INHSBA,  &Processor::INHCBA,  &Processor::INVALID, &Processor::INVALID, &Processor::INVALID, &Processor::INVALID, &Processor::INHTAB,  &Processor::INHTBA,
    &Processor::INVALID, &Processor::INHDAA,  &Processor::INVALID, &Processor::INHABA,  &Processor::INVALID, &Processor::INVALID, &Processor::INVALID, &Processor::INVALID,
    &Processor::RELBRA,  &Processor::RELBRN,  &Processor::RELBHI,  &Processor::RELBPL,  &Processor::RELBCC,  &Processor::RELBCS,  &Processor::RELBNE,  &Processor::RELBEQ,
    &Processor::RELBVC,  &Processor::RELBVS,  &Processor::RELBPL,  &Processor::RELBMI,  &Processor::RELBGE,  &Processor::RELBLT,  &Processor::RELBGT,  &Processor::RELBLE,
    &Processor::INHTSX,  &Processor::INHINS,  &Processor::INHPULA, &Processor::INHPULB, &Processor::INHDES,  &Processor::INHTCS,  &Processor::INHPSHA, &Processor::INHPSHB,
    &Processor::INHPULX, &Processor::INHRTS,  &Processor::INHABX,  &Processor::INHRTI,  &Processor::INHPSHX, &Processor::INHMUL,  &Processor::INHWAI,  &Processor::INHSWI,

    &Processor::INHNEGA, &Processor::INVALID, &Processor::INVALID, &Processor::INHCOMA, &Processor::INHLSRA, &Processor::INVALID, &Processor::INHRORA, &Processor::INHASRA,
    &Processor::INHASLA, &Processor::INHROLA, &Processor::INHDECA, &Processor::INVALID, &Processor::INHINCA, &Processor::INHTSTA, &Processor::INVALID, &Processor::INHCLRA,
    &Processor::INHNEGB, &Processor::INVALID, &Processor::INVALID, &Processor::INHCOMB, &Processor::INHLSRB, &Processor::INVALID, &Processor::INHRORB, &Processor::INHASRB,
    &Processor::INHASLB, &Processor::INHROLB, &Processor::INHDECB, &Processor::INVALID, &Processor::INHINCB, &Processor::INHTSTB, &Processor::INVALID, &Processor::INHCLRB,
    &Processor::INDNEG,  &Processor::INVALID, &Processor::INVALID, &Processor::INDCOM,  &Processor::INDLSR,  &Processor::INVALID, &Processor::INDROR,  &Processor::INDASR,
    &Processor::INDASL,  &Processor::INDROL,  &Processor::INDDEC,  &Processor::INVALID, &Processor::INDINC,  &Processor::INDTST,  &Processor::INDJMP,  &Processor::INDCLR,
    &Processor::EXTNEG,  &Processor::INVALID, &Processor::INVALID, &Processor::EXTCOM,  &Processor::EXTLSR,  &Processor::INVALID, &Processor::EXTROR,  &Processor::EXTASR,
    &Processor::EXTASL,  &Processor::EXTROL,  &Processor::EXTDEC,  &Processor::INVALID, &Processor::EXTINC,  &Processor::EXTTST,  &Processor::EXTJMP,  &Processor::EXTCLR,

    &Processor::IMMSUBA, &Processor::IMMCMPA, &Processor::IMMSBCA, &Processor::IMMSUBD, &Processor::IMMANDA, &Processor::IMMBITA, &Processor::IMMLDAA, &Processor::INVALID,
    &Processor::IMMEORA, &Processor::IMMADCA, &Processor::IMMORAA, &Processor::IMMADDA, &Processor::IMMCPX,  &Processor::RELBSR,  &Processor::IMMLDS,  &Processor::INVALID,
    &Processor::DIRSUBA, &Processor::DIRCMPA, &Processor::DIRSBCA, &Processor::DIRSUBD, &Processor::DIRANDA, &Processor::DIRBITA, &Processor::DIRLDAA, &Processor::DIRSTAA,
    &Processor::DIREORA, &Processor::DIRADCA, &Processor::DIRORAA, &Processor::DIRADDA, &Processor::DIRCPX,  &Processor::DIRJSR,  &Processor::DIRLDS,  &Processor::DIRSTS,
    &Processor::INDSUBA, &Processor::INDCMPA, &Processor::INDSBCA, &Processor::INDSUBD, &Processor::INDANDA, &Processor::INDBITA, &Processor::INDLDAA, &Processor::INDSTAA,
    &Processor::INDEORA, &Processor::INDADCA, &Processor::INDORAA, &Processor::INDADDA, &Processor::INDCPX,  &Processor::INDJSR,  &Processor::INDLDS,  &Processor::INDSTS,
    &Processor::EXTSUBA, &Processor::EXTCMPA, &Processor::EXTSBCA, &Processor::EXTSUBD, &Processor::EXTANDA, &Processor::EXTBITA, &Processor::EXTLDAA, &Processor::EXTSTAA,
    &Processor::EXTEORA, &Processor::EXTADCA, &Processor::EXTORAA, &Processor::EXTADDA, &Processor::EXTCPX,  &Processor::EXTJSR,  &Processor::EXTLDS,  &Processor::EXTSTS,

    &Processor::IMMSUBB, &Processor::IMMCMPB, &Processor::IMMSBCB, &Processor::IMMADDD, &Processor::IMMANDB, &Processor::IMMBITB, &Processor::IMMLDAB, &Processor::INVALID,
    &Processor::IMMEORB, &Processor::IMMADCB, &Processor::IMMORAB, &Processor::IMMADDB, &Processor::IMMLDD,  &Processor::INVALID, &Processor::IMMLDX,  &Processor::INVALID,
    &Processor::DIRSUBB, &Processor::DIRCMPB, &Processor::DIRSBCB, &Processor::DIRADDD, &Processor::DIRANDB, &Processor::DIRBITB, &Processor::DIRLDAB, &Processor::DIRSTAB,
    &Processor::DIREORB, &Processor::DIRADCB, &Processor::DIRORAB, &Processor::DIRADDB, &Processor::DIRLDD,  &Processor::DIRSTD,  &Processor::DIRLDX,  &Processor::DIRSTX,
    &Processor::INDSUBB, &Processor::INDCMPB, &Processor::INDSBCB, &Processor::INDADDD, &Processor::INDANDB, &Processor::INDBITB, &Processor::INDLDAB, &Processor::INDSTAB,
    &Processor::INDEORB, &Processor::INDADCB, &Processor::INDORAB, &Processor::INDADDB, &Processor::INDLDD,  &Processor::INDSTD,  &Processor::INDLDX,  &Processor::INDSTX,
    &Processor::EXTSUBB, &Processor::EXTCMPB, &Processor::EXTSBCB, &Processor::EXTADDD, &Processor::EXTANDB, &Processor::EXTBITB, &Processor::EXTLDAB, &Processor::EXTSTAB,
    &Processor::EXTEORB, &Processor::EXTADCB, &Processor::EXTORAB, &Processor::EXTADDB, &Processor::EXTLDD,  &Processor::EXTSTD,  &Processor::EXTLDX,  &Processor::EXTSTX,
  };
};

#endif // PROCESSOR_H
