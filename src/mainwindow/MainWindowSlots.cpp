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
#include "src/dialogs/InstructionInfoDialog.h"
#include "src/mainwindow/MainWindow.h"
#include "ui_MainWindow.h"
#include <QDir>
#include <QScrollBar>
#include <QTimer>

// Scroll Handlers
void MainWindow::handleCodeVerticalScrollBarValueChanged(int value) {
  ui->plainTextLines->verticalScrollBar()->setValue(value);
}
void MainWindow::handleLinesScroll() {
  ui->plainTextLines->verticalScrollBar()->setValue(ui->plainTextCode->verticalScrollBar()->value());
}
void MainWindow::handleDisplayScrollVertical() {
  // ui->plainTextDisplay->verticalScrollBar()->setValue(0);
}
void MainWindow::handleDisplayScrollHorizontal() {
  // ui->plainTextDisplay->horizontalScrollBar()->setValue(0);
}

// Button bar Handlers
bool MainWindow::on_buttonAssemble_clicked() {
  // ui->plainTextConsole->clear();
  if (writingMode == WritingMode::CODE) {
    return startAssembly();
  } else {
    return startDisassembly();
  }
}
void MainWindow::on_menuVersionSelector_currentIndexChanged(int index) {
  switch (index) {
  case 0:
    processorVersion = M6800;
    break;
  case 1:
    processorVersion = M6803;
    break;
  }
  processor.switchVersion(processorVersion);
  setAssemblyStatus(false);
  resetEmulator();
  drawOPC();
}

void MainWindow::on_buttonLoad_clicked() {
  if (writingMode == WritingMode::CODE) {
    openFile();
  } else {
    loadMemory();
  }
}
void MainWindow::on_buttonSave_clicked() {
  if (writingMode == WritingMode::CODE) {
    saveFile();
  } else {
    saveMemory();
  }
}

void MainWindow::on_buttonReset_clicked() {
  resetEmulator();
}
void MainWindow::on_buttonStep_clicked() {
  processor.stopExecution();

  bool ok = true;
  if (!assembled && assembleOnRun) {
    ok = on_buttonAssemble_clicked();
  }

  if (ok) {
    processor.executeStep();
    drawProcessor();
  }
}
void MainWindow::on_buttonRunStop_clicked() {
  if (processor.running) {
    processor.stopExecution();
  } else {
    bool ok = true;
    if (!assembled && assembleOnRun) {
      ok = on_buttonAssemble_clicked();
    }

    if (ok) {
      if (ui->checkAutoReset->isChecked()) {
        if (processor.Memory[processor.PC] == 0) {
          resetEmulator();
        }
      }
      if (processor.useCycles)
        ui->labelRunningCycleNum->setVisible(true);

      ui->labelRunningIndicatior->setVisible(true);
      processor.startExecution(executionNanoDelay, assemblyMap, markedAddressList);
      ui->buttonRunStop->setStyleSheet(greenButton);
    }
  }
}
void MainWindow::on_menuSpeedSelector_activated() {
  double OPS = ui->menuSpeedSelector->currentText().toDouble();
  executionNanoDelay = 1000000000 / OPS;
  ui->labelRunningIndicatior->setText("Operation/second: " + QString::number(OPS, 'f', OPS < 1 ? 1 : 0));
  processor.addAction(Action{ActionType::UPDATEPROCESSORSPEED, executionNanoDelay});
}

void MainWindow::on_buttonSwitchWrite_clicked() {
  if (writingMode == WritingMode::CODE) {
    setWritingMode(WritingMode::MEMORY);
  } else {
    setWritingMode(WritingMode::CODE);
  }
}

// Settings Handlers
void MainWindow::on_checkUseCycles_clicked(bool checked) {
  processor.stopExecution();

  processor.useCycles = checked;

  if (!checked) {
    ui->labelRunningCycleNum->setVisible(false);
  } else if (processor.running) {
    ui->labelRunningCycleNum->setVisible(true);
  }

  processor.curCycle = 0;
  processor.cycleCount = 0;
}
void MainWindow::on_checkIncrementPC_clicked(bool checked) {
  processor.addAction(Action{ActionType::SETINCONINVALIDINSTR, checked});
}
void MainWindow::on_checkIRQOnKeyPress_clicked(bool checked) {
  processor.addAction(Action{ActionType::SETIRQONKEYPRESS, checked});
}
void MainWindow::on_checkAssembleOnRun_clicked(bool checked) {
  assembleOnRun = checked;
}

