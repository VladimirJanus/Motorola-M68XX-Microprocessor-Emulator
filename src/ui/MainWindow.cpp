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
#include "src/ui/MainWindow.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QScrollBar>
#include <QTimer>
#include "instructioninfodialog.h"
#include "src/assembler/Assembler.h"
#include "src/assembler/Disassembler.h"
#include "ui_mainwindow.h"
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow) {
  // UI Setup
  ui->setupUi(this);
  QWidget::setWindowTitle(Core::programName);
  ui->plainTextInfo->append("Current version: " + Core::softwareVersion + ", designed for Windows 10.");

  sessionId = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");

  // Initialization
  assemblyMap.clear();

  // External Display
  externalDisplay = new ExternalDisplay(this);
  plainTextDisplay = externalDisplay->findChild<QPlainTextEdit *>("plainTextDisplay");
  connect(externalDisplay, &QDialog::finished, this, [=]() { ui->menuDisplayStatus->setCurrentIndex(0); });

  // Memory Table Configuration
  const int memWidth = 28;
  const int memHeight = 20;
  const int fontSize = 9;
  const QString font = "Lucida Console";

  ui->tableWidgetMemory->horizontalHeader()->setMinimumSectionSize(memWidth);
  ui->tableWidgetMemory->verticalHeader()->setMinimumSectionSize(memHeight);
  ui->tableWidgetMemory->horizontalHeader()->setMaximumSectionSize(memWidth);
  ui->tableWidgetMemory->verticalHeader()->setMaximumSectionSize(memHeight);
  ui->tableWidgetMemory->horizontalHeader()->setDefaultSectionSize(memWidth);
  ui->tableWidgetMemory->verticalHeader()->setDefaultSectionSize(memHeight);

  for (int row = 0; row <= 0xFFF; ++row) {
    QString address = QString("%1").arg(row * 16, 4, 16, QChar('0')).toUpper();
    ui->tableWidgetMemory->insertRow(row);

    QTableWidgetItem *headerItem = new QTableWidgetItem(address);
    headerItem->setTextAlignment(Qt::AlignCenter);
    headerItem->setBackground(QBrush(QColor(210, 210, 255)));
    QFont headerFont(font, fontSize, QFont::Bold);
    headerItem->setFont(headerFont);
    ui->tableWidgetMemory->setVerticalHeaderItem(row, headerItem);

    for (int col = 0; col < ui->tableWidgetMemory->columnCount(); ++col) {
      QTableWidgetItem *item = new QTableWidgetItem("00");
      item->setBackground(QBrush(Core::memoryCellDefaultColor));
      item->setTextAlignment(Qt::AlignCenter);
      QFont cellFont(font, fontSize, QFont::Bold);
      item->setFont(cellFont);
      ui->tableWidgetMemory->setItem(row, col, item);
    }
  }

  for (int col = 0; col < ui->tableWidgetMemory->columnCount(); ++col) {
    QTableWidgetItem *columnHeaderItem = ui->tableWidgetMemory->horizontalHeaderItem(col);
    columnHeaderItem->setBackground(QBrush(QColor(210, 210, 255)));
    columnHeaderItem->setTextAlignment(Qt::AlignCenter);
    QFont columnHeaderFont(font, fontSize, QFont::Bold);
    columnHeaderItem->setFont(columnHeaderFont);
  }

  ui->tableWidgetMemory->setTextElideMode(Qt::ElideNone);

  // Disable Cell Interaction
  for (int row = 0; row < ui->tableWidgetMemory->rowCount(); row++) {
    for (int col = 0; col < ui->tableWidgetMemory->columnCount(); col++) {
      QTableWidgetItem *item = ui->tableWidgetMemory->item(row, col);
      item->setFlags(item->flags() & ~Qt::ItemIsEditable);
      item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
    }
  }

  // Simple Memory Initialization
  ui->groupSimpleMemory->setVisible(false);
  ui->groupSimpleMemory->setEnabled(false);

  for (uint8_t i = 0; i < 20; ++i) {
    QTableWidgetItem *item = new QTableWidgetItem(QString("%1").arg(i, 4, 16, QChar('0')).toUpper());
    item->setTextAlignment(Qt::AlignCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setBackground(QBrush(Core::SMMemoryCellColor));
    ui->tableWidgetSM->setItem(i, 0, item);

    item = new QTableWidgetItem(QString("%1").arg(processor.Memory[i], 2, 16, QChar('0').toUpper()));
    item->setTextAlignment(Qt::AlignCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setBackground(QBrush(Core::SMMemoryCellColor2));
    ui->tableWidgetSM->setItem(i, 1, item);

    item = new QTableWidgetItem("");
    item->setTextAlignment(Qt::AlignCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setBackground(QBrush(Core::SMMemoryCellColor));
    ui->tableWidgetSM->setItem(i, 2, item);
  }

  // OPC Configuration
  ui->treeWidget->sortByColumn(0, Qt::AscendingOrder);

  for (int col = 0; col < ui->treeWidget->columnCount(); ++col) {
    if (col == 0) {
      ui->treeWidget->setColumnWidth(col, 90);
    } else if (col == 1) {
      ui->treeWidget->setColumnWidth(col, 200);
    } else if (col == 8) {
      ui->treeWidget->setColumnWidth(col, 50);
    } else if (col == 9 || col == 10) {
      ui->treeWidget->setColumnWidth(col, 60);
    } else if (col == ui->treeWidget->columnCount() - 1) {
      ui->treeWidget->setColumnWidth(col, 1000);
    } else {
      ui->treeWidget->setColumnWidth(col, 30);
    }
  }

  // Indicator and Button Visibility
  ui->labelRunningIndicatior->setVisible(false);
  ui->labelRunningIndicatior->setText("Operation/second: " + QString::number(std::pow(2, ui->menuSpeedSelector->currentIndex())));
  ui->labelRunningCycleNum->setVisible(false);
  ui->labelRunningCycleNum->setText("Instruction cycle: ");
  ui->buttonSwitchWrite->setVisible(false);
  ui->labelWritingMode->setVisible(false);
  ui->buttonSwitchWrite->setEnabled(false);
  ui->labelWritingMode->setEnabled(false);
  ui->labelAt->setVisible(false);
  ui->menuBreakAt->setVisible(false);

  // Scrollbar Connections
  connect(ui->plainTextCode->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::handleCodeVerticalScrollBarValueChanged);
  connect(ui->plainTextLines->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::handleLinesScroll);
  connect(ui->plainTextDisplay->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::handleDisplayScrollVertical);
  connect(ui->plainTextDisplay->horizontalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::handleDisplayScrollHorizontal);

  // Event Filters
  ui->plainTextDisplay->installEventFilter(this);
  ui->plainTextLines->installEventFilter(this);
  plainTextDisplay->installEventFilter(this);
  ui->tableWidgetMemory->installEventFilter(this);

  // Code Editor Settings
  ui->plainTextCode->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->plainTextCode, &QPlainTextEdit::customContextMenuRequested, this, &MainWindow::showContextMenu);
  ui->plainTextCode->setUndoRedoEnabled(true);
  ui->plainTextCode->moveCursor(QTextCursor::End);

  QFontMetrics metrics(ui->plainTextCode->font());
  ui->plainTextCode->setTabStopDistance(metrics.horizontalAdvance(' ') * ui->spinBoxTabWidth->value());

  // Processor Connections
  connect(&processor, &Processor::uiUpdateData, this, &MainWindow::drawProcessorRunning);
  connect(&processor, &Processor::executionStopped, this, &MainWindow::onExecutionStopped);

  // Memory corner widget
  ui->memoryAddressSpinBox->setMaximum(0xFFFF);
  ui->memoryAddressSpinBox->setMinimum(0);
  ui->memoryAddressSpinBox->setSingleStep(1);
  ui->memoryAddressSpinBox->setDisplayIntegerBase(16);

  // Init settings
  on_checkShowHex_clicked(ui->checkShowHex->isChecked());
  on_menuSpeedSelector_activated();
  on_menuDisplayStatus_currentIndexChanged(0);
  on_menuVersionSelector_currentIndexChanged(ui->menuVersionSelector->currentIndex());
  on_checkAdvancedInfo_clicked(ui->checkAdvancedInfo->isChecked());
  on_menuBreakWhen_currentIndexChanged(ui->menuBreakWhen->currentIndex());
  enableCellChangedHandler();
}

MainWindow::~MainWindow() {
  processor.stopExecution();
  QCoreApplication::processEvents();
  delete ui;
}

void MainWindow::drawOPC() {
  ui->treeWidget->clear();

  for (MnemonicInfo info : mnemonics) {
    bool isMnemonicSupported = (info.supportedVersions & processorVersion) != 0;
    if (!isMnemonicSupported)
      continue;

    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(0, info.mnemonic);
    item->setText(1, info.shortDescription);

    /*   if (!isMnemonicSupported) {
      for (int col = 0; col < ui->treeWidget->columnCount(); ++col) {
        item->setForeground(col, QBrush(Qt::red));
      }
    }*/

    QStringList bytesList, cyclesList;
    for (int i = 0; i < info.opCodes.length(); ++i) {
      if (info.opCodes[i] != 0) {
        bool isOpcodeSupported = getInstructionSupported(processorVersion, info.opCodes[i]);
        if (!isOpcodeSupported)
          continue;
        QString hexOpcode = QString::number(info.opCodes[i], 16).toUpper().rightJustified(2, '0');

        item->setText(2 + i, hexOpcode);
        bytesList.append(QString::number(getInstructionLength(processorVersion, info.opCodes[i])));
        cyclesList.append(QString::number(getInstructionCycleCount(processorVersion, info.opCodes[i])));

        /*if (!isOpcodeSupported) {
          item->setForeground(2 + i, QBrush(Qt::red));
        }*/
      }
    }
    item->setText(8, info.flags);
    item->setText(9, bytesList.join(","));
    item->setText(10, cyclesList.join(","));
    item->setText(11, info.longDescription);
    ui->treeWidget->addTopLevelItem(item);
    for (auto aliasIt = alliasMap.begin(); aliasIt != alliasMap.end(); ++aliasIt) {
      if (aliasIt.value().mnemonic == info.mnemonic) {
        if (!(aliasIt.value().supportedVersions & processorVersion)) {
          continue;
        }
        QTreeWidgetItem *aliasItem = new QTreeWidgetItem();
        aliasItem->setText(0, aliasIt.key());
        aliasItem->setText(1, aliasIt.value().shortDescription);
        item->addChild(aliasItem);
      }
    }
    QColor backgroundColor = (ui->treeWidget->topLevelItemCount() % 2 == 0) ? QColor(230, 230, 230) : QColor(240, 240, 240);
    for (int col = 0; col < ui->treeWidget->columnCount(); ++col) {
      item->setBackground(col, QBrush(backgroundColor));
    }
  }
}

void MainWindow::showContextMenu(const QPoint &pos) {
  QMenu *menu = ui->plainTextCode->createStandardContextMenu();
  menu->addSeparator();

  QVariant cursorPosVariant(pos);
  QAction *action = menu->addAction(tr("Mnemonic info"));
  action->setData(cursorPosVariant);

  connect(action, SIGNAL(triggered()), this, SLOT(showMnemonicInfo()));

  menu->exec(ui->plainTextCode->mapToGlobal(pos));
  menu->deleteLater();
}
void MainWindow::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item) {
  InstructionInfoDialog dialog(*item, this);
  dialog.exec();
}
void MainWindow::showMnemonicInfo() {
  QAction *action = qobject_cast<QAction *>(sender());
  if (action) {
    QVariant cursorPosVariant = action->data();
    QPoint cursorPos = cursorPosVariant.toPoint();
    QString plainText = ui->plainTextCode->toPlainText();
    int index = ui->plainTextCode->cursorForPosition(cursorPos).position();
    int rightIndex = index;
    while (rightIndex < plainText.length() && !plainText[rightIndex].isSpace()) {
      rightIndex++;
    }

    int leftIndex = index;
    while (leftIndex >= 0 && !plainText[leftIndex].isSpace()) {
      leftIndex--;
    }

    QString selectedWord = (plainText.mid(leftIndex + 1, rightIndex - leftIndex - 1)).toUpper();

    if (Core::isMnemonic(selectedWord)) {
      showInstructionInfoWindow(selectedWord);
    }
  }
}
void MainWindow::showInstructionInfoWindow(QString instruction) {
  QTreeWidgetItem item_;
  for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
    QTreeWidgetItem *item = ui->treeWidget->topLevelItem(i);
    if (item && item->text(0) == instruction) {
      item_ = *item;
      break;
    }
  }

  InstructionInfoDialog dialog(item_, this);
  dialog.exec();
}

