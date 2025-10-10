Motorola M68XX Emulator
======================

A modern C++17-based emulator for the M6800 processor, built using Qt. Runs on Windows 10+ without requiring additional software.

Currently supports M6800 and M6803.

Note: The source code history is incomplete due to multiple project relocations. Some historical changes are lost.

Project Structure
-----------------

- examples/: Contains example scripts and test cases
    - GOL.txt: Example demonstrating the Game of Life
    - Interrupt example.txt: Example of interrupt handling
    - memory fill.txt: Example showing a self-replicating program
    - x&o.txt: Example demonstrating Tic-Tac-Toe

- res/: Contains resources like icons and Qt resource files
    - M6800.ico: Icon used for the project
    - resources.qrc: Qt resource file linking assets

- src/: Contains the source code for the project
    - assembler/: Handles assembly-related functionalities
        - Assembler.cpp: Implements assembler logic
        - Assembler.h
        - Disassembler.cpp: Implements disassembler logic
        - Disassembler.h
    - core/: Contains the core application logic
        - Core.cpp: Implements various data types and constant data
        - Core.h
        - main.cpp: Entry point of the application
    - processor/: Contains processor-related functionalities
        - InstructionFunctions.cpp: Implements processor instruction functions
        - Processor.cpp: Defines the processor class and operations
        - Processor.h
    - dialogs/: Contains dialog UI-related code
        - ExternalDisplay.cpp: Manages external display functionality
        - ExternalDisplay.h
        - ExternalDisplay.ui: Qt UI file for external display
        - InstructionInfoDialog.cpp: Implements dialog for instruction info
        - InstructionInfoDialog.h
        - InstructionInfoDialog.ui: Qt UI file for instruction info dialog
    - mainwindow/: Contains the main window logic
        - FileManager.cpp: Handles file management operations
        - MainWindow.cpp: Implements the main window logic
        - MainWindow.h
        - MainWindow.ui: Qt UI file for the main window
        - MainWindowSlots.cpp: Contains slots related to the main window
        - SelectionSys.cpp: Implements line selection system logic
    - utils/: Contains utility functions and helper classes
        - ActionQueue.h: Declares action queue class for processor interactions while emulating


Features
--------

- Cross-platform Qt GUI with modern C++17 support
- Emulation of the M6800 processor
- Assembly and disassembly support
- Interactive dialogs for instruction info and external display
- File management and code marking system in the main window
- Example programs included to demonstrate functionality

Examples
--------

- Game of Life (GOL.txt) – Classic cellular automaton
- Interrupt Handling (Interrupt example.txt) – Demonstrates processor interrupts
- Memory Fill (memory fill.txt) – Self-replicating program demo
- Tic-Tac-Toe (x&o.txt) – Simple game demonstration

License
-------

This project is licensed under the GNU Affero General Public License v3.0 (AGPL-3.0).

You must make the source code of any modifications publicly available under the same license.

Full license text: https://www.gnu.org/licenses/agpl-3.0.html