void MainWindow::on_checkAdvancedInfo_clicked(bool checked) {
  if (checked) {
    ui->plainTextLines->setGeometry(ui->plainTextLines->x(), ui->plainTextLines->y(), 181, ui->plainTextLines->height());
    ui->plainTextLines->setMaximumWidth(181);
  } else {
    ui->plainTextLines->setGeometry(ui->plainTextLines->x(), ui->plainTextLines->y(), 101, ui->plainTextLines->height());
    ui->plainTextLines->setMaximumWidth(101);
  }

  updateLinesBox(true);
  drawTextMarkers();
}

void MainWindow::on_checkMemoryWrite_clicked(bool checked) {
  setAllowWritingModeSwitch(checked);
}
void MainWindow::on_checkShowHex_clicked(bool checked) {
  hexReg = checked;
  if (checked) {
    ui->menuBreakAt->setPrefix("$");
    ui->menuBreakIs->setPrefix("$");
    ui->menuBreakAt->setDisplayIntegerBase(16);
    ui->menuBreakIs->setDisplayIntegerBase(16);
    ui->memoryAddressSpinBox->setDisplayIntegerBase(16);
  } else {
    ui->menuBreakAt->setPrefix("");
    ui->menuBreakIs->setPrefix("");
    ui->menuBreakAt->setDisplayIntegerBase(10);
    ui->menuBreakIs->setDisplayIntegerBase(10);
    ui->memoryAddressSpinBox->setDisplayIntegerBase(10);
  }
  drawProcessor(); // SLABO KER DE MED RUNNINGON ACCESSO ONEJE --------------------------------------------------------------------------------------------------------------------------------------------
}

void MainWindow::on_menuScrollLow_valueChanged(int arg1) {
  autoScrollDownLimit = arg1;
}
void MainWindow::on_menuScrollHigh_valueChanged(int arg1) {
  autoScrollUpLimit = arg1;
}
void MainWindow::on_spinBoxTabWidth_valueChanged(int arg1) {
  QFontMetrics metrics(ui->plainTextCode->font());
  ui->plainTextCode->setTabStopDistance(metrics.horizontalAdvance(' ') * arg1);
}
void MainWindow::on_buttonTidyUp_clicked() {
  QStringList lines = ui->plainTextCode->toPlainText().split("\n");

  int maxLabelLength = 0;
  for (const QString &line : lines) {
    if (line.isEmpty())
      continue;
    int charNum = 0;
    if (line[charNum].isLetter()) {
      for (; charNum < line.length(); ++charNum) {
        if (line[charNum] == ' ' || line[charNum] == '\t') {
          break;
        }
      }

      maxLabelLength = std::max(maxLabelLength, charNum);
    }
  }

  int tabCount = static_cast<int>(ceil((maxLabelLength + 1.0) / ui->spinBoxTabWidth->value()));

  for (QString &line : lines) {
    if (line.isEmpty())
      continue;
    int charNum = 0;
    if (line[charNum] == ' ' || line[charNum] == '\t') {
      for (; charNum < line.length(); ++charNum) {
        if (line[charNum] != ' ' && line[charNum] != '\t') {
          break;
        }
      }

      line = QString(tabCount, '\t') + line.mid(charNum);
    } else {
      for (; charNum < line.length(); ++charNum) {
        if (line[charNum] == ' ' || line[charNum] == '\t') {
          break;
        }
      }

      int labelEnd = charNum;
      charNum++;
      for (; charNum < line.length(); ++charNum) {
        if (line[charNum] != ' ' && line[charNum] != '\t') {
          break;
        }
      }

      line = line.mid(0, labelEnd) + QString(tabCount - static_cast<int>(floor(labelEnd / ui->spinBoxTabWidth->value())), '\t') + line.mid(charNum);
    }
  }

  QString modifiedCode = lines.join("\n");
  ui->plainTextCode->setPlainText(modifiedCode);
}
void MainWindow::on_menuDisplayStatus_currentIndexChanged(
    int index) {
  if (index == 0) {
    externalDisplay->hide();
    SetMainDisplayVisibility(false);
    displayStatusIndex = 0;
  } else if (index == 1) {
    if (ui->menuDisplayStatus->count() == 3) {
      externalDisplay->hide();
      SetMainDisplayVisibility(true);
      ui->plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
      displayStatusIndex = 1;
    } else {
      SetMainDisplayVisibility(false);
      externalDisplay->show();
      displayStatusIndex = 2;
      plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
    }
  } else {
    SetMainDisplayVisibility(false);
    externalDisplay->show();
    displayStatusIndex = 2;
    plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
  }
}

// Call Interrupt Handlers
void MainWindow::on_buttonRST_clicked() {
  processor.addAction(Action{ActionType::SETRST, 0});
}
void MainWindow::on_buttonNMI_clicked() {
  processor.addAction(Action{ActionType::SETNMI, 0});
}
void MainWindow::on_pushButtonIRQ_clicked() {
  processor.addAction(Action{ActionType::SETIRQ, 0});
}

