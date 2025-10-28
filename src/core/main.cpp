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
#include "src/core/Core.h"
#include "src/mainwindow/MainWindow.h"
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>
#include <string>
#include <csignal>

using Core::AssemblyResult;
using Core::Msg;

void printHelp() {
  std::cout << Core::programName.toStdString() << "\n"
            << "Usage: MotorolaEmulator.exe [options]\n\n"
            << "Options:\n"
            << "  --help, -h                      Show this help message\n"
            << "  --version, --ver                Show version information\n"
            << "  --asm, --assemble               Assemble a file\n"
            << "    --input, --in <file>          Input assembly file (required)\n"
            << "    --output, --out <file>        Output binary file (default: assembled_M6800.bin)\n"
            << "    --processor, --proc <ver>     Processor version (M6800 or M6803, default: M6800)\n"
            << "Running without arguments launches the GUI mode.\n";
}

void printVersion() {
  std::cout << "Current version: " << Core::softwareVersion.toStdString() << std::endl;
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
  Core::ProcessorVersion processorVersion = Core::ProcessorVersion::M6800;
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
          processorVersion = Core::ProcessorVersion::M6803;
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
    std::cout << message.message.toStdString() << std::endl;
  }
  if (!status.error.ok) {
    if (status.error.errorLineNum != -1) {
      std::cerr << "Error: (line:" << status.error.errorLineNum << ") ";
    }
    std::cerr << "Error:" << status.error.message.toStdString();
    std::cerr << "Assembly failed.\n";
    return 1;
  }

  if (outputFile.empty()) {
    outputFile = "assembled_" + (processorVersion == Core::ProcessorVersion::M6803 ? std::string("M6803") : std::string("M6800")) + ".bin";
  }
  if (!writeOutputFile(outputFile, memory)) {
    return 1;
  }

  std::cout << "Assembly completed successfully. Output written to " << outputFile << std::endl;
  return 0;
}

#ifdef __linux__
#include <execinfo.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#endif

// Common crash reporting function
void writeCrashReport(const std::string& message) {
  std::ofstream crashFile("crashreport.txt", std::ios::app);
  if (crashFile) {
    std::time_t now = std::time(nullptr);
    crashFile << "[" << std::ctime(&now) << "] " << message << "\n";

// Add stack trace if available
#ifdef __linux__
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    for (int i = 0; i < frames; ++i) {
      crashFile << strs[i] << "\n";
    }
    free(strs);
#elif defined(_WIN32)
    void* callstack[128];
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, nullptr, TRUE);
    WORD frames = CaptureStackBackTrace(0, 128, callstack, nullptr);
    SYMBOL_INFO* symbol = static_cast<SYMBOL_INFO*>(calloc(1, sizeof(SYMBOL_INFO) + 256 * sizeof(char)));
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    for (WORD i = 0; i < frames; ++i) {
      SymFromAddr(process, reinterpret_cast<DWORD64>(callstack[i]), nullptr, symbol);
      crashFile << symbol->Name << "\n";
    }
    free(symbol);
#endif

    crashFile << "\n";
  }
}

#ifdef __linux__
void segfaultHandler(int signal) {
  writeCrashReport("Segmentation fault (signal " + std::to_string(signal) + ")");
  std::exit(1);
}
#elif defined(_WIN32)
LONG WINAPI exceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
  writeCrashReport("Critical exception (code " + std::to_string(pExceptionInfo->ExceptionRecord->ExceptionCode) + ")");
  return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main(int argc, char* argv[]) {
// Register platform-specific crash handlers
#ifdef __linux__
  std::signal(SIGSEGV, segfaultHandler);
#elif defined(_WIN32)
  SetUnhandledExceptionFilter(exceptionHandler);
#endif
  Core::initialize();
  try {
    // CLI argument handling
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
      //GUI mode
      QApplication a(argc, argv);
      MainWindow w;
      w.show();
      return a.exec();
    }
  } catch (const std::exception& ex) {
    writeCrashReport(std::string("Exception: ") + ex.what());
    return 1;
  } catch (...) {
    writeCrashReport("Unknown exception occurred");
    return 1;
  }
}
