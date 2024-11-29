#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "src/utils/DataTypes.h"
#include <cstdint>
using DataTypes::AddressingMode;
using DataTypes::DisassemblyResult;
using DataTypes::Msg;
using DataTypes::MsgType;
using DataTypes::ProcessorVersion;
class Disassembler {
public:
  static DisassemblyResult disassemble(ProcessorVersion ver, uint16_t begLoc, uint16_t endLoc, std::array<uint8_t, 0x10000> &Memory);

private:
  //struct InputNextAddressResult {
  //  bool ok;
  //  int16_t address;
  //  QList<Msg> messages;
  //};
  //InputNextAddressResult inputNextAddress(int curAdr, QString err);
};

#endif // DISASSEMBLER_H