// Debug Tools Handlers
void MainWindow::on_lineEditBin_textChanged(
    const QString &arg1) {
  QString arg = arg1;
  if (arg.startsWith('%')) {
    arg = arg.mid(1);
  }
  if (arg != "X") {
    bool ok;
    int number = arg.toInt(&ok, 2);
    if (ok) {
      ui->lineEditDec->blockSignals(true);
      ui->lineEditHex->blockSignals(true);
      ui->lineEditDec->setText(QString::number(number));
      ui->lineEditHex->setText('$' + QString::number(number, 16));
      ui->lineEditDec->blockSignals(false);
      ui->lineEditHex->blockSignals(false);
    } else {
      ui->lineEditDec->setText("X");
      ui->lineEditHex->setText("X");
    }
  }
}
void MainWindow::on_lineEditHex_textChanged(
    const QString &arg1) {
  QString arg = arg1;
  if (arg.startsWith('$')) {
    arg = arg.mid(1);
  }
  if (arg != "X") {
    bool ok;
    int number = arg.toInt(&ok, 16);
    if (ok) {
      ui->lineEditDec->blockSignals(true);
      ui->lineEditBin->blockSignals(true);
      ui->lineEditDec->setText(QString::number(number));
      ui->lineEditBin->setText('%' + QString::number(number, 2));
      ui->lineEditDec->blockSignals(false);
      ui->lineEditBin->blockSignals(false);
    } else {
      ui->lineEditDec->setText("X");
      ui->lineEditBin->setText("X");
    }
  }
}
void MainWindow::on_lineEditDec_textChanged(
    const QString &arg1) {
  if (arg1 != "X") {
    bool ok;
    int number = arg1.toInt(&ok);
    if (ok) {
      ui->lineEditBin->blockSignals(true);
      ui->lineEditHex->blockSignals(true);
      ui->lineEditBin->setText('%' + QString::number(number, 2));
      ui->lineEditHex->setText('$' + QString::number(number, 16));
      ui->lineEditBin->blockSignals(false);
      ui->lineEditHex->blockSignals(false);
    } else {
      ui->lineEditBin->setText("X");
      ui->lineEditHex->setText("X");
    }
  }
}

void MainWindow::on_menuBreakWhen_currentIndexChanged(
    int index) {
  ui->menuBreakAt->setValue(0);
  ui->menuBreakIs->setValue(0);
  processor.addAction(Action{ActionType::SETBREAKWHEN, static_cast<uint32_t>(index)});

  ui->menuBreakIs->setDisplayIntegerBase(16);
  ui->menuBreakIs->setPrefix("$");
  ui->labelAt->setVisible(false);
  ui->menuBreakAt->setVisible(false);
  ui->labelIs->setVisible(true);
  ui->menuBreakIs->setVisible(true);

  switch (index) {
  case 0:
    ui->menuBreakIs->setVisible(false);
    ui->labelIs->setVisible(false);
    break;
  case 1:
    ui->menuBreakIs->setMaximum(0xFFFF);
    ui->menuBreakIs->setDisplayIntegerBase(10);
    ui->menuBreakIs->setPrefix("");
    break;
  case 2:
    ui->menuBreakIs->setMaximum(0xFFFF);
    break;
  case 3:
    ui->menuBreakIs->setMaximum(0xFFFF);
    break;
  case 4:
    ui->menuBreakIs->setMaximum(0xFFFF);
    break;
  case 5:
    ui->menuBreakIs->setMaximum(0xFF);
    break;
  case 6:
    ui->menuBreakIs->setMaximum(0xFF);
    break;
  case 7:
    ui->menuBreakIs->setMaximum(1);
    break;
  case 8:
    ui->menuBreakIs->setMaximum(1);
    break;
  case 9:
    ui->menuBreakIs->setMaximum(1);
    break;
  case 10:
    ui->menuBreakIs->setMaximum(1);
    break;
  case 11:
    ui->menuBreakIs->setMaximum(1);
    break;
  case 12:
    ui->menuBreakIs->setMaximum(1);
    break;
  case 13:
    ui->labelAt->setVisible(true);
    ui->menuBreakAt->setVisible(true);
    ui->menuBreakIs->setMaximum(0xFF);
    break;
  }
}
void MainWindow::on_menuBreakAt_valueChanged(
    int arg1) {
  processor.addAction(Action{ActionType::SETBREAKAT, static_cast<uint32_t>(arg1)});
}
void MainWindow::on_menuBreakIs_valueChanged(
    int arg1) {
  processor.addAction(Action{ActionType::SETBREAKIS, static_cast<uint32_t>(arg1)});
}