inline QString getDisplayText(std::array<uint8_t, 0x10000> memory) {
  QString text;
  for (uint16_t i = 0; i < 20 * 54; ++i) {
    uint8_t val = memory[i + 0xFB00];
    if (val <= 32 || val >= 127) {
      text.append(" ");
    } else {
      text.append(QChar(val));
    }
    if ((i + 1) % 54 == 0)
      text.append("\n");
  }
  return text;
}

void MainWindow::setCompileStatus(bool isCompiled) {
  if (isCompiled) {
    ui->buttonCompile->setStyleSheet(compiledButton);
    assembled = true;
  } else {
    assembled = false;
    ui->buttonCompile->setStyleSheet(uncompiledButton);
    PrintConsole("");
    assemblyMap.clear();
    clearSelections();
    ui->plainTextLines->verticalScrollBar()->setValue(ui->plainTextCode->verticalScrollBar()->value());
  }
  updateLinesBox(true);
  updateMemoryTab();
  drawMemorySelections();
  drawTextSelections();
  setSelectionRuntime(processor.PC);
}

void MainWindow::updateLinesBox(bool redraw) {
  QString code = ui->plainTextCode->toPlainText();
  QString text;
  int rowCount = code.count("\n") + 1;
  if (redraw) {
    if (ui->checkAdvancedInfo->isChecked()) {
      for (int i = 0; i < rowCount; i++) {
        const AssemblyMap::MappedInstr &instr = assemblyMap.getObjectByLine(i);
        if (instr.address == -1) {
          text += QString("%1:----\n").arg(i, 5, 10, QChar('0'));
        } else {
          int length = getInstructionLength(processorVersion, instr.byte1);
          QString advancedString = QString("%1:%2").arg(i, 5, 10, QChar('0')).arg(instr.address, 4, 16, QChar('0'));
          if (length > 0)
            advancedString += ":" + QString("%1").arg(instr.byte1, 2, 16, QChar('0'));
          if (length > 1)
            advancedString += ":" + QString("%1").arg(instr.byte2, 2, 16, QChar('0'));
          if (length > 2)
            advancedString += ":" + QString("%1").arg(instr.byte3, 2, 16, QChar('0'));
          text += advancedString + "\n";
        }
      }
    } else {
      for (int i = 0; i < rowCount; i++) {
        if (assemblyMap.getObjectByLine(i).address == -1) {
          text += QString("%1:----\n").arg(i, 5, 10, QChar('0'));
        } else {
          text += QString("%1:%2\n").arg(i, 5, 10, QChar('0')).arg(assemblyMap.getObjectByLine(i).address, 4, 16, QChar('0'));
        }
      }
    }
  } else {
    int previousRowCount = ui->plainTextLines->toPlainText().count("\n");
    if (rowCount > previousRowCount) {
      text = ui->plainTextLines->toPlainText();
      for (int i = previousRowCount; i < rowCount; i++) {
        text += QString("%1:----\n").arg(i, 5, 10, QChar('0'));
      }
    } else if (rowCount < previousRowCount) {
      QStringList list = ui->plainTextLines->toPlainText().split('\n');
      list = list.mid(0, rowCount);
      text = list.join('\n').append('\n');
    } else {
      return;
    }
  }
  ui->plainTextLines->setPlainText(text);
  ui->plainTextLines->verticalScrollBar()->setValue(ui->plainTextCode->verticalScrollBar()->value());
}
void MainWindow::updateMemoryTab() {
  if (simpleMemory) {
    for (int i = 0; i < 20; ++i) {
      ui->tableWidgetSM->item(i, 2)->setText("");
    }
    for (int i = 0; i < 20; ++i) {
      uint16_t adr = static_cast<uint16_t>(std::clamp(currentSMScroll + i, 0, 0xFFFF));

      ui->tableWidgetSM->item(i, 0)->setText(QString("%1").arg(adr, 4, 16, QChar('0')).toUpper());
      ui->tableWidgetSM->item(i, 1)->setText(QString("%1").arg(processor.Memory[adr], 2, 16, QChar('0')).toUpper());

      if (assembled) {
        auto instruction = assemblyMap.getObjectByAddress(adr);

        if (instruction.lineNumber == -1) {
          continue;
        }

        int length = getInstructionLength(processorVersion, instruction.byte1);

        if (length == 0) {
          continue;
        }

        ui->tableWidgetSM->item(i, 2)->setText(instruction.IN);

        if (length > 1 && i + 1 < 20) {
          ui->tableWidgetSM->item(i + 1, 2)->setText(QString("%1").arg(instruction.byte2, 2, 16, QChar('0')).toUpper());
        }
        if (length > 2 && i + 2 < 20) {
          ui->tableWidgetSM->item(i + 2, 2)->setText(QString("%1").arg(instruction.byte3, 2, 16, QChar('0')).toUpper());
        }
      }
    }
  } else {
    disableCellChangedHandler();
    for (int row = 0; row < ui->tableWidgetMemory->rowCount(); ++row) {
      for (int col = 0; col < ui->tableWidgetMemory->columnCount(); ++col) {
        uint16_t adr = static_cast<uint16_t>(row * 16 + col);
        int value = processor.Memory[adr];

        if (hexReg) {
          ui->tableWidgetMemory->item(row, col)->setText(QString("%1").arg(value, 2, 16, QChar('0')).toUpper());
        } else {
          ui->tableWidgetMemory->item(row, col)->setText(QString("%1").arg(value));
        }
      }
    }
    enableCellChangedHandler();
  }
}

