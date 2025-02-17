# Runs on Win10+ no additional programs required.

The source code history is missing because the project was moved many times and I lost the changes made to it.

## Project Structure

- **examples/**: Contains various example scripts and test cases for the project.
  - **GOL.txt**: Example demonstrating the Game of Life.
  - **Interrupt example.txt**: Example of interrupt handling.
  - **memory fill.txt**: Example showing a self-replicating program.
  - **x&o.txt**: Example demonstrating Tic-Tac-Toe.

- **res/**: Contains resources like icons and Qt resource files.
  - **M6800.ico**: Icon used for the project.
  - **resources.qrc**: Qt resource file linking assets.

- **src/**: Contains the source code for the project.
  - **assembler/**: Handles assembly-related functionalities.
    - **Assembler.cpp**: Implements assembler logic.
    - **Assembler.h**
    - **Disassembler.cpp**: Implements disassembler logic.
    - **Disassembler.h**
  - **core/**: Contains the core application logic.
    - **Core.cpp**: Implements various data types and constant data.
    - **Core.h**
    - **main.cpp**: The entry point of the application.
  - **processor/**: Contains processor-related functionalities.
    - **InstructionFunctions.cpp**: Implements processor instruction functions.
    - **Processor.cpp**: Defines the processor class and operations, which contains everything related to emulation.
    - **Processor.h**
  - **dialogs/**: Contains dialog UI-related code.
    - **ExternalDisplay.cpp**: Manages external display functionality.
    - **ExternalDisplay.h**
    - **ExternalDisplay.ui**: Qt UI file for external display.
    - **InstructionInfoDialog.cpp**: Implements dialog for instruction info.
    - **InstructionInfoDialog.h**
    - **InstructionInfoDialog.ui**: Qt UI file for instruction info dialog.
  - **mainwindow/**: Contains the main window logic.
    - **FileManager.cpp**: Handles file management operations.
    - **MainWindow.cpp**: Implements the main window logic.
    - **MainWindow.h**
    - **MainWindow.ui**: Qt UI file for the main window.
    - **MainWindowSlots.cpp**: Contains slots related to the main window.
    - **SelectionSys.cpp**: Implements line selection system logic.
  - **utils/**: Contains utility functions and helper classes.
    - **ActionQueue.h**: Declares action queue class for processor interactions while emulating.

## License

You can read the full text of the license at https://www.gnu.org/licenses/agpl-3.0.html
GNU Affero General Public License v3.0 (AGPL-3.0)  
This program is licensed under the AGPL-3.0, which means that if you modify and distribute the program, you must make the source code of your modifications available under the same license.  