void MainWindow::on_line2COMIN_valueChanged(
    int i) {
  if (i < 0 || i > 0xFF) {
    ui->line2COMOUT->setText("X");
    return;
  }

  uint8_t byte = static_cast<uint8_t>(0xFF - i + 1);
  ui->line2COMOUT->setText("$" + QString::number(byte, 16).toUpper());
}

// Memory Interaction Handlers
void MainWindow::on_memoryAddressSpinBox_valueChanged(int arg1) {
  int row = arg1 / 16;
  QModelIndex index = ui->tableWidgetMemory->model()->index(row, 0);
  ui->tableWidgetMemory->scrollTo(index, QAbstractItemView::PositionAtTop);

  int column = arg1 % 16;
  auto targetItem = ui->tableWidgetMemory->item(row, column);
  ui->tableWidgetMemory->setCurrentItem(targetItem);
}
void MainWindow::on_simpleMemoryAddressSpinBox_valueChanged(int arg1) { // this spinBox should be capped to [0,0xFFFF]
  currentSMScroll = static_cast<uint16_t>(arg1);
  updateMemoryTab();
  drawMemoryMarkers();
}


// Misc
void MainWindow::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item) {
  InstructionInfoDialog dialog(*item, this);
  dialog.exec();
}
#define CODE_MAX_LINE_COUNT 100000
void MainWindow::on_plainTextCode_textChanged() {
  static QTimer backupTimer;

  // Limit line count
  QString text = ui->plainTextCode->toPlainText();
  if (text.count('\n') > CODE_MAX_LINE_COUNT) {
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    text = lines.mid(0, CODE_MAX_LINE_COUNT + 1).join('\n');
    ui->plainTextCode->setPlainText(text);
  }

  // Backup handling with debounce
  if (!backupTimer.isActive()) {
    backupTimer.setInterval(3000);
    backupTimer.setSingleShot(true);
    QPlainTextEdit *textEdit = ui->plainTextCode;
    connect(&backupTimer, &QTimer::timeout, this, [textEdit, this]() {
      QDir tempDir(QCoreApplication::applicationDirPath() + "/autosave");
      if (!tempDir.exists()) {
        tempDir.mkpath(".");
      }
      QString backupPath = tempDir.filePath("backup_session_" + sessionId + ".txt");
      QFile backupFile(backupPath);
      if (backupFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&backupFile);
        out << textEdit->toPlainText();
        backupFile.close();
      }
    });
  }
  backupTimer.start();

  updateLinesBox(false);
  if (errorDisplayed) {
    clearCodeMarkers();
  }
  if (writingMode == WritingMode::CODE) {
    if (assembled)
      setAssemblyStatus(false);
  }
}
void MainWindow::on_checkBoxBookmarkBreakpoints_clicked(bool checked) {
  processor.addAction(Action{ActionType::SETBOOKMARKBREAKPOINTS, static_cast<uint32_t>(checked)});
}

void MainWindow::on_lineASCIIconvNum_valueChanged(int arg1) // this lineEdit should be capped to [0,255]
{
  ui->lineASCIIconvChar->blockSignals(true);
  QChar c = Core::numToChar(static_cast<uint8_t>(arg1));
  ui->lineASCIIconvChar->setText(c.isNull() ? QStringLiteral("") : c);
  ui->lineASCIIconvChar->blockSignals(false);
}

void MainWindow::on_lineASCIIconvChar_textChanged(const QString &arg1) {
  ui->lineASCIIconvNum->blockSignals(true);
  ui->lineASCIIconvNum->setValue(arg1.length() == 0 ? 0 : Core::charToVal(arg1[0]));
  ui->lineASCIIconvNum->blockSignals(false);
}

void MainWindow::on_menuMemoryDisplayMode_currentIndexChanged(int index) {
  memoryDisplayMode = static_cast<Core::MemoryDisplayMode>(index);
  if (memoryDisplayMode == Core::MemoryDisplayMode::SIMPLE) {
    ui->groupSimpleMemory->setVisible(true);
    ui->groupSimpleMemory->setEnabled(true);
    ui->groupMemory->setVisible(false);
    ui->groupMemory->setEnabled(false);
  } else if (memoryDisplayMode == Core::MemoryDisplayMode::FULL) {
    ui->groupMemory->setVisible(true);
    ui->groupMemory->setEnabled(true);
    ui->groupSimpleMemory->setVisible(false);
    ui->groupSimpleMemory->setEnabled(false);
  } else {
    ui->groupMemory->setVisible(false);
    ui->groupMemory->setEnabled(false);
    ui->groupSimpleMemory->setVisible(false);
    ui->groupSimpleMemory->setEnabled(false);
  }
  updateMemoryTab();
  drawMemoryMarkers();
}
