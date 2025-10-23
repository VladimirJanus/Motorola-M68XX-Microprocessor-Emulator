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
#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "src/core/Core.h"

class Disassembler {
public:
  static Core::DisassemblyResult disassemble(Core::ProcessorVersion ver, uint16_t begLoc, uint16_t endLoc, std::array<uint8_t, 0x10000> &Memory);
};

#endif // DISASSEMBLER_H
