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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt includes
#include <QLineEdit>
#include <QMainWindow>
#include <QFutureWatcher>
#include <QTreeWidget>

// Project includes
#include "src/core/Core.h"
#include "src/dialogs/ExternalDisplay.h"
#include "src/processor/Processor.h"

// Using declarations
using Core::AddressingMode;
using Core::bit;
using Core::ColorType;
using Core::MnemonicInfo;
using Core::Msg;
using Core::MsgType;
using Core::ProcessorVersion;
using Core::ProcessorVersion::M6800;
using Core::ProcessorVersion::M6803;

QT_BEGIN_NAMESPACE
namespace Ui {
  class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

protected:
  void resizeEvent(QResizeEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *ev) override;

private:
  // UI Components
  Ui::MainWindow *ui;
  ExternalDisplay *externalDisplay;
  QPlainTextEdit *plainTextDisplay;

  // Core components
  ProcessorVersion processorVersion = M6800;
  Processor processor = Processor(processorVersion);
  AssemblyMap assemblyMap;
  QString sessionId;

  // UI Setup Methods
  void setupExternalDisplay();
  void setupMemoryTable();
  void setupSimpleMemory();
  void setupOPCTree();
  void setupIndicators();
  void setupScrollbarConnections();
  void setupEventFilters();
  void setupCodeEditor();
  void setupProcessorConnections();
  void setupMemoryCornerWidget();
  void setupMenus();
  void applyInitialSettings();

  // Assembly and Memory Management
  bool startAssembly();
  bool startDisassembly();
  void updateMemoryTab();
  void colorMemory(int address, ColorType colorType);
  void setCurrentInstructionMarker(int address);
  void setAssemblyErrorMarker(int charNum, int lineNum);
  int inputNextAddress(int curAdr, QString err);
  void disableCellChangedHandler();
  void enableCellChangedHandler();

  // Display and Drawing
  void drawOPC();
  void drawProcessor();
  void processDisplayInputs(QPlainTextEdit *display);
  QString getDisplayText(std::array<uint8_t, 0x10000> &memory);
  void SetMainDisplayVisibility(bool visible);

  // Code Marker System
  QVector<int> markedAddressList;
  void drawTextMarkers();
  void drawMemoryMarkers();
  void clearCodeMarkers();
  void clearMarkers();
  void toggleCodeMarker(int line);
  void updateLinesBox(bool redraw);

  // File Operations
  void newFile();
  void openFile();
  void saveFile();
  void saveAsFile();
  void exit();
  void loadMemory();
  void saveMemory();

  enum class WritingMode {
    MEMORY,
    CODE,
  };

  // Utility Methods
  void PrintConsole(const QString &text, MsgType type = MsgType::NONE);
  void Err(const QString &text);
  void setWritingMode(WritingMode mode);
  void setAllowWritingModeSwitch(bool allow);
  void setAssemblyStatus(bool isAssembled);
  void resetEmulator();

  // State Variables
  bool errorDisplayed = false;
  bool assembled = false;
  Core::MemoryDisplayMode memoryDisplayMode = Core::MemoryDisplayMode::FULL;
  WritingMode writingMode = WritingMode::CODE;
  bool hexReg = true;
  bool assembleOnRun = true;

  int displayStatusIndex;
  bool displayPossible = false;

  float OPS = 1;
  uint16_t currentSMScroll = 0;
  int autoScrollUpLimit = 20;
  int autoScrollDownLimit = 5;
  int previousScrollCode = 0;

  QString fileSavePath = "";

  // Style definitions
  QString redButton = "QPushButton{\n" "    color: rgb(0,0,0);\n" "    background-color: rgb(225,225,225);\n" "    border: 2px solid rgb(255,30,30);\n" "}\n" "QPushButton:hover{\n" "    background-color: rgb(229, 241, 251);\n" "    border: 2px solid rgb(255, 0, 50);\n" "}\n" "QPushButton:pressed{\n" "    background-color: rgb(204, 228, 247);\n" "    border: 2px solid rgb(255, 0, 50);\n" "}";

