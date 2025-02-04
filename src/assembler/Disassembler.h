#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "src/core/Core.h"
#include <cstdint>
using Core::AddressingMode;
using Core::AssemblyMap;
using Core::DisassemblyResult;
using Core::Msg;
using Core::MsgType;
using Core::ProcessorVersion;
class Disassembler {
public:
  static DisassemblyResult disassemble(ProcessorVersion ver, uint16_t begLoc, uint16_t endLoc, std::array<uint8_t, 0x10000> &Memory);
};

#endif // DISASSEMBLER_H
