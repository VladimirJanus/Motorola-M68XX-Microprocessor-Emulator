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
#include <QApplication>
#include "src/assembler/Assembler.h"
#include "src/ui/MainWindow.h"
#include "src/utils/DataTypes.h"
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>

void printHelp() {
  std::cout << DataTypes::programName.toStdString() << "\n"
            << "Usage: MotorolaEmulator.exe [options]\n\n"
            << "Options:\n"
            << "  --help, -h                      Show this help message\n"
            << "  --version, --ver                Show version information\n"
            << "  --ass, --assemble               Assemble a file\n"
            << "    --input, --in <file>          Input assembly file (required for --ass)\n"
            << "    --output, --out <file>        Output binary file (default: assembled_M6800.bin)\n"
            << "    --processor, --proc <ver>     Processor version (M6800 or M6803, default: M6800)\n"
            << "Running without arguments launches the GUI mode.\n";
}

void printVersion() {
  std::cout << "Current version: " << DataTypes::softwareVersion.toStdString() << std::endl;
}
bool readInputFile(const std::string& inputFile, std::string& content) {
  std::ifstream file(inputFile);
  if (!file) {
    std::cerr << "Error: Unable to open input file '" << inputFile << "'\n";
    return false;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  content = buffer.str();
  return true;
}

bool writeOutputFile(const std::string& outputFile, const std::array<uint8_t, 0x10000>& memory) {
  std::ofstream outFile(outputFile, std::ios::binary);
  if (!outFile) {
    std::cerr << "Error: Unable to open output file '" << outputFile << "' for writing\n";
    return false;
  }
  outFile.write(reinterpret_cast<const char*>(memory.data()), memory.size());
  outFile.close();
  return true;
}

int handleAssembly(int argc, char* argv[]) {
  DataTypes::ProcessorVersion processorVersion = DataTypes::ProcessorVersion::M6800;
  std::string inputFile;
  std::string outputFile;

  // Parse command-line arguments for assembly mode
  for (int i = 2; i < argc; ++i) {
    std::string flag = argv[i];
    if (flag == "--input" || flag == "--in") {
      if (++i < argc)
        inputFile = argv[i];
      else {
        std::cerr << "Error: --input requires a file argument\n";
        return 1;
      }
    } else if (flag == "--output" || flag == "--out") {
      if (++i < argc)
        outputFile = argv[i];
      else {
        std::cerr << "Error: --output requires a file argument\n";
        return 1;
      }
    } else if (flag == "--processor" || flag == "--proc") {
      if (++i < argc) {
        std::string procVersionStr = argv[i];
        if (procVersionStr == "M6803") {
          processorVersion = DataTypes::ProcessorVersion::M6803;
        } else if (procVersionStr != "M6800") {
          std::cerr << "Error: Invalid processor version. Use M6800 or M6803.\n";
          return 1;
        }
      } else {
        std::cerr << "Error: --processor requires a version argument\n";
        return 1;
      }
    } else {
      std::cerr << "Error: Unknown option '" << flag << "'\n";
      return 1;
    }
  }

  if (inputFile.empty()) {
    std::cerr << "Error: --input <file> is required for assembly mode\n";
    return 1;
  }

  std::string fileContent;
  if (!readInputFile(inputFile, fileContent)) {
    return 1;
  }
  QString qFileContent = QString::fromStdString(fileContent);

  std::array<uint8_t, 0x10000> memory = {0};
  AssemblyResult status = Assembler::assemble(processorVersion, qFileContent, memory);

  for (const Msg& message : status.messages) {
    if (message.type == MsgType::ERROR) {
      std::cout << "Error: (line:" << status.errorLineNum << ") ";
    }
    std::cout << message.message.toStdString() << std::endl;
  }
  if (!status.messages.empty() && status.messages.last().type == MsgType::ERROR) {
    std::cerr << "Assembly failed.\n";
    return 1;
  }

  if (outputFile.empty()) {
    outputFile = "assembled_" + (processorVersion == DataTypes::ProcessorVersion::M6803 ? std::string("M6803") : std::string("M6800")) + ".bin";
  }
  if (!writeOutputFile(outputFile, memory)) {
    return 1;
  }

  std::cout << "Assembly completed successfully. Output written to " << outputFile << std::endl;
  return 0;
}

int main(int argc, char* argv[]) {
  if (argc > 1) {
    std::string arg = argv[1];
    if (arg == "--help" || arg == "-h") {
      printHelp();
      return 0;
    }
    if (arg == "--asm" || arg == "--assemble") {
      return handleAssembly(argc, argv);
    }
    if (arg == "--version" || arg == "--ver") {
      printVersion();
      return 0;
    }
    std::cerr << "Unknown command. Use --help for usage information.\n";
    return 1;
  } else {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
  }
}