void MainWindow::PrintConsole(const QString &text, MsgType type) {
  QString consoleText;
  if (type == MsgType::DEBUG) {
    consoleText = "DEBUG: " + text;
  } else if (type == MsgType::ERROR) {
    consoleText = "ERROR: " + text;
  } else if (type == MsgType::WARN) {
    consoleText = "WARN: " + text;
  } else {
    consoleText = text;
  }

  ui->plainTextConsole->appendPlainText(consoleText);
}

void MainWindow::handleCodeVerticalScrollBarValueChanged(int value) {
  ui->plainTextLines->verticalScrollBar()->setValue(value);
}
int displayPossible = 0;
void MainWindow::handleResize(QSize newSize) {
  if (newSize.width() >= 1785) {
    if (ui->menuDisplayStatus->count() == 2) {
      ui->menuDisplayStatus->insertItem(1, "Main Window");
    }
    if (!displayPossible) {
      ui->menuDisplayStatus->setCurrentIndex(1);
      displayPossible = true;
    }
  } else {
    displayPossible = false;
    if (ui->menuDisplayStatus->currentIndex() == 1) {
      ui->menuDisplayStatus->setCurrentIndex(0);
    }
    if (ui->menuDisplayStatus->count() == 3) {
      ui->menuDisplayStatus->model()->removeRow(1);
    }
  }
}
void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  handleResize(this->size());
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
  if (!writeToMemory) {
    if (assembled)
      setCompileStatus(false);
  }
}
bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
  if (obj == ui->plainTextLines) {
    if (event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
      if (mouseEvent) {
        if (assembled) {
          if (mouseEvent->button() == Qt::LeftButton) {
            QPoint mousePos = mouseEvent->pos();
            QTextCursor cursor = ui->plainTextLines->cursorForPosition(mousePos);
            if (!cursor.atEnd()) {
              int lineNumber = cursor.blockNumber();
              int totalLines = ui->plainTextLines->document()->blockCount();
              if (lineNumber >= 0 && lineNumber < totalLines) {
                toggleHighlight(lineNumber);
              }
            }
          } else if (mouseEvent->button() == Qt::RightButton) {
            clearHighlights();
          }
        }
      }
    } else if (event->type() == QEvent::Wheel || event->type() == QEvent::Scroll || event->type() == QEvent::User || event->type() == QEvent::KeyPress) {
      return true;
    }
  } else if (obj == plainTextDisplay || obj == ui->plainTextDisplay) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
      if (keyEvent->key() <= Qt::Key_AsciiTilde) {
        uint8_t asciiValue = static_cast<uint8_t>(keyEvent->key());
        processor.addAction(Action{ActionType::SETKEY, asciiValue});
      }

      return true;
    } else if (event->type() == QMouseEvent::MouseButtonPress || event->type() == QMouseEvent::MouseButtonDblClick) {
      QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
      if (mouseEvent->button() == Qt::LeftButton) {
        processor.addAction(Action{ActionType::SETMOUSECLICK, 1});
      } else if (mouseEvent->button() == Qt::RightButton) {
        processor.addAction(Action{ActionType::SETMOUSECLICK, 2});
      } else if (mouseEvent->button() == Qt::MiddleButton) {
        processor.addAction(Action{ActionType::SETMOUSECLICK, 3});
      }

      return true;
    }
  } else if (obj == ui->tableWidgetMemory) {
    if (writeToMemory) {
      if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Delete) {
          auto selectedItems = ui->tableWidgetMemory->selectedItems();
          if (!selectedItems.isEmpty()) {
            for (QTableWidgetItem *item : selectedItems) {
              uint16_t adr = static_cast<uint16_t>(item->row() * 16 + item->column());
              processor.Memory[adr] = 0x00;
              if (adr >= 0xFB00 && adr <= 0xFF37) {
                if (displayStatusIndex == 1) {
                  ui->plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
                } else if (displayStatusIndex == 2) {
                  plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
                }
              }
            }
            updateMemoryTab();
          }
        }
      }
    }
  }

  return QMainWindow::eventFilter(obj, event);
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

