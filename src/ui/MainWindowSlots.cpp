#include "mainwindow.h"
#include <QDir>
#include <QTimer>
#include <QScrollBar>
#include "src/ui/InstructionInfoDialog.h"
#include "ui_MainWindow.h"

// Scroll Handlers
void MainWindow::handleCodeVerticalScrollBarValueChanged(int value) {
  ui->plainTextLines->verticalScrollBar()->setValue(value);
}
void MainWindow::handleLinesScroll() {
  ui->plainTextLines->verticalScrollBar()->setValue(ui->plainTextCode->verticalScrollBar()->value());
}
void MainWindow::handleDisplayScrollVertical() {
  ui->plainTextDisplay->verticalScrollBar()->setValue(0);
}
void MainWindow::handleDisplayScrollHorizontal() {
  ui->plainTextDisplay->horizontalScrollBar()->setValue(0);
}

// Button bar Handlers
bool MainWindow::on_buttonAssemble_clicked() {
  ui->plainTextConsole->clear();
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
      processor.startExecution(OPS, assemblyMap);
      ui->buttonRunStop->setStyleSheet(greenButton);
    }
  }
}
void MainWindow::on_menuSpeedSelector_activated() {
  processor.stopExecution();

  OPS = ui->menuSpeedSelector->currentText().toFloat();
  ui->labelRunningIndicatior->setText("Operation/second: " + QString::number(OPS, 'f', OPS < 1 ? 1 : 0));
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

  processor.addAction(Action{ActionType::SETUSECYCLES, checked});

  if (!checked) {
    ui->labelRunningCycleNum->setVisible(false);
  } else if (processor.running) {
    ui->labelRunningCycleNum->setVisible(true);
  }

  processor.curCycle = 0;
  processor.cycleCount = 0;
}
void MainWindow::on_checkIncrementPC_clicked(bool checked) {
  processor.addAction(Action{ActionType::SETINCONUNKNOWN, checked});
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
  drawTextSelections();
  handleResize(this->size());
}
void MainWindow::on_checkSimpleMemory_clicked(bool checked) {
  simpleMemory = checked;
  if (checked) {
    ui->groupSimpleMemory->setVisible(true);
    ui->groupSimpleMemory->setEnabled(true);
    ui->groupMemory->setVisible(false);
    ui->groupMemory->setEnabled(false);
  } else {
    ui->groupMemory->setVisible(true);
    ui->groupMemory->setEnabled(true);
    ui->groupSimpleMemory->setVisible(false);
    ui->groupSimpleMemory->setEnabled(false);
  }
  updateMemoryTab();
  drawMemorySelections();
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
  drawProcessor(); //NOROST KER DE MED RUNNINGON ACCESSO ONEJE --------------------------------------------------------------------------------------------------------------------------------------------
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

      float labelEnd = charNum;
      charNum++;
      for (; charNum < line.length(); ++charNum) {
        if (line[charNum] != ' ' && line[charNum] != '\t') {
          break;
        }
      }

      line = line.mid(0, labelEnd) + QString(tabCount - floor(labelEnd / ui->spinBoxTabWidth->value()), '\t') + line.mid(charNum);
    }
  }

  QString modifiedCode = lines.join("\n");
  ui->plainTextCode->setPlainText(modifiedCode);
}
void MainWindow::on_menuDisplayStatus_currentIndexChanged(int index) {
  if (index == 0) {
    externalDisplay->hide();
    ui->plainTextDisplay->setEnabled(false);
    ui->plainTextDisplay->setVisible(false);
    ui->frameDisplay->setEnabled(false);
    ui->frameDisplay->setVisible(false);
    displayStatusIndex = 0;
  } else if (index == 1) {
    if (ui->menuDisplayStatus->count() == 3) {
      externalDisplay->hide();
      ui->plainTextDisplay->setEnabled(true);
      ui->plainTextDisplay->setVisible(true);
      ui->frameDisplay->setEnabled(true);
      ui->frameDisplay->setVisible(true);
      ui->plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
      displayStatusIndex = 1;
    } else {
      ui->plainTextDisplay->setEnabled(false);
      ui->plainTextDisplay->setVisible(false);
      ui->frameDisplay->setEnabled(false);
      ui->frameDisplay->setVisible(false);
      externalDisplay->show();
      displayStatusIndex = 2;
      plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
    }
  } else {
    ui->plainTextDisplay->setEnabled(false);
    ui->plainTextDisplay->setVisible(false);
    ui->frameDisplay->setEnabled(false);
    ui->frameDisplay->setVisible(false);
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
void MainWindow::on_lineEditBin_textChanged(const QString &arg1) {
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
void MainWindow::on_lineEditHex_textChanged(const QString &arg1) {
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
void MainWindow::on_lineEditDec_textChanged(const QString &arg1) {
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

void MainWindow::on_menuBreakWhen_currentIndexChanged(int index) {
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
void MainWindow::on_menuBreakAt_valueChanged(int arg1) {
  processor.addAction(Action{ActionType::SETBREAKAT, static_cast<uint32_t>(arg1)});
}
void MainWindow::on_menuBreakIs_valueChanged(int arg1) {
  processor.addAction(Action{ActionType::SETBREAKIS, static_cast<uint32_t>(arg1)});
}

void MainWindow::on_line2COMIN_valueChanged(int i) {
  if (i <= 0xFF && i >= 0) {
    uint8_t byte = static_cast<uint8_t>(i);
    byte = 0xFF - byte + 1;
    ui->line2COMOUT->setText("$" + QString::number(byte, 16).toUpper());
  } else {
    ui->line2COMOUT->setText("X");
  }
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
void MainWindow::on_simpleMemoryAddressSpinBox_valueChanged(int arg1) {
  currentSMScroll = arg1;
  updateMemoryTab();
  drawMemorySelections();
}

void MainWindow::tableMemory_cellChanged(int row, int column) {
  ui->tableWidgetMemory->blockSignals(true);
  QTableWidgetItem *item = ui->tableWidgetMemory->item(row, column);
  if (item == ui->tableWidgetMemory->currentItem() && ui->tableWidgetMemory->isPersistentEditorOpen(item)) {
    QString newText = item->text();
    bool ok;
    int value;
    if (hexReg) {
      value = newText.toInt(&ok, 16);
    } else {
      value = newText.toInt(&ok, 10);
    }
    uint16_t adr = static_cast<uint16_t>(row * 16 + column);
    if (!ok || value < 0 || value > 0xFF) {
      if (hexReg) {
        item->setText(QString("%1").arg(processor.Memory[adr], 2, 16, QChar('0')).toUpper());
      } else {
        item->setText(QString("%1").arg(processor.Memory[adr]));
      }
    } else {
      if (hexReg) {
        item->setText(QString("%1").arg(value, 2, 16, QChar('0')).toUpper());
      } else {
        item->setText(QString("%1").arg(value));
      }
      if (!processor.running) {
        processor.Memory[adr] = value;
        if (adr >= 0xFB00 && adr <= 0xFF37) {
          if (displayStatusIndex == 1) {
            ui->plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
          } else if (displayStatusIndex == 2) {
            plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
          }
        }
        if (assembled) {
          setAssemblyStatus(false);
        }
        std::memcpy(processor.backupMemory.data(), processor.Memory.data(), processor.Memory.size() * sizeof(uint8_t));
      } else {
        processor.addAction(Action{ActionType::SETMEMORY, static_cast<uint32_t>(adr | (value << 16))});
      }
    }
  }
  ui->tableWidgetMemory->blockSignals(false);
}

// Misc
void MainWindow::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item) {
  InstructionInfoDialog dialog(*item, this);
  dialog.exec();
}
void MainWindow::on_plainTextCode_textChanged() {
  static QTimer backupTimer;

  // Limit line count
  QString text = ui->plainTextCode->toPlainText();
  if (text.count('\n') > 65535) {
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    text = lines.mid(0, 65536).join('\n');
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
    clearSelections();
  }
  if (writingMode == WritingMode::CODE) {
    if (assembled)
      setAssemblyStatus(false);
  }
}