  QString greenButton = "QPushButton{\n" "    color: rgb(0,0,0);\n" "    background-color: rgb(225,225,225);\n" "    border: 2px solid rgb(0,180,0);\n" "}\n" "QPushButton:hover{\n" "    background-color: rgb(229, 241, 251);\n" "    border: 2px solid rgb(0, 180, 20);\n" "}\n" "QPushButton:pressed{\n" "    background-color: rgb(204, 228, 247);\n" "    border: 2px solid rgb(0, 180, 20);\n" "}";

  // Menu Actions
  QAction *loadMemoryAction;
  QAction *saveMemoryAction;

  // Context Menu and Info Display
  void showContextMenu(const QPoint &);
  void showMnemonicInfo(int cursorPos);
  void showInstructionInfoWindow(QString instruction);

private slots:

  // Processor Event Handlers
  void drawProcessorRunning(std::array<uint8_t, 0x10000>, int curCycle, uint8_t flags, uint16_t PC, uint16_t SP, uint8_t aReg, uint8_t bReg, uint16_t xReg, bool useCycles, uint64_t opertaionsSinceStart);
  void onExecutionStopped();

  // Scroll Handlers
  void handleCodeVerticalScrollBarValueChanged(int value);
  void handleLinesScroll();
  void handleDisplayScrollVertical();
  void handleDisplayScrollHorizontal();

  // Button Bar Handlers
  bool on_buttonAssemble_clicked();
  void on_menuVersionSelector_currentIndexChanged(int index);
  void on_buttonLoad_clicked();
  void on_buttonSave_clicked();
  void on_buttonReset_clicked();
  void on_buttonStep_clicked();
  void on_buttonRunStop_clicked();
  void on_menuSpeedSelector_activated();
  void on_buttonSwitchWrite_clicked();

  // Settings Handlers
  void on_checkUseCycles_clicked(bool checked);
  void on_checkIncrementPC_clicked(bool checked);
  void on_checkIRQOnKeyPress_clicked(bool checked);
  void on_checkAssembleOnRun_clicked(bool checked);
  void on_checkAdvancedInfo_clicked(bool checked);
  void on_checkMemoryWrite_clicked(bool checked);
  void on_checkShowHex_clicked(bool checked);
  void on_menuScrollLow_valueChanged(int arg1);
  void on_menuScrollHigh_valueChanged(int arg1);
  void on_spinBoxTabWidth_valueChanged(int arg1);
  void on_buttonTidyUp_clicked();
  void on_menuDisplayStatus_currentIndexChanged(int index);

  // Call Interrupt Handlers
  void on_buttonRST_clicked();
  void on_buttonNMI_clicked();
  void on_pushButtonIRQ_clicked();

  // Debug Tools Handlers
  void on_lineEditBin_textChanged(const QString &arg1);
  void on_lineEditHex_textChanged(const QString &arg1);
  void on_lineEditDec_textChanged(const QString &arg1);

  // Breakpoint Handlers
  void on_menuBreakWhen_currentIndexChanged(int index);
  void on_menuBreakAt_valueChanged(int arg1);
  void on_menuBreakIs_valueChanged(int arg1);

  // Memory Interaction Handlers
  void on_memoryAddressSpinBox_valueChanged(int arg1);
  void on_simpleMemoryAddressSpinBox_valueChanged(int arg1);
  void tableMemory_cellChanged(int row, int column);
  void on_line2COMIN_valueChanged(int i);

  // Misc
  void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item);
  void on_plainTextCode_textChanged();
  void on_checkBoxBookmarkBreakpoints_clicked(bool checked);
  void on_lineASCIIconvNum_valueChanged(int arg1);
  void on_lineASCIIconvChar_textChanged(const QString &arg1);
  void on_menuMemoryDisplayMode_currentIndexChanged(int index);
};

#endif // MAINWINDOW_H
