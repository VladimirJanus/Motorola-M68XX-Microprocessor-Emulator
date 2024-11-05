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

inline bool bit(int variable, int bitNum) {
  return (variable & (1 << bitNum)) != 0;
}

void Processor::ZERO() {
  if (running) {
    running = false;
  } else {
    PC++;
  }
}
void Processor::INVALID() {
  if (incrementPCOnMissingInstruction) {
    PC++;
  } else if (running) {
    running = false;
  }
}

void Processor::INHNOP() {
  PC++;
}
void Processor::INHLSRD() {
  uint8_t uInt8 = bReg & 0x01;
  uint16_t uInt16 = (aReg << 8) + bReg;
  uInt16 = (uInt16 >> 1);
  aReg = (uInt16 >> 8);
  bReg = (uInt16 & 0xFF);
  updateFlags(Negative, 0);
  updateFlags(Zero, uInt16 == 0);
  updateFlags(Overflow, uInt8);
  updateFlags(Carry, uInt8);
  PC++;
}
void Processor::INHASLD() {
  uint8_t uInt8 = (bit(aReg, 7));
  uint16_t uInt16 = (aReg << 8) + bReg;
  uInt16 = uInt16 << 1;
  aReg = (uInt16 >> 8);
  bReg = (uInt16 & 0xFF);
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, uInt16 == 0);
  updateFlags(Overflow, uInt8 ^ bit(aReg, 7));
  updateFlags(Carry, uInt8);
  PC++;
}
void Processor::INHTAP() {
  updateFlags(HalfCarry, bit(aReg, 5));
  updateFlags(InterruptMask, bit(aReg, 4));
  updateFlags(Negative, bit(aReg, 3));
  updateFlags(Zero, bit(aReg, 2));
  updateFlags(Overflow, bit(aReg, 1));
  updateFlags(Carry, bit(aReg, 0));
  PC++;
}
void Processor::INHTPA() {
  aReg = flags;
  PC++;
}
void Processor::INHINX() {
  (*curIndReg)++;
  updateFlags(Zero, (*curIndReg) == 0);
  PC++;
}
void Processor::INHDEX() {
  (*curIndReg)--;
  updateFlags(Zero, (*curIndReg) == 0);
  PC++;
}
void Processor::INHCLV() {
  updateFlags(Overflow, 0);
  PC++;
}
void Processor::INHSEV() {
  updateFlags(Overflow, 1);
  PC++;
}
void Processor::INHCLC() {
  updateFlags(Carry, 0);
  PC++;
}
void Processor::INHSEC() {
  updateFlags(Carry, 1);
  PC++;
}
void Processor::INHCLI() {
  updateFlags(InterruptMask, 0);
  PC++;
}
void Processor::INHSEI() {
  updateFlags(InterruptMask, 1);
  PC++;
}
void Processor::INHSBA() {
  uint8_t uInt8 = aReg - bReg;
  updateFlags(Negative, bit(uInt8, 7));
  updateFlags(Zero, uInt8 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(bReg, 7) && !bit(uInt8, 7)) || (!bit(aReg, 7) && bit(bReg, 7) && bit(uInt8, 7)));
  updateFlags(Carry, ((!bit(aReg, 7) && bit(bReg, 7)) || (bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && !bit(aReg, 7))));
  aReg = uInt8;
  PC++;
}
void Processor::INHCBA() {
  uint8_t uInt8 = aReg - bReg;
  updateFlags(Negative, bit(uInt8, 7));
  updateFlags(Zero, uInt8 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(bReg, 7) && !bit(uInt8, 7)) || (!bit(aReg, 7) && bit(bReg, 7) && bit(uInt8, 7)));
  updateFlags(Carry, ((!bit(aReg, 7) && bit(bReg, 7)) || (bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && !bit(aReg, 7))));
  PC++;
}
void Processor::INHTAB() {
  bReg = aReg;
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);
  PC++;
}
void Processor::INHTBA() {
  aReg = bReg;
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);
  PC++;
}
void Processor::INHDAA() {
  uint8_t uInt8 = flags & 0x01;
  uint8_t uInt82 = bit(flags, 5);
  uint16_t uInt16 = aReg >> 4;
  uint16_t uInt162 = aReg & 0xF;
  if (uInt8 != 1 && uInt16 <= 9 && uInt82 != 1 && uInt162 <= 9) {
    aReg += 0x0;
    updateFlags(Carry, 0);
  } else if (uInt8 != 1 && uInt16 <= 8 && uInt82 != 1 && uInt162 >= 0xA && uInt162 <= 0xF) {
    aReg += 0x6;
    updateFlags(Carry, 0);
  } else if (uInt8 != 1 && uInt16 <= 9 && uInt82 && uInt162 <= 3) {
    aReg += 0x6;
    updateFlags(Carry, 0);
  } else if (uInt8 != 1 && uInt16 >= 0xA && uInt16 <= 0xF && uInt82 != 1 && uInt162 <= 9) {
    aReg += 0x60;
    updateFlags(Carry, 1);
  } else if (uInt8 != 1 && uInt16 >= 9 && uInt16 <= 0xF && uInt82 != 1 && uInt162 >= 0xA && uInt162 <= 0xF) {
    aReg += 0x66;
    updateFlags(Carry, 1);
  } else if (uInt8 != 1 && uInt16 >= 0xA && uInt16 <= 0xF && uInt82 && uInt162 <= 3) {
    aReg += 0x66;
    updateFlags(Carry, 1);
  } else if (uInt8 && uInt16 <= 2 && uInt82 != 1 && uInt162 <= 9) {
    aReg += 0x60;
    updateFlags(Carry, 1);
  } else if (uInt8 && uInt16 <= 2 && uInt82 != 1 && uInt162 >= 0xA && uInt162 <= 0xF) {
    aReg += 0x66;
    updateFlags(Carry, 1);
  } else if (uInt8 && uInt16 <= 3 && uInt82 && uInt162 <= 3) {
    aReg += 0x66;
    updateFlags(Carry, 1);
  }

  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  PC++;
}
void Processor::INHABA() {
  uint16_t uInt16 = aReg + bReg;
  updateFlags(Carry, (bit(aReg, 7) && bit(bReg, 7)) || (!bit(uInt16, 7) && bit(aReg, 7)) || (bit(bReg, 7) && !bit(uInt16, 7)));
  updateFlags(HalfCarry, (bit(aReg, 3) && bit(bReg, 3)) || (bit(aReg, 3) && !bit(uInt16, 3)) || (bit(bReg, 3) && !bit(uInt16, 3)));
  updateFlags(Overflow, (bit(aReg, 7) && bit(bReg, 7) && !bit(uInt16, 7)) || (!bit(aReg, 7) && !bit(bReg, 7) && bit(uInt16, 7)));
  aReg = static_cast<uint8_t>(uInt16);
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  PC++;
}