void MainWindow::resetEmulator() {
  processor.reset();
  drawProcessor();
  ui->plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
  plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
}

void MainWindow::drawProcessor() {
  ui->lineEditHValue->setText(QString::number(bit(processor.flags, 5)));
  ui->lineEditIValue->setText(QString::number(bit(processor.flags, 4)));
  ui->lineEditNValue->setText(QString::number(bit(processor.flags, 3)));
  ui->lineEditZValue->setText(QString::number(bit(processor.flags, 2)));
  ui->lineEditVValue->setText(QString::number(bit(processor.flags, 1)));
  ui->lineEditCValue->setText(QString::number(bit(processor.flags, 0)));
  if (hexReg) {
    ui->lineEditPCValue->setText(QString("%1").arg(processor.PC, 4, 16, QLatin1Char('0')).toUpper());
    ui->lineEditSPValue->setText(QString("%1").arg(processor.SP, 4, 16, QLatin1Char('0')).toUpper());
    ui->lineEditAValue->setText(QString("%1").arg(processor.aReg, 2, 16, QLatin1Char('0')).toUpper());
    ui->lineEditBValue->setText(QString("%1").arg(processor.bReg, 2, 16, QLatin1Char('0')).toUpper());
    ui->lineEditXValue->setText(QString("%1").arg(processor.xReg, 4, 16, QLatin1Char('0')).toUpper());
  } else {
    ui->lineEditPCValue->setText(QString::number(processor.PC));
    ui->lineEditSPValue->setText(QString::number(processor.SP));
    ui->lineEditAValue->setText(QString::number(processor.aReg));
    ui->lineEditBValue->setText(QString::number(processor.bReg));
    ui->lineEditXValue->setText(QString::number(processor.xReg));
  }
  if (processor.useCycles) {
    ui->labelRunningCycleNum->setText("Instruction cycle: " + QString::number(processor.curCycle));
  }
  int lineNum = assemblyMap.getObjectByAddress(processor.PC).lineNumber;
  if (ui->checkAutoScroll->isChecked()) {
    if (lineNum >= 0) {
      if (lineNum > previousScrollCode + autoScrollUpLimit) {
        previousScrollCode = lineNum - autoScrollUpLimit;
        ui->plainTextLines->verticalScrollBar()->setValue(previousScrollCode);
        ui->plainTextCode->verticalScrollBar()->setValue(previousScrollCode);
      } else if (lineNum < previousScrollCode + autoScrollDownLimit) {
        previousScrollCode = lineNum - autoScrollDownLimit;
        ui->plainTextLines->verticalScrollBar()->setValue(previousScrollCode);
        ui->plainTextCode->verticalScrollBar()->setValue(previousScrollCode);
      }
    }
  }

  if (displayStatusIndex == 1) {
    ui->plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
  } else if (displayStatusIndex == 2) {
    plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
  }
  updateMemoryTab();
  setSelectionRuntime(processor.PC);
}
void MainWindow::drawProcessorRunning(
  std::array<uint8_t, 0x10000> memory, int curCycle, uint8_t flags, uint16_t PC, uint16_t SP, uint8_t aReg, uint8_t bReg, uint16_t xReg, bool useCycles) {
  ui->lineEditHValue->setText(QString::number(bit(flags, 5)));
  ui->lineEditIValue->setText(QString::number(bit(flags, 4)));
  ui->lineEditNValue->setText(QString::number(bit(flags, 3)));
  ui->lineEditZValue->setText(QString::number(bit(flags, 2)));
  ui->lineEditVValue->setText(QString::number(bit(flags, 1)));
  ui->lineEditCValue->setText(QString::number(bit(flags, 0)));
  if (hexReg) {
    ui->lineEditPCValue->setText(QString("%1").arg(PC, 4, 16, QLatin1Char('0')).toUpper());
    ui->lineEditSPValue->setText(QString("%1").arg(SP, 4, 16, QLatin1Char('0')).toUpper());
    ui->lineEditAValue->setText(QString("%1").arg(aReg, 2, 16, QLatin1Char('0')).toUpper());
    ui->lineEditBValue->setText(QString("%1").arg(bReg, 2, 16, QLatin1Char('0')).toUpper());
    ui->lineEditXValue->setText(QString("%1").arg(xReg, 4, 16, QLatin1Char('0')).toUpper());
  } else {
    ui->lineEditPCValue->setText(QString::number(PC));
    ui->lineEditSPValue->setText(QString::number(SP));
    ui->lineEditAValue->setText(QString::number(aReg));
    ui->lineEditBValue->setText(QString::number(bReg));
    ui->lineEditXValue->setText(QString::number(xReg));
  }
  if (useCycles) {
    ui->labelRunningCycleNum->setText("Instruction cycle: " + QString::number(curCycle));
  }
  if (ui->checkAutoScroll->isChecked()) {
    int lineNum = assemblyMap.getObjectByAddress(PC).lineNumber;
    if (lineNum >= 0) {
      if (lineNum > previousScrollCode + autoScrollUpLimit) {
        previousScrollCode = lineNum - autoScrollUpLimit;
        ui->plainTextLines->verticalScrollBar()->setValue(previousScrollCode);
        ui->plainTextCode->verticalScrollBar()->setValue(previousScrollCode);
      } else if (lineNum < previousScrollCode + autoScrollDownLimit) {
        previousScrollCode = lineNum - autoScrollDownLimit;
        ui->plainTextLines->verticalScrollBar()->setValue(previousScrollCode);
        ui->plainTextCode->verticalScrollBar()->setValue(previousScrollCode);
      }
    }
  }
  if (!simpleMemory) {
    int firstVisibleRow = ui->tableWidgetMemory->rowAt(0);
    int lastVisibleRow = std::ceil(ui->tableWidgetMemory->rowAt(ui->tableWidgetMemory->viewport()->height() - 1));
    disableCellChangedHandler();
    for (int row = firstVisibleRow; row <= lastVisibleRow; ++row) {
      for (int col = 0; col < ui->tableWidgetMemory->columnCount(); ++col) {
        if (hexReg) {
          ui->tableWidgetMemory->item(row, col)->setText(QString("%1").arg(memory[static_cast<uint16_t>(row * 16 + col)], 2, 16, QChar('0')).toUpper());
        } else {
          ui->tableWidgetMemory->item(row, col)->setText(QString("%1").arg(memory[static_cast<uint16_t>(row * 16 + col)]));
        }
      }
    }
    enableCellChangedHandler();
  } else {
    for (int i = 0; i < 20; ++i) {
      ui->tableWidgetSM->item(i, 0)->setText(QString("%1").arg(currentSMScroll + i, 4, 16, QChar('0')).toUpper());

      ui->tableWidgetSM->item(i, 1)->setText(QString("%1").arg(memory[static_cast<uint16_t>(std::clamp(currentSMScroll + i, 0, 0xFFFF))], 2, 16, QChar('0').toUpper()));
    }
  }
  if (displayStatusIndex == 1) {
    ui->plainTextDisplay->setPlainText(getDisplayText(memory));
    if (ui->plainTextDisplay->hasFocus()) {
      QPoint position = QCursor::pos();
      QPoint localMousePos = ui->plainTextDisplay->mapFromGlobal(position);
      localMousePos.setX(localMousePos.x() - 3);
      localMousePos.setY(localMousePos.y() - 5);
      QFontMetrics fontMetrics(ui->plainTextDisplay->font());
      int charWidth = fontMetrics.averageCharWidth();
      int charHeight = fontMetrics.height();
      int x = localMousePos.x() / charWidth;
      int y = localMousePos.y() / charHeight;
      x = std::clamp(x, 0, 53);
      y = std::clamp(y, 0, 19);
      processor.Memory[0xFFF2] = static_cast<uint8_t>(x);
      processor.Memory[0xFFF3] = static_cast<uint8_t>(y);
    }
  } else if (displayStatusIndex == 2) {
    plainTextDisplay->setPlainText(getDisplayText(memory));
    if (plainTextDisplay->hasFocus()) {
      QPoint position = QCursor::pos();
      QPoint localMousePos = plainTextDisplay->mapFromGlobal(position);
      localMousePos.setX(localMousePos.x() - 3);
      localMousePos.setY(localMousePos.y() - 5);
      QFontMetrics fontMetrics(plainTextDisplay->font());
      int charWidth = fontMetrics.averageCharWidth();
      int charHeight = fontMetrics.height();
      int x = localMousePos.x() / charWidth;
      int y = localMousePos.y() / charHeight;
      x = std::clamp(x, 0, 53);
      y = std::clamp(y, 0, 19);
      processor.Memory[0xFFF2] = static_cast<uint8_t>(x);
      processor.Memory[0xFFF3] = static_cast<uint8_t>(y);
    }
  }
  setSelectionRuntime(PC);
}

