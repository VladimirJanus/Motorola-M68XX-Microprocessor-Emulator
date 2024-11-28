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

#include <QLineEdit>
#include <QMainWindow>
#include "externaldisplay.h"
#include "src/processor/Processor.h"
#include <qfuturewatcher.h>
#include <qtreewidget.h>

using DataTypes::AddressingMode;
using DataTypes::bit;
using DataTypes::ColorType;
using DataTypes::MnemonicInfo;
using DataTypes::Msg;
using DataTypes::MsgType;
using DataTypes::ProcessorVersion;
using DataTypes::ProcessorVersion::M6800;
using DataTypes::ProcessorVersion::M6803;
/*#define FUNCTION_CALL_COUNT() \
  static std::unordered_map<std::string, size_t> function_call_counts; \
  std::string current_function = __FUNCTION__; \
  function_call_counts[current_function]++; \
  qDebug() << current_function << "has been called" << function_call_counts[current_function] << "times.";
*/
QT_BEGIN_NAMESPACE
namespace Ui {
  class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  const QColor memoryCellDefaultColor = QColor(230, 230, 255);
  const QColor SMMemoryCellColor = QColor(150, 150, 150);
  const QColor SMMemoryCellColor2 = QColor(204, 204, 204);

private:
  Ui::MainWindow *ui;

  ExternalDisplay *externalDisplay;
  DataTypes::AssemblyMap assemblyMap;
  QPlainTextEdit *plainTextDisplay;

  Processor processor;
  ProcessorVersion processorVersion = M6800;

  void drawOPC();
  bool startAssembly();
  bool startDisassembly();
  bool disassemble(ProcessorVersion ver, int begLoc);
  void handleResize(QSize size);

  void updateMemoryTab();
  void updateLinesBox(bool redraw);

  void colorMemory(int address, ColorType colorType);
  void toggleHighlight(int line);
  void drawTextSelections();
  void drawMemorySelections();
  void clearSelections();
  void clearHighlights();
  void setSelectionRuntime(int address);
  void setSelectionCompileError(int charNum, int lineNum);

  void drawProcessor();

  void PrintConsole(const QString &text, MsgType type = MsgType::NONE);
  void Err(const QString &text);

  int displayStatusIndex;

  float OPS = 1;

  void resetEmulator();

  bool assembled = false;
  void setCompileStatus(bool isCompile);
  QString uncompiledButton = "QPushButton{\n	color: rgb(0,0,0);\n	background-color: rgb(225,225,225);\n	border: 2px solid rgb(255,30,30);\n}\nQPushButton:hover{\n    "
                             "background-color: rgb(229, 241, 251);\n    border: 2px solid rgb(255, 0, 50);\n}\nQPushButton:pressed{\n background-color: "
                             "rgb(204, 228, 247);\n border: 2px solid rgb(255, 0, 50);\n}";
  QString compiledButton = "QPushButton{\n	color: rgb(0,0,0);\n	background-color: rgb(225,225,225);\n	border: 2px solid rgb(0,180,0);\n}\nQPushButton:hover{\n    "
                           "background-color: rgb(229, 241, 251);\n    border: 2px solid rgb(0, 180, 20);\n}\nQPushButton:pressed{\n background-color: rgb(204, "
                           "228, 247);\n border: 2px solid rgb(0, 180, 20);\n}";
  int inputNextAddress(int curAdr, QString err);

  bool simpleMemory = false;
  uint16_t currentSMScroll = 0;
  bool writeToMemory = false;

  bool hexReg = true;
  bool compileOnRun = true;
  int autoScrollUpLimit = 20;
  int autoScrollDownLimit = 5;
  int previousScrollCode = 0;

  void disableCellChangedHandler();
  void enableCellChangedHandler();
  void tableMemory_cellChanged(int row, int column);

protected:
  void resizeEvent(QResizeEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *ev) override;
public slots:
  void showContextMenu(const QPoint &);
  void showMnemonicInfo();
  void showInstructionInfoWindow(QString instruction);
  void drawProcessorRunning(std::array<uint8_t, 0x10000>, int curCycle, uint8_t flags, uint16_t PC, uint16_t SP, uint8_t aReg, uint8_t bReg, uint16_t xReg, bool useCycles);
  void onExecutionStopped();
private slots:
  void handleCodeVerticalScrollBarValueChanged(int value);
  void handleLinesScroll();
  void handleDisplayScrollVertical();
  void handleDisplayScrollHorizontal();

  void on_memoryAddressSpinBox_valueChanged(int arg1);
  bool on_buttonCompile_clicked();
  void on_buttonLoad_clicked();
  void on_buttonNMI_clicked();
  void on_buttonReset_clicked();
  void on_buttonRST_clicked();
  void on_buttonRunStop_clicked();
  void on_buttonSave_clicked();
  void on_buttonStep_clicked();
  void on_buttonSwitchWrite_clicked();
  void on_buttonTidyUp_clicked();
  void on_checkAdvancedInfo_clicked(bool checked);
  void on_checkUseCycles_clicked(bool checked);
  void on_checkIncrementPC_clicked(bool checked);
  void on_checkIRQOnKeyPress_clicked(bool checked);
  void on_checkSimpleMemory_clicked(bool checked);
  void on_checkCompileOnRun_clicked(bool checked);
  void on_checkShowHex_clicked(bool checked);
  void on_checkMemoryWrite_clicked(bool checked);
  void on_menuBreakWhen_currentIndexChanged(int index);
  void on_menuDisplayStatus_currentIndexChanged(int index);
  void on_menuSpeedSelector_activated();
  void on_menuVersionSelector_currentIndexChanged(int index);
  void on_lineEditBin_textChanged(const QString &arg1);
  void on_lineEditDec_textChanged(const QString &arg1);
  void on_lineEditHex_textChanged(const QString &arg1);
  void on_plainTextCode_textChanged();
  void on_pushButtonIRQ_clicked();
  void on_simpleMemoryAddressSpinBox_valueChanged(int arg1);
  void on_menuBreakAt_valueChanged(int arg1);
  void on_menuBreakIs_valueChanged(int arg1);
  void on_menuScrollLow_valueChanged(int arg1);
  void on_spinBoxTabWidth_valueChanged(int arg1);
  void on_menuScrollHigh_valueChanged(int arg1);
  void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item);
  void on_line2COMIN_valueChanged(int arg1);
};
#endif // MAINWINDOW_H
