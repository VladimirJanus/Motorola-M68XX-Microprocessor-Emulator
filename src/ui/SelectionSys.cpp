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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qscrollbar.h>
#include <qtextobject.h>

struct Selection {
  QTextEdit::ExtraSelection lineSelection;
  QTextEdit::ExtraSelection codeSelection;
};

int runTimeSelectionAddress = 0;

QVector<int> highlightLineList;
QVector<int> highlightAddressList;

QBrush brushHighlight(Qt::green);
QBrush brushRuntime(Qt::yellow);
QBrush brushHighlightRuntime(Qt::GlobalColor::cyan);
QBrush brushError(Qt::darkYellow);
QBrush brushErrorChar(Qt::darkGreen);
QBrush brushNone(QColor(230, 230, 255));

QBrush &getBrushForType(ColorType colorType) {
  switch (colorType) {
  case ColorType::HIGHLIGHT:
    return brushHighlight;
  case ColorType::RUNTIME:
    return brushRuntime;
  case ColorType::HIGHLIGHT_RUNTIME:
    return brushHighlightRuntime;
  case ColorType::ERROR:
    return brushError;
  case ColorType::ERRORCHAR:
    return brushErrorChar;
  case ColorType::NONE:
    return brushNone;
  }
  return brushNone;
}

void MainWindow::colorMemory(int address, ColorType colorType) {
  if (!simpleMemory) {
    ui->tableWidgetMemory->item(address / 16, address % 16)->setBackground(getBrushForType(colorType));
  } else {
    int row = address - currentSMScroll;
    if (row >= 0 && row < 20) {
      QBrush brush = getBrushForType(colorType);
      if (colorType == ColorType::NONE) {
        ui->tableWidgetSM->item(row, 0)->setBackground(QBrush(Core::SMMemoryCellColor));
        ui->tableWidgetSM->item(row, 1)->setBackground(QBrush(Core::SMMemoryCellColor2));
        ui->tableWidgetSM->item(row, 2)->setBackground(QBrush(Core::SMMemoryCellColor));
      } else {
        ui->tableWidgetSM->item(row, 0)->setBackground(brush);
        ui->tableWidgetSM->item(row, 1)->setBackground(brush);
        ui->tableWidgetSM->item(row, 2)->setBackground(brush);
      }
    }
  }
}
QTextEdit::ExtraSelection getSelectionByLine(QPlainTextEdit *textEdit, int line, ColorType colorType) {
  QTextEdit::ExtraSelection lineSelection;
  lineSelection.format.setBackground(getBrushForType(colorType));
  QTextBlock block = textEdit->document()->findBlockByLineNumber(line);
  QTextCursor cursor(textEdit->document());
  cursor.setPosition(block.position());
  cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
  lineSelection.cursor = cursor;
  return lineSelection;
}

void MainWindow::drawTextSelections() {
  errorDisplayed = false;
  QList<QTextEdit::ExtraSelection> lineSelections;
  QList<QTextEdit::ExtraSelection> codeSelections;

  int runtimeLine = assemblyMap.getObjectByAddress(runTimeSelectionAddress).lineNumber;

  for (int line : highlightLineList) {
    ColorType type = line == runtimeLine ? ColorType::HIGHLIGHT_RUNTIME : ColorType::HIGHLIGHT;
    lineSelections.append(getSelectionByLine(ui->plainTextLines, line, type));
    codeSelections.append(getSelectionByLine(ui->plainTextCode, line, type));
  }

  if (runtimeLine != -1 && !highlightLineList.contains(runtimeLine)) {
    lineSelections.append(getSelectionByLine(ui->plainTextLines, runtimeLine, ColorType::RUNTIME));
    codeSelections.append(getSelectionByLine(ui->plainTextCode, runtimeLine, ColorType::RUNTIME));
  }

  ui->plainTextLines->setExtraSelections(lineSelections);
  ui->plainTextCode->setExtraSelections(codeSelections);
}

void MainWindow::drawMemorySelections() {
  if (simpleMemory) {
    for (int row = 0; row < ui->tableWidgetSM->rowCount(); ++row) {
      int adr = row + currentSMScroll;
      if (highlightAddressList.contains(adr)) {
        ui->tableWidgetSM->item(row, 0)->setBackground(getBrushForType(ColorType::HIGHLIGHT));
        ui->tableWidgetSM->item(row, 1)->setBackground(getBrushForType(ColorType::HIGHLIGHT));
        ui->tableWidgetSM->item(row, 2)->setBackground(getBrushForType(ColorType::HIGHLIGHT));
      } else {
        ui->tableWidgetSM->item(row, 0)->setBackground(QBrush(Core::SMMemoryCellColor));
        ui->tableWidgetSM->item(row, 1)->setBackground(QBrush(Core::SMMemoryCellColor2));
        ui->tableWidgetSM->item(row, 2)->setBackground(QBrush(Core::SMMemoryCellColor));
      }
    }
  } else {
    for (int row = 0; row < ui->tableWidgetMemory->rowCount(); ++row) {
      for (int col = 0; col < ui->tableWidgetMemory->columnCount(); ++col) {
        uint16_t address = static_cast<uint16_t>(row * 16 + col);

        if (highlightAddressList.contains(address)) {
          colorMemory(address, ColorType::HIGHLIGHT);
        } else {
          colorMemory(address, ColorType::NONE);
        }
      }
    }
  }

  if (highlightAddressList.contains(runTimeSelectionAddress)) {
    colorMemory(runTimeSelectionAddress, ColorType::HIGHLIGHT_RUNTIME);
  } else {
    colorMemory(runTimeSelectionAddress, ColorType::RUNTIME);
  }
}