void MainWindow::onExecutionStopped() {
  //endTime = std::chrono::high_resolution_clock::now();
  //qDebug() << static_cast<qint64>(std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
  //qDebug() << static_cast<qint64>(processor.count);
  //qDebug() << static_cast<qint64>(std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count()) - processor.count;
  //processor.count = 0;
  drawProcessor();
  ui->labelRunningIndicatior->setVisible(false);
  ui->labelRunningCycleNum->setVisible(false);
}
bool MainWindow::startAssembly() {
  processor.stopExecution();
  std::fill(std::begin(processor.Memory), std::end(processor.Memory), static_cast<uint8_t>(0));
  std::fill(std::begin(processor.backupMemory), std::end(processor.backupMemory), static_cast<uint8_t>(0));
  assemblyMap.clear();
  AssemblyResult assResult;
  try {
    QString code = ui->plainTextCode->toPlainText();
    assResult = Assembler::assemble(processorVersion, code, processor.Memory);
  } catch (const std::exception &e) {
    PrintConsole("Critical error in assembler: " + QString(e.what()), MsgType::ERROR);
    ui->tabWidget->setCurrentIndex(0);
    setCompileStatus(false);
    resetEmulator();
    return false;
  } catch (...) {
    PrintConsole("Unknown critical error in assembler", MsgType::ERROR);
    ui->tabWidget->setCurrentIndex(0);
    setCompileStatus(false);
    resetEmulator();
    return false;
  }

  foreach (Msg message, assResult.messages) {
    PrintConsole(message.message, message.type);
  }

  if (!assResult.error.ok) {
    PrintConsole(assResult.error.message, MsgType::ERROR);

    setCompileStatus(false);
    resetEmulator();
    setSelectionCompileError(assResult.error.errorCharNum, assResult.error.errorLineNum);
    ui->tabWidget->setCurrentIndex(0);

    return false;
  } else {
    assemblyMap = assResult.assemblyMap;

    std::memcpy(processor.backupMemory.data(), processor.Memory.data(), processor.Memory.size() * sizeof(uint8_t));
    setCompileStatus(true);
    PrintConsole("\nAssembly Successful.");
    resetEmulator();
    return true;
  }
}
bool MainWindow::startDisassembly() {
  processor.stopExecution();
  bool ORGOK;
  QString text = QInputDialog::getText(this,
                                       "Input Dialog",
                                       "Enter the decimal address where the program beggins. Data before that will be written with .BYTE.",
                                       QLineEdit::Normal,
                                       QString(),
                                       &ORGOK);
  if (!ORGOK) {
    PrintConsole("Invalid address", MsgType::ERROR);
    return false;
  }
  bool ORGNUMOK;
  int number = text.toInt(&ORGNUMOK);
  if (!ORGNUMOK || number < 0 || number > 0xFFFF) {
    PrintConsole("Invalid address", MsgType::ERROR);
    return false;
  }

  DisassemblyResult disassResult = Disassembler::disassemble(processorVersion, number, 0xFFFF, processor.Memory);

  ui->plainTextCode->setPlainText(disassResult.code);
  assemblyMap = disassResult.assemblyMap;

  setCompileStatus(true);
  resetEmulator();
  foreach (Msg message, disassResult.messages) {
    PrintConsole(message.message, message.type);
  }
  return true;
}
bool MainWindow::on_buttonCompile_clicked() {
  ui->plainTextConsole->clear();
  if (!writeToMemory) {
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
  setCompileStatus(false);
  resetEmulator();
  drawOPC();
}
void MainWindow::on_buttonLoad_clicked() {
  if (!writeToMemory) {
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Text Files (*.txt);;All Files (*)"));

    if (!filePath.isEmpty()) {
      QFile file(filePath);
      if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        ui->plainTextCode->setPlainText(in.readAll());
        file.close();
      } else {
        PrintConsole("Error loading script", MsgType::ERROR);
      }
    }
  } else {
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Binary Files (*.bin);;All Files (*)"));

    if (!filePath.isEmpty()) {
      QFile file(filePath);
      if (file.open(QIODevice::ReadOnly)) {
        QByteArray byteArray = file.readAll();
        if (byteArray.size() == sizeof(processor.Memory)) {
          processor.stopExecution();
          std::memcpy(processor.Memory.data(), byteArray.constData(), sizeof(processor.Memory));
          std::memcpy(processor.backupMemory.data(), processor.Memory.data(), processor.Memory.size() * sizeof(uint8_t));
          setCompileStatus(false);

        } else {
          PrintConsole("Error: File size doesn't match Memory size", MsgType::ERROR);
        }

        file.close();
      } else {
        PrintConsole("Error loading memory", MsgType::ERROR);
      }
    }
  }
}
void MainWindow::on_buttonSave_clicked() {
  if (!writeToMemory) {
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("Text Files (*.txt);;All Files (*)"));
    if (!filePath.isEmpty()) {
      QFile file(filePath);
      if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << ui->plainTextCode->toPlainText();
        file.close();
      } else {
        PrintConsole("Error saving script", MsgType::ERROR);
      }
    }
  } else {
    processor.stopExecution();
    QByteArray byteArray(reinterpret_cast<const char *>(processor.Memory.data()), processor.Memory.size());
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("Binary Files (*.bin);;All Files (*)"));

    if (!filePath.isEmpty()) {
      QFile file(filePath);
      if (file.open(QIODevice::WriteOnly)) {
        file.write(byteArray);
        file.close();
      } else {
        PrintConsole("Error saving memory", MsgType::ERROR);
      }
    }
  }
}
void MainWindow::on_buttonReset_clicked() {
  resetEmulator();
}
void MainWindow::on_buttonStep_clicked() {
  processor.stopExecution();

  bool ok = true;
  if (!assembled && compileOnRun) {
    ok = on_buttonCompile_clicked();
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
    if (!assembled && compileOnRun) {
      ok = on_buttonCompile_clicked();
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
    }
  }
}
void MainWindow::on_menuSpeedSelector_activated() {
  processor.stopExecution();

  OPS = ui->menuSpeedSelector->currentText().toFloat();
  //qDebug()<< "OPS:"+QString::number(OPS) + "WAIT:"+QString::number(executionSpeed) + " BATCHSIZE:" + QString::number(stepSkipCount);
  ui->labelRunningIndicatior->setText("Operation/second: " + QString::number(OPS, 'f', OPS < 1 ? 1 : 0));
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
void MainWindow::on_checkCompileOnRun_clicked(bool checked) {
  compileOnRun = checked;
}
void MainWindow::on_menuScrollLow_valueChanged(int arg1) {
  autoScrollDownLimit = arg1;
}
void MainWindow::on_menuScrollHigh_valueChanged(int arg1) {
  autoScrollUpLimit = arg1;
}
void MainWindow::on_checkMemoryWrite_clicked(bool checked) {
  if (checked) {
    ui->buttonSwitchWrite->setVisible(true);
    ui->labelWritingMode->setVisible(true);
    ui->buttonSwitchWrite->setEnabled(true);
    ui->labelWritingMode->setEnabled(true);
  } else {
    ui->buttonSwitchWrite->setVisible(false);
    ui->labelWritingMode->setVisible(false);
    ui->buttonSwitchWrite->setEnabled(false);
    ui->labelWritingMode->setEnabled(false);
    writeToMemory = false;
    ui->plainTextCode->setReadOnly(false);
    ui->checkCompileOnRun->setEnabled(true);
    compileOnRun = true;
    ui->labelWritingMode->setText("Code");
    ui->buttonLoad->setText("Load Code");
    ui->buttonSave->setText("Save Code");
    ui->buttonCompile->setText("Assemble");
    for (int row = 0; row < ui->tableWidgetMemory->rowCount(); row++) {
      for (int col = 0; col < ui->tableWidgetMemory->columnCount(); col++) {
        QTableWidgetItem *item = ui->tableWidgetMemory->item(row, col);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
      }
    }
  }
}
void MainWindow::on_buttonSwitchWrite_clicked() {
  if (writeToMemory) {
    writeToMemory = false;
    ui->plainTextCode->setReadOnly(false);
    ui->checkCompileOnRun->setEnabled(true);
    compileOnRun = true;
    ui->labelWritingMode->setText("Code");
    ui->buttonLoad->setText("Load Code");
    ui->buttonSave->setText("Save Code");
    ui->buttonCompile->setText("Assemble");
    for (int row = 0; row < ui->tableWidgetMemory->rowCount(); row++) {
      for (int col = 0; col < ui->tableWidgetMemory->columnCount(); col++) {
        QTableWidgetItem *item = ui->tableWidgetMemory->item(row, col);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
      }
    }
  } else {
    writeToMemory = true;
    ui->plainTextCode->setReadOnly(true);
    ui->checkCompileOnRun->setEnabled(false);
    compileOnRun = false;
    ui->labelWritingMode->setText("Memory");

    ui->buttonLoad->setText("Load Memory");
    ui->buttonSave->setText("Save Memory");
    ui->buttonCompile->setText("Disassemble");
    for (int row = 0; row < ui->tableWidgetMemory->rowCount(); row++) {
      for (int col = 0; col < ui->tableWidgetMemory->columnCount(); col++) {
        QTableWidgetItem *item = ui->tableWidgetMemory->item(row, col);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
      }
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
    ui->menuBreakIs->setMaximum(65535);
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
void MainWindow::on_simpleMemoryAddressSpinBox_valueChanged(int arg1) {
  currentSMScroll = arg1;
  updateMemoryTab();
  drawMemorySelections();
}
void MainWindow::on_memoryAddressSpinBox_valueChanged(int arg1) {
  int row = arg1 / 16;
  QModelIndex index = ui->tableWidgetMemory->model()->index(row, 0);
  ui->tableWidgetMemory->scrollTo(index, QAbstractItemView::PositionAtTop);

  int column = arg1 % 16;
  auto targetItem = ui->tableWidgetMemory->item(row, column);
  ui->tableWidgetMemory->setCurrentItem(targetItem);
}
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

  int tabCount = static_cast<int>(ceil((maxLabelLength + 1.0F) / ui->spinBoxTabWidth->value()));

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
void MainWindow::on_buttonRST_clicked() {
  processor.addAction(Action{ActionType::SETRST, 0});
}
void MainWindow::on_buttonNMI_clicked() {
  processor.addAction(Action{ActionType::SETNMI, 0});
}
void MainWindow::on_pushButtonIRQ_clicked() {
  processor.addAction(Action{ActionType::SETIRQ, 0});
}

void MainWindow::on_checkIncrementPC_clicked(bool checked) {
  processor.addAction(Action{ActionType::SETINCONUNKNOWN, checked});
}
void MainWindow::disableCellChangedHandler() {
  disconnect(ui->tableWidgetMemory, &QTableWidget::cellChanged, this, &MainWindow::tableMemory_cellChanged);
}
void MainWindow::enableCellChangedHandler() {
  connect(ui->tableWidgetMemory, &QTableWidget::cellChanged, this, &MainWindow::tableMemory_cellChanged);
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
    if (!ok) {
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
          setCompileStatus(false);
        }
        std::memcpy(processor.backupMemory.data(), processor.Memory.data(), processor.Memory.size() * sizeof(uint8_t));
      } else {
        processor.addAction(Action{ActionType::SETMEMORY, static_cast<uint32_t>(adr | (value << 16))});
      }
    }
  }
  ui->tableWidgetMemory->blockSignals(false);
}

void MainWindow::on_checkIRQOnKeyPress_clicked(bool checked) {
  processor.addAction(Action{ActionType::SETIRQONKEYPRESS, checked});
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