void Processor::RELBRA() {
  int8_t sInt8 = Memory[(PC + 1) % 0x10000];
  PC += sInt8 + 2;
}

void Processor::RELBRN() {
  PC += 2;
}
void Processor::RELBHI() {
  if ((bit(flags, 2) || (flags & 0x01)) == 0) {
    int8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::RELBLS() {
  if (bit(flags, 2) || (flags & 0x01)) {
    uint8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::RELBCC() {
  if ((flags & 0x01) == 0) {
    int8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::RELBCS() {
  if ((flags & 0x01)) {
    int8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::RELBNE() {
  if (bit(flags, 2) == 0) {
    int8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::RELBEQ() {
  if (bit(flags, 2)) {
    int8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::RELBVC() {
  if (bit(flags, 1) == 0) {
    int8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::RELBVS() {
  if (bit(flags, 1)) {
    int8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::RELBPL() {
  if (bit(flags, 3) == 0) {
    int8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::RELBMI() {
  if (bit(flags, 3)) {
    int8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::RELBGE() {
  if ((bit(flags, 3) ^ bit(flags, 1)) == 0) {
    int8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::RELBLT() {
  if (bit(flags, 3) ^ bit(flags, 1)) {
    int8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::RELBGT() {
  if ((bit(flags, 2) || (bit(flags, 3) ^ bit(flags, 1))) == 0) {
    int8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::RELBLE() {
  if (bit(flags, 2) || (bit(flags, 3) ^ bit(flags, 1))) {
    int8_t sInt8 = Memory[(PC + 1) % 0x10000];
    PC += sInt8;
  }
  PC += 2;
}
void Processor::INHTSX() {
  (*curIndReg) = SP + 1;
  PC++;
}
void Processor::INHINS() {
  SP++;
  PC++;
}
void Processor::INHPULA() {
  aReg = Memory[++SP];
  PC++;
}
void Processor::INHPULB() {
  bReg = Memory[++SP];
  PC++;
}
void Processor::INHDES() {
  SP--;
  PC++;
}
void Processor::INHTCS() {
  SP = (*curIndReg) - 1;
  PC++;
}
void Processor::INHPSHA() {
  Memory[SP--] = aReg;
  PC++;
}
void Processor::INHPSHB() {
  Memory[SP--] = bReg;
  PC++;
}
void Processor::INHPULX() {
  (*curIndReg) = (Memory[++SP] << 8);
  (*curIndReg) += Memory[++SP];
  PC++;
}
void Processor::INHRTS() {
  PC = Memory[++SP] << 8;
  PC += Memory[++SP];
}
void Processor::INHABX() {
  (*curIndReg) = (*curIndReg) + bReg;
  PC++;
}
void Processor::INHRTI() {
  flags = Memory[++SP];
  bReg = Memory[++SP];
  aReg = Memory[++SP];
  (*curIndReg) = (Memory[++SP] << 8);
  (*curIndReg) += Memory[++SP];
  PC = Memory[++SP] << 8;
  PC += Memory[++SP];
}
void Processor::INHPSHX() {
  Memory[SP--] = (*curIndReg) & 0xFF;
  Memory[SP--] = (*curIndReg) >> 8;
  PC++;
}
void Processor::INHMUL() {
  uint16_t uInt16 = static_cast<uint16_t>(aReg) * static_cast<uint16_t>(bReg);
  updateFlags(Carry, (uInt16 >> 8) != 0);
  aReg = (uInt16 >> 8);
  bReg = (uInt16 & 0xFF);
  PC++;
}
void Processor::INHWAI() {
  if (WAIStatus == false) {
    uint16_t uInt16 = PC + 1;
    Memory[SP--] = uInt16 & 0xFF;
    Memory[SP--] = uInt16 >> 8;
    Memory[SP--] = (*curIndReg) & 0xFF;
    Memory[SP--] = (*curIndReg) >> 8;
    Memory[SP--] = aReg;
    Memory[SP--] = bReg;
    Memory[SP--] = flags;
    WAIStatus = true;
  }
}
void Processor::INHSWI() {
  PC++;
  Memory[SP--] = PC & 0xFF;
  Memory[SP--] = PC >> 8;
  Memory[SP--] = (*curIndReg) & 0xFF;
  Memory[SP--] = (*curIndReg) >> 8;
  Memory[SP--] = aReg;
  Memory[SP--] = bReg;
  Memory[SP--] = flags;
  updateFlags(InterruptMask, 1);
  PC = (Memory[(interruptLocations - 5)] << 8) + Memory[(interruptLocations - 4)];
}
void Processor::INHNEGA() {
  aReg = 0x0 - aReg;
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, aReg == 0x80);
  updateFlags(Carry, aReg != 0);
  PC++;
}
void Processor::INHCOMA() {
  aReg = 0xFF - aReg;
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);
  updateFlags(Carry, 1);
  PC++;
}
void Processor::INHLSRA() {
  uint8_t uInt8 = (aReg & 0x1);
  aReg = (aReg >> 1);
  updateFlags(Negative, 0);
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, uInt8);
  updateFlags(Carry, uInt8);
  PC++;
}
void Processor::INHRORA() {
  uint8_t uInt8 = (aReg & 0x01);
  aReg = aReg >> 1;
  aReg += (flags & 0x01) << 7;
  updateFlags(Carry, uInt8);
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, uInt8 ^ bit(aReg, 7));
  PC++;
}
void Processor::INHASRA() {
  uint8_t uInt8 = aReg & 0x01;
  aReg = (aReg >> 1) + (aReg & 0x80);
  updateFlags(Carry, uInt8);
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, uInt8 ^ bit(aReg, 7));
  PC++;
}
void Processor::INHASLA() {
  uint8_t uInt8 = bit(aReg, 7);
  aReg = aReg << 1;
  updateFlags(Carry, uInt8);
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, uInt8 ^ bit(aReg, 7));
  PC++;
}
void Processor::INHROLA() {
  uint8_t uInt8 = bit(aReg, 7);
  aReg = (aReg << 1) + (flags & 0x01);
  updateFlags(Carry, uInt8);
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, uInt8 ^ bit(aReg, 7));
  PC++;
}
void Processor::INHDECA() {
  updateFlags(Overflow, aReg == 0x80);
  aReg--;
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  PC++;
}
void Processor::INHINCA() {
  updateFlags(Overflow, aReg == 0x7F);
  aReg++;
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  PC++;
}
void Processor::INHTSTA() {
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);
  updateFlags(Carry, 0);
  PC++;
}
void Processor::INHCLRA() {
  aReg = 0;
  updateFlags(Negative, 0);
  updateFlags(Zero, 1);
  updateFlags(Overflow, 0);
  updateFlags(Carry, 0);
  PC++;
}
void Processor::INHNEGB() {
  bReg = 0x0 - bReg;
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, bReg == 0x80);
  updateFlags(Carry, bReg != 0);
  PC++;
}
void Processor::INHCOMB() {
  bReg = 0xFF - bReg;
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);
  updateFlags(Carry, 1);
  PC++;
}
void Processor::INHLSRB() {
  uint8_t uInt8 = (bReg & 0x1);
  bReg = (bReg >> 1);
  updateFlags(Negative, 0);
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, uInt8);
  updateFlags(Carry, uInt8);
  PC++;
}
void Processor::INHRORB() {
  uint8_t uInt8 = (bReg & 0x01);
  bReg = bReg >> 1;
  bReg += (flags & 0x01) << 7;
  updateFlags(Carry, uInt8);
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, uInt8 ^ bit(bReg, 7));
  PC++;
}
void Processor::INHASRB() {
  uint8_t uInt8 = bReg & 0x01;
  bReg = (bReg >> 1) + (bReg & 0x80);
  updateFlags(Carry, uInt8);
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, uInt8 ^ bit(bReg, 7));
  PC++;
}
void Processor::INHASLB() {
  uint8_t uInt8 = bit(bReg, 7);
  bReg = bReg << 1;
  updateFlags(Carry, uInt8);
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, uInt8 ^ bit(bReg, 7));
  PC++;
}
void Processor::INHROLB() {
  uint8_t uInt8 = bit(bReg, 7);
  bReg = (bReg << 1) + (flags & 0x01);
  updateFlags(Carry, uInt8);
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, uInt8 ^ bit(bReg, 7));
  PC++;
}
void Processor::INHDECB() {
  updateFlags(Overflow, bReg == 0x80);
  bReg--;
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  PC++;
}
void Processor::INHINCB() {
  updateFlags(Overflow, bReg == 0x7F);
  bReg++;
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  PC++;
}
void Processor::INHTSTB() {
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);
  updateFlags(Carry, 0);
  PC++;
}
void Processor::INHCLRB() {
  bReg = 0;
  updateFlags(Negative, 0);
  updateFlags(Zero, 1);
  updateFlags(Overflow, 0);
  updateFlags(Carry, 0);
  PC++;
}
void Processor::INDNEG() {
  uint16_t adr = Memory[(PC + 1) % 0x10000] + *curIndReg;
  Memory[adr] = 0x0 - Memory[adr];
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, Memory[adr] == 0x80);
  updateFlags(Carry, Memory[adr] != 0);

  PC += 2;
}
void Processor::INDCOM() {
  uint16_t adr = Memory[(PC + 1) % 0x10000] + *curIndReg;
  Memory[adr] = 0xFF - Memory[adr];
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, 0);
  updateFlags(Carry, 1);

  PC += 2;
}
void Processor::INDLSR() {
  uint16_t adr = Memory[(PC + 1) % 0x10000] + *curIndReg;
  uint8_t uInt8 = (Memory[adr] & 0x1);
  Memory[adr] = (Memory[adr] >> 1);
  updateFlags(Negative, 0);
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, uInt8);
  updateFlags(Carry, uInt8);

  PC += 2;
}
void Processor::INDROR() {
  uint16_t adr = Memory[(PC + 1) % 0x10000] + *curIndReg;
  uint8_t uInt8 = (Memory[adr] & 0x01);
  Memory[adr] = Memory[adr] >> 1;
  Memory[adr] += (flags & 0x01) << 7;
  updateFlags(Carry, uInt8);
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, uInt8 ^ bit(Memory[adr], 7));

  PC += 2;
}
void Processor::INDASR() {
  uint16_t adr = Memory[(PC + 1) % 0x10000] + *curIndReg;
  uint8_t uInt8 = (Memory[adr] & 0x01);
  updateFlags(Carry, uInt8);
  Memory[adr] = (Memory[adr] >> 1) + (Memory[adr] & 0x80);
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, uInt8 ^ bit(Memory[adr], 7));

  PC += 2;
}
void Processor::INDASL() {
  uint16_t adr = Memory[(PC + 1) % 0x10000] + *curIndReg;
  uint8_t uInt8 = bit(Memory[adr], 7);
  updateFlags(Carry, uInt8);
  Memory[adr] = Memory[adr] << 1;
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, uInt8 ^ bit(Memory[adr], 7));

  PC += 2;
}
void Processor::INDROL() {
  uint16_t adr = Memory[(PC + 1) % 0x10000] + *curIndReg;
  uint8_t uInt8 = bit(Memory[adr], 7);
  Memory[adr] = (Memory[adr] << 1) + (flags & 0x01);
  updateFlags(Carry, uInt8);
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, uInt8 ^ bit(Memory[adr], 7));

  PC += 2;
}
void Processor::INDDEC() {
  uint16_t adr = Memory[(PC + 1) % 0x10000] + *curIndReg;
  updateFlags(Overflow, Memory[adr] == 0x80);
  Memory[adr]--;
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);

  PC += 2;
}
void Processor::INDINC() {
  uint16_t adr = Memory[(PC + 1) % 0x10000] + *curIndReg;
  updateFlags(Overflow, Memory[adr] == 0x7F);
  Memory[adr]++;
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);

  PC += 2;
}
void Processor::INDTST() {
  uint16_t adr = Memory[(PC + 1) % 0x10000] + *curIndReg;
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, 0);
  updateFlags(Carry, 0);
  PC += 2;
}
void Processor::INDJMP() {
  PC = ((Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000);
}
void Processor::INDCLR() {
  uint16_t adr = Memory[(PC + 1) % 0x10000] + *curIndReg;
  Memory[adr] = 0;
  updateFlags(Negative, 0);
  updateFlags(Zero, 1);
  updateFlags(Overflow, 0);
  updateFlags(Carry, 0);

  PC += 2;
}
void Processor::EXTNEG() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  Memory[adr] = 0x0 - Memory[adr];
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, Memory[adr] == 0x80);
  updateFlags(Carry, Memory[adr] != 0);
  PC += 3;
}
void Processor::EXTCOM() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  Memory[adr] = 0xFF - Memory[adr];
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, 0);
  updateFlags(Carry, 1);
  PC += 3;
}
void Processor::EXTLSR() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  uint8_t uInt8 = (Memory[adr] & 0x1);
  Memory[adr] = (Memory[adr] >> 1);
  updateFlags(Negative, 0);
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, uInt8);
  updateFlags(Carry, uInt8);
  PC += 3;
}
void Processor::EXTROR() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  uint8_t uInt8 = (Memory[adr] & 0x01);
  Memory[adr] = Memory[adr] >> 1;
  Memory[adr] += (flags & 0x01) << 7;
  updateFlags(Carry, uInt8);
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, uInt8 ^ bit(Memory[adr], 7));
  PC += 3;
}
void Processor::EXTASR() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  uint8_t uInt8 = (Memory[adr] & 0x01);
  updateFlags(Carry, uInt8);
  Memory[adr] = (Memory[adr] >> 1) + (Memory[adr] & 0x80);
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, uInt8 ^ bit(Memory[adr], 7));
  PC += 3;
}
void Processor::EXTASL() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  uint8_t uInt8 = bit(Memory[adr], 7);
  updateFlags(Carry, uInt8);
  Memory[adr] = Memory[adr] << 1;
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, uInt8 ^ bit(Memory[adr], 7));
  PC += 3;
}
void Processor::EXTROL() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  uint8_t uInt8 = bit(Memory[adr], 7);
  Memory[adr] = (Memory[adr] << 1) + (flags & 0x01);
  updateFlags(Carry, uInt8);
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, uInt8 ^ bit(Memory[adr], 7));
  PC += 3;
}
void Processor::EXTDEC() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  updateFlags(Overflow, Memory[adr] == 0x80);
  Memory[adr]--;
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  PC += 3;
}
void Processor::EXTINC() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  updateFlags(Overflow, Memory[adr] == 0x7F);
  Memory[adr]++;
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  PC += 3;
}
void Processor::EXTTST() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  updateFlags(Negative, bit(Memory[adr], 7));
  updateFlags(Zero, Memory[adr] == 0);
  updateFlags(Overflow, 0);
  updateFlags(Carry, 0);
  PC += 3;
}
void Processor::EXTJMP() {
  PC = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
}
void Processor::EXTCLR() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  Memory[adr] = 0;
  updateFlags(Negative, 0);
  updateFlags(Zero, 1);
  updateFlags(Overflow, 0);
  updateFlags(Carry, 0);
  PC += 3;
}
void Processor::IMMSUBA() {
  uint8_t uInt8 = Memory[(PC + 1) % 0x10000];
  uint8_t uInt82 = aReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(aReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(aReg, 7)));
  aReg = uInt82;
  PC += 2;
}
void Processor::IMMCMPA() {
  uint8_t uInt8 = Memory[(PC + 1) % 0x10000];
  uint8_t uInt82 = aReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(aReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(aReg, 7)));
  PC += 2;
}
void Processor::IMMSBCA() {
  uint8_t uInt8 = Memory[(PC + 1) % 0x10000];
  uint8_t uInt82 = aReg - uInt8 - (flags & 0x1);
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(aReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(aReg, 7)));
  aReg = uInt82;
  PC += 2;
}
void Processor::IMMSUBD() {
  uint16_t val = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  uint8_t uInt16 = (aReg << 8) + bReg;
  uint8_t uInt162 = uInt16 - val;
  updateFlags(Negative, bit(uInt162, 15));
  updateFlags(Zero, uInt162 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(val, 15) && !bit(uInt162, 15)) || (!bit(aReg, 7) && bit(val, 15) && bit(uInt162, 15)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(val, 15)) || (bit(val, 15) && bit(uInt162, 15)) || (bit(uInt162, 15) && !bit(aReg, 7)));
  aReg = (uInt162 >> 8);
  bReg = (uInt162 & 0xFF);

  PC += 3;
}
void Processor::IMMANDA() {
  aReg = (aReg & Memory[(PC + 1) % 0x10000]);
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::IMMBITA() {
  uint8_t uInt8 = (aReg & Memory[(PC + 1) % 0x10000]);
  updateFlags(Negative, bit(uInt8, 7));
  updateFlags(Zero, uInt8 == 0);
  updateFlags(Overflow, 0);
  PC += 2;
}
void Processor::IMMLDAA() {
  aReg = Memory[(PC + 1) % 0x10000];
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::IMMEORA() {
  aReg = aReg ^ Memory[(PC + 1) % 0x10000];
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::IMMADCA() {
  uint8_t uInt8 = Memory[(PC + 1) % 0x10000];
  uint16_t uInt16 = aReg + uInt8 + (flags & 0x01);
  updateFlags(HalfCarry, (bit(aReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(aReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(aReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(aReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(aReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(aReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  aReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, aReg == 0);

  PC += 2;
}
void Processor::IMMORAA() {
  aReg = aReg | Memory[(PC + 1) % 0x10000];
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::IMMADDA() {
  uint8_t uInt8 = Memory[(PC + 1) % 0x10000];
  uint16_t uInt16 = aReg + uInt8;
  updateFlags(HalfCarry, (bit(aReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(aReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(aReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(aReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(aReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(aReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  aReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, aReg == 0);

  PC += 2;
}
void Processor::IMMCPX() {
  uint16_t uInt16 = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  uint16_t uInt162 = (*curIndReg) - uInt16;
  updateFlags(Negative, bit(uInt162, 15));
  updateFlags(Zero, uInt162 == 0);
  updateFlags(Overflow, (bit((*curIndReg), 15) && !bit(uInt16, 15) && !bit(uInt162, 15)) || (!bit((*curIndReg), 15) && bit(uInt16, 15) && bit(uInt162, 15)));
  PC += 3;
}
void Processor::RELBSR() {
  int8_t sInt8 = Memory[(PC + 1) % 0x10000];
  PC += 2;
  Memory[SP] = (PC & 0xFF);
  Memory[SP - 1] = ((PC >> 8));
  SP -= 2;

  PC += sInt8;
}
void Processor::IMMLDS() {
  SP = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  updateFlags(Negative, bit(SP, 15));
  updateFlags(Zero, SP == 0);
  updateFlags(Overflow, 0);
  PC += 3;
}
void Processor::DIRSUBA() {
  uint8_t uInt8 = Memory[Memory[(PC + 1) % 0x10000]];
  uint8_t uInt82 = aReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(aReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(aReg, 7)));
  aReg = uInt82;
  PC += 2;
}
void Processor::DIRCMPA() {
  uint8_t uInt8 = Memory[Memory[(PC + 1) % 0x10000]];
  uint8_t uInt82 = aReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(aReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(aReg, 7)));
  PC += 2;
}
void Processor::DIRSBCA() {
  uint8_t uInt8 = Memory[Memory[(PC + 1) % 0x10000]];
  uint8_t uInt82 = aReg - uInt8 - (flags & 0x1);
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(aReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(aReg, 7)));
  aReg = uInt82;
  PC += 2;
}
void Processor::DIRSUBD() {
  uint8_t uInt8 = Memory[(PC + 1) % 0x10000];
  uint16_t adr = (Memory[uInt8] << 8) + Memory[uInt8 + 1];
  uint16_t uInt16 = (aReg << 8) + bReg;
  uint16_t uInt162 = uInt16 - adr;
  updateFlags(Negative, bit(uInt162, 15));
  updateFlags(Zero, uInt162 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(adr, 15) && !bit(uInt162, 15)) || (!bit(aReg, 7) && bit(adr, 15) && bit(uInt162, 15)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(adr, 15)) || (bit(adr, 15) && bit(uInt162, 15)) || (bit(uInt162, 15) && !bit(aReg, 7)));
  aReg = (uInt162 >> 8);
  bReg = (uInt162 & 0xFF);
  PC += 2;
}
void Processor::DIRANDA() {
  aReg = (aReg & Memory[Memory[(PC + 1) % 0x10000]]);
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);
  PC += 2;
}
void Processor::DIRBITA() {
  uint8_t uInt8 = (aReg & Memory[Memory[(PC + 1) % 0x10000]]);
  updateFlags(Negative, bit(uInt8, 7));
  updateFlags(Zero, uInt8 == 0);
  updateFlags(Overflow, 0);
  PC += 2;
}
void Processor::DIRLDAA() {
  aReg = Memory[Memory[(PC + 1) % 0x10000]];
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);
  PC += 2;
}
void Processor::DIRSTAA() {
  uint16_t adr = Memory[(PC + 1) % 0x10000];
  Memory[adr] = aReg;
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::DIREORA() {
  aReg = aReg ^ Memory[Memory[(PC + 1) % 0x10000]];
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::DIRADCA() {
  uint8_t uInt8 = Memory[Memory[(PC + 1) % 0x10000]];
  uint16_t uInt16 = aReg + uInt8 + (flags & 0x01);
  updateFlags(HalfCarry, (bit(aReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(aReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(aReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(aReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(aReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(aReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  aReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, aReg == 0);

  PC += 2;
}
void Processor::DIRORAA() {
  aReg = aReg | Memory[Memory[(PC + 1) % 0x10000]];
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::DIRADDA() {
  uint8_t uInt8 = Memory[Memory[(PC + 1) % 0x10000]];
  uint16_t uInt16 = aReg + uInt8;
  updateFlags(HalfCarry, (bit(aReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(aReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(aReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(aReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(aReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(aReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  aReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, aReg == 0);

  PC += 2;
}
void Processor::DIRCPX() {
  uint16_t adr = Memory[(PC + 1) % 0x10000];
  uint16_t uInt16 = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  uint16_t uInt162 = (*curIndReg) - uInt16;
  updateFlags(Negative, bit(uInt162, 15));
  updateFlags(Zero, uInt162 == 0);
  updateFlags(Overflow, (bit((*curIndReg), 15) && !bit(uInt16, 15) && !bit(uInt162, 15)) || (!bit((*curIndReg), 15) && bit(uInt16, 15) && bit(uInt162, 15)));
  PC += 2;
}
void Processor::DIRJSR() {
  uint16_t adr = Memory[(PC + 1) % 0x10000];
  PC += 2;
  Memory[SP] = (PC & 0xFF);

  Memory[SP - 1] = (PC >> 8);

  SP -= 2;
  PC = adr;
}

void Processor::DIRLDS() {
  uint16_t adr = Memory[(PC + 1) % 0x10000];
  SP = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  updateFlags(Negative, bit(SP, 15));
  updateFlags(Zero, SP == 0);
  updateFlags(Overflow, 0);
  PC += 2;
}
void Processor::DIRSTS() {
  uint16_t adr = Memory[(PC + 1) % 0x10000];
  Memory[adr] = SP >> 8;
  Memory[(adr + 1) % 0x10000] = (SP & 0xFF);
  updateFlags(Negative, bit(SP, 15));
  updateFlags(Zero, SP == 0);
  updateFlags(Overflow, 0);
  PC += 2;
}
void Processor::INDSUBA() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  uint8_t uInt82 = aReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(aReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(aReg, 7)));
  aReg = uInt82;
  PC += 2;
}
void Processor::INDCMPA() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  uint8_t uInt82 = aReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(aReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(aReg, 7)));
  PC += 2;
}
void Processor::INDSBCA() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  uint8_t uInt82 = aReg - uInt8 - (flags & 0x1);
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(aReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(aReg, 7)));
  aReg = uInt82;
  PC += 2;
}
void Processor::INDSUBD() {
  uint8_t uInt8 = (Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000;
  uint16_t adr = (Memory[uInt8] << 8) + Memory[uInt8 + 1];
  uint16_t uInt16 = (aReg << 8) + bReg;
  uint16_t uInt162 = uInt16 - adr;
  updateFlags(Negative, bit(uInt162, 15));
  updateFlags(Zero, uInt162 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(adr, 15) && !bit(uInt162, 15)) || (!bit(aReg, 7) && bit(adr, 15) && bit(uInt162, 15)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(adr, 15)) || (bit(adr, 15) && bit(uInt162, 15)) || (bit(uInt162, 15) && !bit(aReg, 7)));
  aReg = (uInt162 >> 8);
  bReg = (uInt162 & 0xFF);

  PC += 2;
}
void Processor::INDANDA() {
  aReg = (aReg & Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000]);
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::INDBITA() {
  uint8_t uInt8 = (aReg & Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000]);
  updateFlags(Negative, bit(uInt8, 7));
  updateFlags(Zero, uInt8 == 0);
  updateFlags(Overflow, 0);
  PC += 2;
}
void Processor::INDLDAA() {
  aReg = Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::INDSTAA() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000;
  Memory[adr] = aReg;
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::INDEORA() {
  aReg = aReg ^ Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::INDADCA() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  uint16_t uInt16 = aReg + uInt8 + (flags & 0x01);
  updateFlags(HalfCarry, (bit(aReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(aReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(aReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(aReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(aReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(aReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  aReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, aReg == 0);

  PC += 2;
}
void Processor::INDORAA() {
  aReg = aReg | Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::INDADDA() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  uint16_t uInt16 = aReg + uInt8;
  updateFlags(HalfCarry, (bit(aReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(aReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(aReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(aReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(aReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(aReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  aReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, aReg == 0);

  PC += 2;
}
void Processor::INDCPX() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000;
  uint16_t uInt16 = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  uint16_t uInt162 = (*curIndReg) - uInt16;
  updateFlags(Negative, bit(uInt162, 15));
  updateFlags(Zero, uInt162 == 0);
  updateFlags(Overflow, (bit((*curIndReg), 15) && !bit(uInt16, 15) && !bit(uInt162, 15)) || (!bit((*curIndReg), 15) && bit(uInt16, 15) && bit(uInt162, 15)));
  PC += 2;
}
void Processor::INDJSR() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000;
  PC += 2;
  Memory[SP] = (PC & 0xFF);

  Memory[SP - 1] = ((PC >> 8));

  SP -= 2;
  PC = adr;
}
void Processor::INDLDS() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000;
  SP = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  updateFlags(Negative, bit(SP, 15));
  updateFlags(Zero, SP == 0);
  updateFlags(Overflow, 0);
  PC += 2;
}
void Processor::INDSTS() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000;
  Memory[adr] = SP >> 8;
  Memory[(adr + 1) % 0x10000] = (SP & 0xFF);
  updateFlags(Negative, bit(SP, 15));
  updateFlags(Zero, SP == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::EXTSUBA() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  uint8_t uInt82 = aReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(aReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(aReg, 7)));
  aReg = uInt82;
  PC += 3;
}
void Processor::EXTCMPA() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  uint8_t uInt82 = aReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(aReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(aReg, 7)));
  PC += 3;
}
void Processor::EXTSBCA() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  uint8_t uInt82 = aReg - uInt8 - (flags & 0x1);
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(aReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(aReg, 7)));
  aReg = uInt82;
  PC += 3;
}
void Processor::EXTSUBD() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  adr = (Memory[adr] << 8) + Memory[adr + 1];
  uint16_t uInt16 = (aReg << 8) + bReg;
  uint16_t uInt162 = uInt16 - adr;
  updateFlags(Negative, bit(uInt162, 15));
  updateFlags(Zero, uInt162 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && !bit(adr, 15) && !bit(uInt162, 15)) || (!bit(aReg, 7) && bit(adr, 15) && bit(uInt162, 15)));
  updateFlags(Carry, (!bit(aReg, 7) && bit(adr, 15)) || (bit(adr, 15) && bit(uInt162, 15)) || (bit(uInt162, 15) && !bit(aReg, 7)));
  aReg = (uInt162 >> 8);
  bReg = (uInt162 & 0xFF);

  PC += 3;
}
void Processor::EXTANDA() {
  aReg = (aReg & Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]]);
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 3;
}
void Processor::EXTBITA() {
  uint8_t uInt8 = (aReg & Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]]);
  updateFlags(Negative, bit(uInt8, 7));
  updateFlags(Zero, uInt8 == 0);
  updateFlags(Overflow, 0);
  PC += 3;
}
void Processor::EXTLDAA() {
  aReg = Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 3;
}
void Processor::EXTSTAA() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  Memory[adr] = aReg;
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 3;
}
void Processor::EXTEORA() {
  aReg = aReg ^ Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 3;
}
void Processor::EXTADCA() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  uint16_t uInt16 = aReg + uInt8 + (flags & 0x01);
  updateFlags(HalfCarry, (bit(aReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(aReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(aReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(aReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(aReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(aReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  aReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, aReg == 0);

  PC += 3;
}
void Processor::EXTORAA() {
  aReg = aReg | Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, aReg == 0);
  updateFlags(Overflow, 0);

  PC += 3;
}
void Processor::EXTADDA() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  uint16_t uInt16 = aReg + uInt8;
  updateFlags(HalfCarry, (bit(aReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(aReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(aReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(aReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(aReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(aReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  aReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, aReg == 0);

  PC += 3;
}
void Processor::EXTCPX() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  uint16_t uInt16 = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  uint16_t uInt162 = (*curIndReg) - uInt16;
  updateFlags(Negative, bit(uInt162, 15));
  updateFlags(Zero, uInt162 == 0);
  updateFlags(Overflow, (bit((*curIndReg), 15) && !bit(uInt16, 15) && !bit(uInt162, 15)) || (!bit((*curIndReg), 15) && bit(uInt16, 15) && bit(uInt162, 15)));
  PC += 3;
}
void Processor::EXTJSR() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  PC += 3;
  Memory[SP] = (PC & 0xFF);

  Memory[SP - 1] = ((PC >> 8));

  SP -= 2;
  PC = adr;
}
void Processor::EXTLDS() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  SP = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  updateFlags(Negative, bit(SP, 15));
  updateFlags(Zero, SP == 0);
  updateFlags(Overflow, 0);
  PC += 3;
}
void Processor::EXTSTS() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  Memory[adr] = SP >> 8;
  Memory[(adr + 1) % 0x10000] = (SP & 0xFF);
  updateFlags(Negative, bit(SP, 15));
  updateFlags(Zero, SP == 0);
  updateFlags(Overflow, 0);

  PC += 3;
}
void Processor::IMMSUBB() {
  uint8_t uInt8 = Memory[(PC + 1) % 0x10000];
  uint8_t uInt82 = bReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(bReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(bReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(bReg, 7)));
  bReg = uInt82;
  PC += 2;
}
void Processor::IMMCMPB() {
  uint8_t uInt8 = Memory[(PC + 1) % 0x10000];
  uint8_t uInt82 = bReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(bReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(bReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(bReg, 7)));
  PC += 2;
}
void Processor::IMMSBCB() {
  uint8_t uInt8 = Memory[(PC + 1) % 0x10000];
  uint8_t uInt82 = bReg - uInt8 - (flags & 0x1);
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(bReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(bReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(bReg, 7)));
  bReg = uInt82;
  PC += 2;
}
void Processor::IMMADDD() {
  uint16_t uInt16 = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  uint16_t uInt162 = (aReg << 8) + bReg;
  uInt162 = uInt162 + uInt16;
  updateFlags(Negative, bit(uInt162, 15));
  updateFlags(Zero, uInt162 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && bit(uInt16, 15) && !bit(uInt162, 12)) || (!bit(aReg, 7) && !bit(uInt16, 15) && bit(uInt162, 12)));
  updateFlags(Carry, (bit(aReg, 7) && bit(uInt16, 15)) || (bit(uInt16, 15) && !bit(uInt162, 12)) || (!bit(uInt162, 12) && bit(aReg, 7)));
  aReg = (uInt162 >> 8);
  bReg = (uInt162 & 0xFF);

  PC += 3;
}
void Processor::IMMANDB() {
  bReg = (bReg & Memory[(PC + 1) % 0x10000]);
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::IMMBITB() {
  uint8_t uInt8 = (bReg & Memory[(PC + 1) % 0x10000]);
  updateFlags(Negative, bit(uInt8, 7));
  updateFlags(Zero, uInt8 == 0);
  updateFlags(Overflow, 0);
  PC += 2;
}
void Processor::IMMLDAB() {
  bReg = Memory[(PC + 1) % 0x10000];
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::IMMEORB() {
  bReg = bReg ^ Memory[(PC + 1) % 0x10000];
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::IMMADCB() {
  uint8_t uInt8 = Memory[(PC + 1) % 0x10000];
  uint16_t uInt16 = bReg + uInt8 + (flags & 0x01);
  updateFlags(HalfCarry, (bit(bReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(bReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(bReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(bReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(bReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(bReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  bReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, bReg == 0);

  PC += 2;
}
void Processor::IMMORAB() {
  bReg = bReg | Memory[(PC + 1) % 0x10000];
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::IMMADDB() {
  uint8_t uInt8 = Memory[(PC + 1) % 0x10000];
  uint16_t uInt16 = bReg + uInt8;
  updateFlags(HalfCarry, (bit(bReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(bReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(bReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(bReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(bReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(bReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  bReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, bReg == 0);

  PC += 2;
}
void Processor::IMMLDD() {
  uint16_t uInt16 = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  updateFlags(Negative, bit(uInt16, 15));
  updateFlags(Zero, uInt16 == 0);
  updateFlags(Overflow, 0);
  aReg = (uInt16 >> 8);
  bReg = (uInt16 & 0xFF);

  PC += 3;
}
void Processor::IMMLDX() {
  (*curIndReg) = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  updateFlags(Negative, bit((*curIndReg), 15));
  updateFlags(Zero, (*curIndReg) == 0);
  updateFlags(Overflow, 0);
  PC += 3;
}
void Processor::DIRSUBB() {
  uint8_t uInt8 = Memory[Memory[(PC + 1) % 0x10000]];
  uint8_t uInt82 = bReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(bReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(bReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(bReg, 7)));
  bReg = uInt82;
  PC += 2;
}
void Processor::DIRCMPB() {
  uint8_t uInt8 = Memory[Memory[(PC + 1) % 0x10000]];
  uint8_t uInt82 = bReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(bReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(bReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(bReg, 7)));
  PC += 2;
}
void Processor::DIRSBCB() {
  uint8_t uInt8 = Memory[Memory[(PC + 1) % 0x10000]];
  uint8_t uInt82 = bReg - uInt8 - (flags & 0x1);
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(bReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(bReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(bReg, 7)));
  bReg = uInt82;
  PC += 2;
}
void Processor::DIRADDD() {
  uint16_t adr = Memory[(PC + 1) % 0x10000];
  uint16_t uInt16 = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  uint16_t uInt162 = (aReg << 8) + bReg;
  uInt162 = uInt162 + uInt16;
  updateFlags(Negative, bit(uInt162, 15));
  updateFlags(Zero, uInt162 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && bit(uInt16, 15) && !bit(uInt162, 12)) || (!bit(aReg, 7) && !bit(uInt16, 15) && bit(uInt162, 12)));
  updateFlags(Carry, (bit(aReg, 7) && bit(uInt16, 15)) || (bit(uInt16, 15) && !bit(uInt162, 12)) || (!bit(uInt162, 12) && bit(aReg, 7)));
  aReg = (uInt162 >> 8);
  bReg = (uInt162 & 0xFF);

  PC += 2;
}
void Processor::DIRANDB() {
  bReg = (bReg & Memory[Memory[(PC + 1) % 0x10000]]);
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::DIRBITB() {
  uint8_t uInt8 = (bReg & Memory[Memory[(PC + 1) % 0x10000]]);
  updateFlags(Negative, bit(uInt8, 7));
  updateFlags(Zero, uInt8 == 0);
  updateFlags(Overflow, 0);
  PC += 2;
}
void Processor::DIRLDAB() {
  bReg = Memory[Memory[(PC + 1) % 0x10000]];
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::DIRSTAB() {
  uint16_t adr = Memory[(PC + 1) % 0x10000];
  Memory[adr] = bReg;
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::DIREORB() {
  bReg = bReg ^ Memory[Memory[(PC + 1) % 0x10000]];
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::DIRADCB() {
  uint8_t uInt8 = Memory[Memory[(PC + 1) % 0x10000]];
  uint16_t uInt16 = bReg + uInt8 + (flags & 0x01);
  updateFlags(HalfCarry, (bit(bReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(bReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(bReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(bReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(bReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(bReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  bReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, bReg == 0);

  PC += 2;
}
void Processor::DIRORAB() {
  bReg = bReg | Memory[Memory[(PC + 1) % 0x10000]];
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::DIRADDB() {
  uint8_t uInt8 = Memory[Memory[(PC + 1) % 0x10000]];
  uint16_t uInt16 = bReg + uInt8;
  updateFlags(HalfCarry, (bit(bReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(bReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(bReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(bReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(bReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(bReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  bReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, bReg == 0);

  PC += 2;
}
void Processor::DIRLDD() {
  uint16_t adr = Memory[(PC + 1) % 0x10000];
  uint16_t uInt16 = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  updateFlags(Negative, bit(uInt16, 15));
  updateFlags(Zero, uInt16 == 0);
  updateFlags(Overflow, 0);
  aReg = (uInt16 >> 8);
  bReg = (uInt16 & 0xFF);

  PC += 2;
}
void Processor::DIRSTD() {
  uint16_t adr = Memory[(PC + 1) % 0x10000];
  Memory[adr] = aReg;
  Memory[adr + 1] = bReg;
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, bReg + aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::DIRLDX() {
  uint16_t adr = Memory[(PC + 1) % 0x10000];
  (*curIndReg) = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  updateFlags(Negative, bit((*curIndReg), 15));
  updateFlags(Zero, (*curIndReg) == 0);
  updateFlags(Overflow, 0);
  PC += 2;
}
void Processor::DIRSTX() {
  uint16_t adr = Memory[(PC + 1) % 0x10000];
  Memory[adr] = (*curIndReg) >> 8;
  Memory[(adr + 1) % 0x10000] = ((*curIndReg) & 0xFF);
  updateFlags(Negative, bit((*curIndReg), 15));
  updateFlags(Zero, (*curIndReg) == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::INDSUBB() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  uint8_t uInt82 = bReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(bReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(bReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(bReg, 7)));
  bReg = uInt82;
  PC += 2;
}
void Processor::INDCMPB() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  uint8_t uInt82 = bReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(bReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(bReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(bReg, 7)));
  PC += 2;
}
void Processor::INDSBCB() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  uint8_t uInt82 = bReg - uInt8 - (flags & 0x1);
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(bReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(bReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(bReg, 7)));
  bReg = uInt82;
  PC += 2;
}
void Processor::INDADDD() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000;
  uint16_t uInt16 = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  uint16_t uInt162 = (aReg << 8) + bReg;
  uInt162 = uInt162 + uInt16;
  updateFlags(Negative, bit(uInt162, 15));
  updateFlags(Zero, uInt162 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && bit(uInt16, 15) && !bit(uInt162, 12)) || (!bit(aReg, 7) && !bit(uInt16, 15) && bit(uInt162, 12)));
  updateFlags(Carry, (bit(aReg, 7) && bit(uInt16, 15)) || (bit(uInt16, 15) && !bit(uInt162, 12)) || (!bit(uInt162, 12) && bit(aReg, 7)));
  aReg = (uInt162 >> 8);
  bReg = (uInt162 & 0xFF);

  PC += 2;
}
void Processor::INDANDB() {
  bReg = (bReg & Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000]);
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::INDBITB() {
  uint8_t uInt8 = (bReg & Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000]);
  updateFlags(Negative, bit(uInt8, 7));
  updateFlags(Zero, uInt8 == 0);
  updateFlags(Overflow, 0);
  PC += 2;
}
void Processor::INDLDAB() {
  bReg = Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::INDSTAB() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000;
  Memory[adr] = bReg;
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::INDEORB() {
  bReg = bReg ^ Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::INDADCB() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  uint16_t uInt16 = bReg + uInt8 + (flags & 0x01);
  updateFlags(HalfCarry, (bit(bReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(bReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(bReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(bReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(bReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(bReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  bReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, bReg == 0);

  PC += 2;
}
void Processor::INDORAB() {
  bReg = bReg | Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::INDADDB() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000];
  uint16_t uInt16 = bReg + uInt8;
  updateFlags(HalfCarry, (bit(bReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(bReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(bReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(bReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(bReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(bReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  bReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, bReg == 0);

  PC += 2;
}
void Processor::INDLDD() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000;
  uint16_t uInt16 = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  updateFlags(Negative, bit(uInt16, 15));
  updateFlags(Zero, uInt16 == 0);
  updateFlags(Overflow, 0);
  aReg = (uInt16 >> 8);
  bReg = (uInt16 & 0xFF);

  PC += 2;
}
void Processor::INDSTD() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000;
  Memory[adr] = aReg;
  Memory[adr + 1] = bReg;
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, bReg + aReg == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::INDLDX() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000;
  (*curIndReg) = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  updateFlags(Negative, bit((*curIndReg), 15));
  updateFlags(Zero, (*curIndReg) == 0);
  updateFlags(Overflow, 0);
  PC += 2;
}
void Processor::INDSTX() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] + *curIndReg) % 0x10000;
  Memory[adr] = (*curIndReg) >> 8;
  Memory[(adr + 1) % 0x10000] = ((*curIndReg) & 0xFF);
  updateFlags(Negative, bit((*curIndReg), 15));
  updateFlags(Zero, (*curIndReg) == 0);
  updateFlags(Overflow, 0);

  PC += 2;
}
void Processor::EXTSUBB() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  uint8_t uInt82 = bReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(bReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(bReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(bReg, 7)));
  bReg = uInt82;
  PC += 3;
}
void Processor::EXTCMPB() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  uint8_t uInt82 = bReg - uInt8;
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(bReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(bReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(bReg, 7)));
  PC += 3;
}
void Processor::EXTSBCB() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  uint8_t uInt82 = bReg - uInt8 - (flags & 0x1);
  updateFlags(Negative, bit(uInt82, 7));
  updateFlags(Zero, uInt82 == 0);
  updateFlags(Overflow, (bit(bReg, 7) && !bit(uInt8, 7) && !bit(uInt82, 7)) || (!bit(bReg, 7) && bit(uInt8, 7) && bit(uInt82, 7)));
  updateFlags(Carry, (!bit(bReg, 7) && bit(uInt8, 7)) || (bit(uInt8, 7) && bit(uInt82, 7)) || (bit(uInt82, 7) && !bit(bReg, 7)));
  bReg = uInt82;
  PC += 3;
}
void Processor::EXTADDD() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  uint16_t uInt16 = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  uint16_t uInt162 = (aReg << 8) + bReg;
  uInt162 = uInt162 + uInt16;
  updateFlags(Negative, bit(uInt162, 15));
  updateFlags(Zero, uInt162 == 0);
  updateFlags(Overflow, (bit(aReg, 7) && bit(uInt16, 15) && !bit(uInt162, 12)) || (!bit(aReg, 7) && !bit(uInt16, 15) && bit(uInt162, 12)));
  updateFlags(Carry, (bit(aReg, 7) && bit(uInt16, 15)) || (bit(uInt16, 15) && !bit(uInt162, 12)) || (!bit(uInt162, 12) && bit(aReg, 7)));
  aReg = (uInt162 >> 8);
  bReg = (uInt162 & 0xFF);

  PC += 3;
}
void Processor::EXTANDB() {
  bReg = (bReg & Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]]);
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 3;
}
void Processor::EXTBITB() {
  uint8_t uInt8 = (bReg & Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]]);
  updateFlags(Negative, bit(uInt8, 7));
  updateFlags(Zero, uInt8 == 0);
  updateFlags(Overflow, 0);
  PC += 3;
}
void Processor::EXTLDAB() {
  bReg = Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 3;
}
void Processor::EXTSTAB() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  Memory[adr] = bReg;
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 3;
}
void Processor::EXTEORB() {
  bReg = bReg ^ Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 3;
}
void Processor::EXTADCB() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  uint16_t uInt16 = bReg + uInt8 + (flags & 0x01);
  updateFlags(HalfCarry, (bit(bReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(bReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(bReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(bReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(bReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(bReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  bReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, bReg == 0);

  PC += 3;
}
void Processor::EXTORAB() {
  bReg = bReg | Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  updateFlags(Negative, bit(bReg, 7));
  updateFlags(Zero, bReg == 0);
  updateFlags(Overflow, 0);

  PC += 3;
}
void Processor::EXTADDB() {
  uint8_t uInt8 = Memory[(Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000]];
  uint16_t uInt16 = bReg + uInt8;
  updateFlags(HalfCarry, (bit(bReg, 3) && bit(uInt8, 3)) || (bit(uInt8, 3) && !bit(uInt16, 3)) || (!bit(uInt16, 3) && bit(bReg, 3)));
  updateFlags(Negative, bit(uInt16, 7));
  updateFlags(Overflow, (bit(bReg, 7) && bit(uInt8, 7) && !bit(uInt16, 7)) || (!bit(bReg, 7) && !bit(uInt8, 7) && bit(uInt16, 7)));
  updateFlags(Carry, (bit(bReg, 7) && bit(uInt8, 7)) || (!bit(uInt16, 7) && bit(bReg, 7)) || (bit(uInt8, 7) && !bit(uInt16, 7)));
  bReg = static_cast<uint8_t>(uInt16);
  updateFlags(Zero, bReg == 0);

  PC += 3;
}
void Processor::EXTLDD() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  uint16_t uInt16 = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  updateFlags(Negative, bit(uInt16, 15));
  updateFlags(Zero, uInt16 == 0);
  updateFlags(Overflow, 0);
  aReg = (uInt16 >> 8);
  bReg = (uInt16 & 0xFF);

  PC += 3;
}
void Processor::EXTSTD() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  Memory[adr] = aReg;
  Memory[adr + 1] = bReg;
  updateFlags(Negative, bit(aReg, 7));
  updateFlags(Zero, bReg + aReg == 0);
  updateFlags(Overflow, 0);

  PC += 3;
}
void Processor::EXTLDX() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  (*curIndReg) = (Memory[adr] << 8) + Memory[(adr + 1) % 0x10000];
  updateFlags(Negative, bit((*curIndReg), 15));
  updateFlags(Zero, (*curIndReg) == 0);
  updateFlags(Overflow, 0);
  PC += 3;
}
void Processor::EXTSTX() {
  uint16_t adr = (Memory[(PC + 1) % 0x10000] << 8) + Memory[(PC + 2) % 0x10000];
  Memory[adr] = (*curIndReg) >> 8;
  Memory[(adr + 1) % 0x10000] = ((*curIndReg) & 0xFF);
  updateFlags(Negative, bit((*curIndReg), 15));
  updateFlags(Zero, (*curIndReg) == 0);
  updateFlags(Overflow, 0);

  PC += 3;
}