void MainWindow::toggleHighlight(int line) {
  if (line == -1)
    return;
  int address = assemblyMap.getObjectByLine(line).address;
  if (highlightLineList.contains(line)) {
    highlightLineList.removeOne(line);
    if (address >= 0) {
      highlightAddressList.removeOne(address);

      ColorType type = (address == runTimeSelectionAddress) ? ColorType::RUNTIME : ColorType::NONE;
      colorMemory(address, type);
    }
  } else {
    highlightLineList.append(line);
    if (address >= 0) {
      if (highlightAddressList.count(address) == 0) {
        highlightAddressList.append(address);
      }
      ColorType type = (address == runTimeSelectionAddress) ? ColorType::HIGHLIGHT_RUNTIME : ColorType::HIGHLIGHT;
      colorMemory(address, type);
    }
  }
  drawTextSelections();
}
void MainWindow::setSelectionRuntime(int address) {
  ColorType type = highlightAddressList.contains(address) ? ColorType::HIGHLIGHT_RUNTIME : ColorType::RUNTIME;
  colorMemory(address, type);

  if (address != runTimeSelectionAddress) {
    if (highlightAddressList.contains(runTimeSelectionAddress)) {
      colorMemory(runTimeSelectionAddress, ColorType::HIGHLIGHT);
    } else {
      colorMemory(runTimeSelectionAddress, ColorType::NONE);
    }
  }
  runTimeSelectionAddress = address;

  drawTextSelections();
}
void MainWindow::setSelectionCompileError(int charNum, int lineNum) {
  if (lineNum == -1) {
    return;
  }
  errorDisplayed = true;
  QList<QTextEdit::ExtraSelection> selections;
  selections.append(getSelectionByLine(ui->plainTextCode, lineNum, ColorType::ERROR));
  if (charNum != -1) {
    QTextEdit::ExtraSelection charSelection;
    charSelection.cursor = QTextCursor(ui->plainTextCode->document()->findBlockByLineNumber(lineNum));
    charSelection.cursor.setPosition(charSelection.cursor.position() + charNum);
    charSelection.cursor.setPosition(charSelection.cursor.position() + 1, QTextCursor::KeepAnchor);
    charSelection.format.setBackground(getBrushForType(ColorType::ERRORCHAR));

    selections.append(charSelection);
  }
  ui->plainTextCode->setExtraSelections(selections);

  int scrollValue = ui->plainTextCode->verticalScrollBar()->value();
  int scrollAdjustment = (lineNum > scrollValue + autoScrollUpLimit) ? autoScrollUpLimit : autoScrollDownLimit;

  ui->plainTextLines->verticalScrollBar()->setValue(lineNum - scrollAdjustment);
  ui->plainTextCode->verticalScrollBar()->setValue(lineNum - scrollAdjustment);
}

void MainWindow::clearSelections() {
  errorDisplayed = false;
  colorMemory(runTimeSelectionAddress, ColorType::NONE);
  if (!simpleMemory) {
    colorMemory(runTimeSelectionAddress, ColorType::NONE);
    for (int i = 0; i < highlightAddressList.length(); ++i) {
      colorMemory(highlightAddressList[i], ColorType::NONE);
    }
  } else {
    for (int i = 0; i < 20; ++i) {
      ui->tableWidgetSM->item(i, 0)->setBackground(QBrush(Core::SMMemoryCellColor));
      ui->tableWidgetSM->item(i, 1)->setBackground(QBrush(Core::SMMemoryCellColor2));
      ui->tableWidgetSM->item(i, 2)->setBackground(QBrush(Core::SMMemoryCellColor));
    }
  }
  highlightLineList.clear();
  highlightAddressList.clear();
  runTimeSelectionAddress = 0;
  ui->plainTextLines->setExtraSelections({});
  ui->plainTextCode->setExtraSelections({});
  drawMemorySelections();
}
void MainWindow::clearHighlights() {
  clearSelections();
  setSelectionRuntime(processor.PC);
}
