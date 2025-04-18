﻿/*
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
#include "src/mainwindow/MainWindow.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QScrollBar>
#include <QTimer>
#include <QMessageBox>
#include "src/dialogs/InstructionInfoDialog.h"
#include "ui_MainWindow.h"
#include "src/assembler/Assembler.h"
#include "src/assembler/Disassembler.h"

MainWindow::MainWindow(
  QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow) {
  // Basic UI setup and version info display
  ui->setupUi(this);
  QWidget::setWindowTitle(Core::programName);
  ui->plainTextInfo->append("Current version: " + Core::softwareVersion + ", designed for " + Core::envName + ".");

  // Generate unique session ID
  sessionId = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
  assemblyMap.clear();

  setupExternalDisplay();
  setupMemoryTable();
  setupSimpleMemory();
  setupOPCTree();
  setupIndicators();
  setupScrollbarConnections();
  setupEventFilters();
  setupCodeEditor();
  setupProcessorConnections();
  setupMemoryCornerWidget();
  setupMenus();
  applyInitialSettings();
}

// Helper methods for component initialization

void MainWindow::setupExternalDisplay() {
  externalDisplay = new ExternalDisplay(this);
  plainTextDisplay = externalDisplay->getPlainTextEdit();
  connect(externalDisplay, &QDialog::finished, this, [=]() { ui->menuDisplayStatus->setCurrentIndex(0); });
}
void MainWindow::setupMemoryTable() {
  // Constants
  const int memCellWidth = 28;
  const int memCellHeight = 20;
  const int fontSize = 9;
  const QString font = "Lucida Console";
  QFont cellFont(font, fontSize, QFont::Bold);

  // Set table dimensions - use the same size for min/max/default
  auto *hHeader = ui->tableWidgetMemory->horizontalHeader();
  auto *vHeader = ui->tableWidgetMemory->verticalHeader();

  hHeader->setMinimumSectionSize(memCellWidth);
  hHeader->setMaximumSectionSize(memCellWidth);
  hHeader->setDefaultSectionSize(memCellWidth);

  vHeader->setMinimumSectionSize(memCellHeight);
  vHeader->setMaximumSectionSize(memCellHeight);
  vHeader->setDefaultSectionSize(memCellHeight);

  // Style column headers
  for (int col = 0; col < ui->tableWidgetMemory->columnCount(); ++col) {
    QTableWidgetItem *header = ui->tableWidgetMemory->horizontalHeaderItem(col);
    header->setBackground(QBrush(QColor(210, 210, 255)));
    header->setTextAlignment(Qt::AlignCenter);
    header->setFont(cellFont);
  }

  // Initialize memory cells (rows and columns)
  for (int row = 0; row <= 0xFFF; ++row) {
    ui->tableWidgetMemory->insertRow(row);

    // Set row header (address)
    QString address = QString("%1").arg(row * 16, 4, 16, QChar('0')).toUpper();
    QTableWidgetItem *rowHeader = new QTableWidgetItem(address);
    rowHeader->setTextAlignment(Qt::AlignCenter);
    rowHeader->setBackground(QBrush(QColor(210, 210, 255)));
    rowHeader->setFont(cellFont);
    ui->tableWidgetMemory->setVerticalHeaderItem(row, rowHeader);

    // Initialize memory cells in this row
    for (int col = 0; col < ui->tableWidgetMemory->columnCount(); ++col) {
      QTableWidgetItem *cell = new QTableWidgetItem("00");
      cell->setBackground(QBrush(Core::memoryCellDefaultColor));
      cell->setTextAlignment(Qt::AlignCenter);
      cell->setFont(cellFont);
      cell->setFlags(cell->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsUserCheckable));
      ui->tableWidgetMemory->setItem(row, col, cell);
    }
  }

  ui->tableWidgetMemory->setTextElideMode(Qt::ElideNone);
}
void MainWindow::setupSimpleMemory() {
  ui->groupSimpleMemory->setVisible(false);
  ui->groupSimpleMemory->setEnabled(false);

  // Initialize simple memory view cells
  for (uint8_t i = 0; i < 20; ++i) {
    // Address column
    QTableWidgetItem *item = new QTableWidgetItem(QString("%1").arg(i, 4, 16, QChar('0')).toUpper());
    item->setTextAlignment(Qt::AlignCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setBackground(QBrush(Core::SMMemoryCellColor));
    ui->tableWidgetSM->setItem(i, 0, item);

    // Value column
    item = new QTableWidgetItem(QString("%1").arg(processor.Memory[i], 2, 16, QChar('0').toUpper()));
    item->setTextAlignment(Qt::AlignCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setBackground(QBrush(Core::SMMemoryCellColor2));
    ui->tableWidgetSM->setItem(i, 1, item);

    // Description column
    item = new QTableWidgetItem("");
    item->setTextAlignment(Qt::AlignCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setBackground(QBrush(Core::SMMemoryCellColor));
    ui->tableWidgetSM->setItem(i, 2, item);
  }
}
void MainWindow::setupOPCTree() {
  ui->treeWidget->sortByColumn(0, Qt::AscendingOrder);

  // Set column widths
  const QVector<int> columnWidths = {90, 200, 30, 30, 30, 30, 30, 30, 50, 60, 60, 1000};
  for (int col = 0; col < ui->treeWidget->columnCount(); ++col) {
    ui->treeWidget->setColumnWidth(col, columnWidths[col]);
  }
}
void MainWindow::setupIndicators() {
  // Configure visibility and initial state of UI indicators
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
}
void MainWindow::setupScrollbarConnections() {
  connect(ui->plainTextCode->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::handleCodeVerticalScrollBarValueChanged);
  connect(ui->plainTextLines->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::handleLinesScroll);
  connect(ui->plainTextDisplay->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::handleDisplayScrollVertical);
  connect(ui->plainTextDisplay->horizontalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::handleDisplayScrollHorizontal);
}
void MainWindow::setupEventFilters() {
  ui->plainTextDisplay->installEventFilter(this);
  ui->plainTextLines->installEventFilter(this);
  plainTextDisplay->installEventFilter(this);
  ui->tableWidgetMemory->installEventFilter(this);
}
void MainWindow::setupCodeEditor() {
  // Configure code editor properties
  ui->plainTextCode->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->plainTextCode, &QPlainTextEdit::customContextMenuRequested, this, &MainWindow::showContextMenu);
  ui->plainTextCode->setUndoRedoEnabled(true);
  ui->plainTextCode->moveCursor(QTextCursor::End);

  // Set tab width based on font metrics
  QFontMetrics metrics(ui->plainTextCode->font());
  ui->plainTextCode->setTabStopDistance(metrics.horizontalAdvance(' ') * ui->spinBoxTabWidth->value());
}
void MainWindow::setupProcessorConnections() {
  connect(&processor, &Processor::uiUpdateData, this, &MainWindow::drawProcessorRunning);
  connect(&processor, &Processor::executionStopped, this, &MainWindow::onExecutionStopped);
}
void MainWindow::setupMemoryCornerWidget() {
  ui->memoryAddressSpinBox->setMaximum(0xFFFF);
  ui->memoryAddressSpinBox->setMinimum(0);
  ui->memoryAddressSpinBox->setSingleStep(1);
  ui->memoryAddressSpinBox->setDisplayIntegerBase(16);
}
void MainWindow::setupMenus() {
  // Helper function to create and connect actions
  auto createAction = [this](QMenu *menu, const QString &name, QKeySequence shortcut, auto callback) {
    QAction *action = new QAction(name, this);
    action->setShortcut(shortcut);
    menu->addAction(action);
    connect(action, &QAction::triggered, this, callback);
    return action;
  };

  // FILE MENU
  QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
  createAction(fileMenu, tr("New"), QKeySequence(Qt::CTRL | Qt::Key_N), &MainWindow::newFile);
  createAction(fileMenu, tr("Open"), QKeySequence(Qt::CTRL | Qt::Key_O), &MainWindow::openFile);
  createAction(fileMenu, tr("Save"), QKeySequence(Qt::CTRL | Qt::Key_S), &MainWindow::saveFile);
  createAction(fileMenu, tr("Save As"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), &MainWindow::saveAsFile);
  fileMenu->addSeparator();
  createAction(fileMenu, tr("Exit"), QKeySequence(), &MainWindow::exit);

  // EDIT MENU
  QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
  createAction(editMenu, tr("Undo"), QKeySequence(Qt::CTRL | Qt::Key_Z), [this]() { ui->plainTextCode->undo(); });
  createAction(editMenu, tr("Redo"), QKeySequence(Qt::CTRL | Qt::Key_Y), [this]() { ui->plainTextCode->redo(); });
  editMenu->addSeparator();
  createAction(editMenu, tr("Cut"), QKeySequence(Qt::CTRL | Qt::Key_X), [this]() { ui->plainTextCode->cut(); });
  createAction(editMenu, tr("Copy"), QKeySequence(Qt::CTRL | Qt::Key_C), [this]() { ui->plainTextCode->copy(); });
  createAction(editMenu, tr("Paste"), QKeySequence(Qt::CTRL | Qt::Key_V), [this]() { ui->plainTextCode->paste(); });
  editMenu->addSeparator();
  createAction(editMenu, tr("Select All"), QKeySequence(Qt::CTRL | Qt::Key_A), [this]() { ui->plainTextCode->selectAll(); });
  editMenu->addSeparator();
  createAction(editMenu, tr("Mnemonic Info"), QKeySequence(Qt::CTRL | Qt::Key_I), [this]() { showMnemonicInfo(ui->plainTextCode->textCursor().position()); });

  // EMULATOR MENU
  QMenu *emulationMenu = menuBar()->addMenu(tr("&Emulator"));
  createAction(emulationMenu, tr("Assemble"), QKeySequence(Qt::Key_F9), [this]() { on_buttonAssemble_clicked(); });
  emulationMenu->addSeparator();
  createAction(emulationMenu, tr("Run/Stop"), QKeySequence(Qt::Key_F5), [this]() { on_buttonRunStop_clicked(); });
  createAction(emulationMenu, tr("Step"), QKeySequence(Qt::Key_F6), [this]() { on_buttonStep_clicked(); });
  createAction(emulationMenu, tr("Reset"), QKeySequence(Qt::Key_F7), [this]() { resetEmulator(); });

  loadMemoryAction = createAction(emulationMenu, tr("Load Memory"), QKeySequence(), &MainWindow::loadMemory);
  loadMemoryAction->setEnabled(writingMode == WritingMode::MEMORY);

  saveMemoryAction = createAction(emulationMenu, tr("Save Memory"), QKeySequence(), &MainWindow::saveMemory);
  saveMemoryAction->setEnabled(writingMode == WritingMode::MEMORY);

  emulationMenu->addSeparator();
  createAction(emulationMenu, tr("Switch Writing Mode"), QKeySequence(), [this]() {
    if (writingMode == WritingMode::MEMORY) {
      setWritingMode(WritingMode::CODE);
    } else {
      if (!ui->checkMemoryWrite->isChecked()) {
        setAllowWritingModeSwitch(true);
      }
      setWritingMode(WritingMode::MEMORY);
    }
  });

  QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
  QAction *showRegisters = createAction(viewMenu, tr("Show Registers"), QKeySequence(), [this]() {
    bool checked = ui->groupRegisters->isVisible();
    ui->groupRegisters->setEnabled(!checked);
    ui->groupRegisters->setVisible(!checked);
  });
  showRegisters->setCheckable(true);
  showRegisters->setChecked(true);

  QAction *showUtility = createAction(viewMenu, tr("Show Utility"), QKeySequence(), [this]() {
    bool checked = ui->tabWidget->isVisible();
    ui->tabWidget->setEnabled(!checked);
    ui->tabWidget->setVisible(!checked);
  });
  showUtility->setCheckable(true);
  showUtility->setChecked(true);

  viewMenu->addSeparator();
  createAction(viewMenu, tr("Switch Display Mode"), QKeySequence(), [this]() { ui->menuDisplayStatus->setCurrentIndex((ui->menuDisplayStatus->currentIndex() + 1) % ui->menuDisplayStatus->count()); });

  // ABOUT MENU
  QMenu *aboutMenu = menuBar()->addMenu(tr("&About"));
  createAction(aboutMenu, tr("Info"), QKeySequence(Qt::Key_F1), [this]() {
    QWidget *window = new QWidget(nullptr);
    window->setWindowTitle("Info");
    window->resize(600, 400);
    window->setAttribute(Qt::WA_DeleteOnClose);

    QTextEdit *textEdit = new QTextEdit(window);
    textEdit->setHtml(ui->plainTextInfo->toHtml());
    textEdit->setReadOnly(true);

    QVBoxLayout *layout = new QVBoxLayout(window);
    layout->addWidget(textEdit);
    window->setLayout(layout);

    window->setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    window->show();
  });

  createAction(aboutMenu, tr("Credits"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::ALT | Qt::Key_V), []() {
    const QString GES = "Vladimir Januš";
    const QString projectUrl = "https://github.com/VladimirJanus/Motorola-M68XX-Microprocessor-Emulator";

    QMessageBox msgBox;
    msgBox.setWindowTitle("About " + Core::programName);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowIcon(QIcon(":/images/info_icon.png"));

    QString message = QString("<b>Made by: </b>%1<br><br>"
                              "<b>Project Location: </b><a href='%2'>%2</a><br><br>"
                              "<b>Software Version: </b>%3")
                        .arg(GES, projectUrl, Core::softwareVersion);

    msgBox.setText(message);
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setDefaultButton(QMessageBox::Ok);

    msgBox.exec();
  });
}
void MainWindow::applyInitialSettings() {
  on_checkShowHex_clicked(ui->checkShowHex->isChecked());
  on_menuSpeedSelector_activated();
  on_menuDisplayStatus_currentIndexChanged(0);
  on_menuVersionSelector_currentIndexChanged(ui->menuVersionSelector->currentIndex());
  on_checkAdvancedInfo_clicked(ui->checkAdvancedInfo->isChecked());
  on_menuBreakWhen_currentIndexChanged(ui->menuBreakWhen->currentIndex());
  enableCellChangedHandler();
  setAllowWritingModeSwitch(false);
  ui->tabWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow() {
  processor.stopExecution();
  QCoreApplication::processEvents();
  delete ui;
}

void MainWindow::setAllowWritingModeSwitch(bool allow) {
  if (allow) {
    ui->checkMemoryWrite->blockSignals(true);
    ui->checkMemoryWrite->setChecked(true);
    ui->checkMemoryWrite->blockSignals(false);

    ui->buttonSwitchWrite->setVisible(true);
    ui->labelWritingMode->setVisible(true);
    ui->buttonSwitchWrite->setEnabled(true);
    ui->labelWritingMode->setEnabled(true);
  } else {
    ui->checkMemoryWrite->blockSignals(true);
    ui->checkMemoryWrite->setChecked(false);
    ui->checkMemoryWrite->blockSignals(false);

    ui->buttonSwitchWrite->setVisible(false);
    ui->labelWritingMode->setVisible(false);
    ui->buttonSwitchWrite->setEnabled(false);
    ui->labelWritingMode->setEnabled(false);

    setWritingMode(WritingMode::CODE);
  }
}
void MainWindow::setWritingMode(WritingMode mode) {
  writingMode = mode;
  if (mode == WritingMode::MEMORY) {
    ui->plainTextCode->setReadOnly(true);
    ui->checkAssembleOnRun->setEnabled(false);
    assembleOnRun = false;

    ui->labelWritingMode->setText("Memory");
    ui->buttonLoad->setText("Load Memory");
    ui->buttonSave->setText("Save Memory");
    ui->buttonAssemble->setText("Disassemble");

    for (int row = 0; row < ui->tableWidgetMemory->rowCount(); row++) {
      for (int col = 0; col < ui->tableWidgetMemory->columnCount(); col++) {
        QTableWidgetItem *item = ui->tableWidgetMemory->item(row, col);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
      }
    }
  } else {
    ui->plainTextCode->setReadOnly(false);
    ui->checkAssembleOnRun->setEnabled(true);
    assembleOnRun = ui->checkAssembleOnRun->isChecked();

    ui->labelWritingMode->setText("Code");
    ui->buttonLoad->setText("Load Code");
    ui->buttonSave->setText("Save Code");
    ui->buttonAssemble->setText("Assemble");

    for (int row = 0; row < ui->tableWidgetMemory->rowCount(); row++) {
      for (int col = 0; col < ui->tableWidgetMemory->columnCount(); col++) {
        QTableWidgetItem *item = ui->tableWidgetMemory->item(row, col);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
      }
    }
  }

  loadMemoryAction->setEnabled(mode == WritingMode::MEMORY);
  saveMemoryAction->setEnabled(mode == WritingMode::MEMORY);
}

void MainWindow::drawOPC() {
  ui->treeWidget->clear();

    for (const MnemonicInfo &info : Core::mnemonics) {
    bool isMnemonicSupported = (info.supportedVersions & processorVersion) != 0;
    if (!isMnemonicSupported)
      continue;

    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(0, info.mnemonic);
    item->setText(1, info.shortDescription);

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
      }
    }
    item->setText(8, info.flags);
    item->setText(9, bytesList.join(","));
    item->setText(10, cyclesList.join(","));
    item->setText(11, info.longDescription);
    ui->treeWidget->addTopLevelItem(item);
    for (auto aliasIt = Core::alliasMap.begin(); aliasIt != Core::alliasMap.end(); ++aliasIt) {
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

  QAction *action = menu->addAction(tr("Mnemonic Info"));
  connect(action, &QAction::triggered, this, [this, pos]() { showMnemonicInfo(ui->plainTextCode->cursorForPosition(pos).position()); });

  menu->exec(ui->plainTextCode->mapToGlobal(pos));
  menu->deleteLater();
}

void MainWindow::showMnemonicInfo(int cursorPos) {
  const QString plainText = ui->plainTextCode->toPlainText();

  if (cursorPos < 0 || cursorPos >= plainText.length()) {
    return;
  }

  int right = cursorPos;
  int left = cursorPos;
  while (right < plainText.length() && !plainText[right].isSpace())
    right++;
  while (left >= 0 && !plainText[left].isSpace())
    left--;

  QString word = plainText.mid(left + 1, right - left - 1).toUpper();
  if (Core::isMnemonic(word)) {
    showInstructionInfoWindow(word);
  }
}
void MainWindow::showInstructionInfoWindow(QString instruction) {
  QTreeWidgetItem item_;
  for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
    QTreeWidgetItem *item = ui->treeWidget->topLevelItem(i);
    if (!item)
      continue;

    if (item->text(0) == instruction) {
      item_ = *item;
      break;
    }

    // Check all children (aliases)
    for (int j = 0; j < item->childCount(); ++j) {
      QTreeWidgetItem *child = item->child(j);
      if (child && child->text(0) == instruction) {
        item_ = *child;
        break;
      }
    }
  }
  InstructionInfoDialog dialog(item_, this);
  dialog.exec();
}

QString MainWindow::getDisplayText(std::array<uint8_t, 0x10000> &memory) {
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

void MainWindow::setAssemblyStatus(bool isAssembled) {
  if (isAssembled) {
    ui->buttonAssemble->setStyleSheet(greenButton);
    assembled = true;
  } else {
    assembled = false;
    ui->buttonAssemble->setStyleSheet(redButton);
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
  ui->plainTextConsole->moveCursor(QTextCursor::End);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  if (this->size().width() >= 1785) {
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
    if (writingMode == WritingMode::MEMORY) {
      if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Delete) {
          auto selectedItems = ui->tableWidgetMemory->selectedItems();
          if (!selectedItems.isEmpty()) {
            for (QTableWidgetItem *item : selectedItems) {
              uint16_t adr = static_cast<uint16_t>(item->row() * 16 + item->column());
              processor.Memory[adr] = 0x00;
            }
            updateMemoryTab();
            if (displayStatusIndex == 1) {
              ui->plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
            } else if (displayStatusIndex == 2) {
              plainTextDisplay->setPlainText(getDisplayText(processor.Memory));
            }
          }
        }
      }
    }
  }

  return QMainWindow::eventFilter(obj, event);
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
void MainWindow::processDisplayInputs(
  QPlainTextEdit *display) {
  QPoint position = QCursor::pos();
  QPoint localMousePos = display->mapFromGlobal(position);
  localMousePos.setX(localMousePos.x() - 3);
  localMousePos.setY(localMousePos.y() - 5);
  QFontMetrics fontMetrics(display->font());
  int charWidth = fontMetrics.averageCharWidth();
  int charHeight = fontMetrics.height();
  int x = localMousePos.x() / charWidth;
  int y = localMousePos.y() / charHeight;
  x = std::clamp(x, 0, 53);
  y = std::clamp(y, 0, 19);
  processor.Memory[0xFFF2] = static_cast<uint8_t>(x);
  processor.Memory[0xFFF3] = static_cast<uint8_t>(y);
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
      processDisplayInputs(ui->plainTextDisplay);
    }
  } else if (displayStatusIndex == 2) {
    plainTextDisplay->setPlainText(getDisplayText(memory));
    if (plainTextDisplay->hasFocus()) {
      processDisplayInputs(plainTextDisplay);
    }
  }
  setSelectionRuntime(PC);
}

void MainWindow::onExecutionStopped() {
  ui->buttonRunStop->setStyleSheet(redButton);
  drawProcessor();
  ui->labelRunningIndicatior->setVisible(false);
  ui->labelRunningCycleNum->setVisible(false);
}
bool MainWindow::startAssembly() {
  processor.stopExecution();
  std::fill(std::begin(processor.Memory), std::end(processor.Memory), static_cast<uint8_t>(0));
  std::fill(std::begin(processor.backupMemory), std::end(processor.backupMemory), static_cast<uint8_t>(0));
  assemblyMap.clear();
  Core::AssemblyResult assResult;
  PrintConsole("\nStarting assembly: Time: " + QTime::currentTime().toString("hh:mm:ss") + "\n");
  try {
    QString code = ui->plainTextCode->toPlainText();
    assResult = Assembler::assemble(processorVersion, code, processor.Memory);
  } catch (const std::exception &e) {
    PrintConsole("Critical error in assembler: " + QString(e.what()), MsgType::ERROR);
    ui->tabWidget->setCurrentIndex(0);
    setAssemblyStatus(false);
    resetEmulator();
    return false;
  } catch (...) {
    PrintConsole("Unknown critical error in assembler", MsgType::ERROR);
    ui->tabWidget->setCurrentIndex(0);
    setAssemblyStatus(false);
    resetEmulator();
    return false;
  }

  foreach (Msg message, assResult.messages) {
    PrintConsole(message.message, message.type);
  }

  if (!assResult.error.ok) {
    PrintConsole(assResult.error.message, MsgType::ERROR);

    setAssemblyStatus(false);
    resetEmulator();
    setSelectionAssemblyError(assResult.error.errorCharNum, assResult.error.errorLineNum);
    ui->tabWidget->setCurrentIndex(0);

    return false;
  } else {
    assemblyMap = assResult.assemblyMap;

    std::memcpy(processor.backupMemory.data(), processor.Memory.data(), processor.Memory.size() * sizeof(uint8_t));
    setAssemblyStatus(true);
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

  setAssemblyStatus(true);
  resetEmulator();
  foreach (Msg message, disassResult.messages) {
    PrintConsole(message.message, message.type);
  }
  return true;
}

void MainWindow::disableCellChangedHandler() {
  disconnect(ui->tableWidgetMemory, &QTableWidget::cellChanged, this, &MainWindow::tableMemory_cellChanged);
}
void MainWindow::enableCellChangedHandler() {
  connect(ui->tableWidgetMemory, &QTableWidget::cellChanged, this, &MainWindow::tableMemory_cellChanged);
}
void MainWindow::SetMainDisplayVisibility(
  bool visible) {
  ui->plainTextDisplay->setEnabled(visible);
  ui->plainTextDisplay->setVisible(visible);
}
