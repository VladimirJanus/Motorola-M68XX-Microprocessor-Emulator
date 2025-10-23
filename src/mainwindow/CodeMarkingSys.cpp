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
#include "qscrollbar.h"
#include "src/mainwindow/MainWindow.h"
#include "src/processor/Processor.h"
#include "ui_MainWindow.h"
#include <QTextBlock>

using Core::Action;
using Core::ActionType;
using Core::ColorType;
using Core::MemoryDisplayMode;

int runtimeMarkerAddress = 0;
QVector<int> markedLineList;

QBrush brushMarked(Qt::GlobalColor::green);
QBrush brushRuntime(Qt::GlobalColor::yellow);
QBrush brushMarkedRuntime(Qt::GlobalColor::cyan);
QBrush brushError(Qt::GlobalColor::darkYellow);
QBrush brushErrorChar(Qt::GlobalColor::darkGreen);
QBrush brushNone(QColor(230, 230, 255));

QBrush &getBrushForType(ColorType colorType) {
  switch (colorType) {
  case ColorType::MARKED:
    return brushMarked;
  case ColorType::CURRENTINSTRUCTION:
    return brushRuntime;
  case ColorType::MARKED_CURRENTINSTRUCTION:
    return brushMarkedRuntime;
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
  if (memoryDisplayMode == MemoryDisplayMode::FULL) {
    ui->tableWidgetMemory->item(address / 16, address % 16)->setBackground(getBrushForType(colorType));
  } else if (memoryDisplayMode == MemoryDisplayMode::SIMPLE) {
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

QTextEdit::ExtraSelection getLineMarker(QPlainTextEdit *textEdit, int line, ColorType colorType) {
  QTextEdit::ExtraSelection lineMarker;
  lineMarker.format.setBackground(getBrushForType(colorType));
  QTextBlock block = textEdit->document()->findBlockByLineNumber(line);
  QTextCursor cursor(textEdit->document());
  cursor.setPosition(block.position());
  cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
  lineMarker.cursor = cursor;
  return lineMarker;
}

void MainWindow::drawTextMarkers() {
  errorDisplayed = false;
  QList<QTextEdit::ExtraSelection> lineMarkers;
  QList<QTextEdit::ExtraSelection> codeMarkers;

  int runtimeLine = assemblyMap.getObjectByAddress(runtimeMarkerAddress).lineNumber;

  for (int line : markedLineList) {
    ColorType type = line == runtimeLine ? ColorType::MARKED_CURRENTINSTRUCTION : ColorType::MARKED;
    lineMarkers.append(getLineMarker(ui->plainTextLines, line, type));
    codeMarkers.append(getLineMarker(ui->plainTextCode, line, type));
  }

  if (runtimeLine != -1 && !markedLineList.contains(runtimeLine)) {
    lineMarkers.append(getLineMarker(ui->plainTextLines, runtimeLine, ColorType::CURRENTINSTRUCTION));
    codeMarkers.append(getLineMarker(ui->plainTextCode, runtimeLine, ColorType::CURRENTINSTRUCTION));
  }

  ui->plainTextLines->setExtraSelections(lineMarkers);
  ui->plainTextCode->setExtraSelections(codeMarkers);
}

void MainWindow::drawMemoryMarkers() {
  if (memoryDisplayMode == MemoryDisplayMode::SIMPLE) {
    for (int row = 0; row < ui->tableWidgetSM->rowCount(); ++row) {
      int adr = row + currentSMScroll;
      if (markedAddressList.contains(adr)) {
        ui->tableWidgetSM->item(row, 0)->setBackground(getBrushForType(ColorType::MARKED));
        ui->tableWidgetSM->item(row, 1)->setBackground(getBrushForType(ColorType::MARKED));
        ui->tableWidgetSM->item(row, 2)->setBackground(getBrushForType(ColorType::MARKED));
      } else {
        ui->tableWidgetSM->item(row, 0)->setBackground(QBrush(Core::SMMemoryCellColor));
        ui->tableWidgetSM->item(row, 1)->setBackground(QBrush(Core::SMMemoryCellColor2));
        ui->tableWidgetSM->item(row, 2)->setBackground(QBrush(Core::SMMemoryCellColor));
      }
    }
  } else if (memoryDisplayMode == MemoryDisplayMode::FULL) {
    for (int row = 0; row < ui->tableWidgetMemory->rowCount(); ++row) {
      for (int col = 0; col < ui->tableWidgetMemory->columnCount(); ++col) {
        uint16_t address = static_cast<uint16_t>(row * 16 + col);

        if (markedAddressList.contains(address)) {
          colorMemory(address, ColorType::MARKED);
        } else {
          colorMemory(address, ColorType::NONE);
        }
      }
    }
  }

  if (markedAddressList.contains(runtimeMarkerAddress)) {
    colorMemory(runtimeMarkerAddress, ColorType::MARKED_CURRENTINSTRUCTION);
  } else {
    colorMemory(runtimeMarkerAddress, ColorType::CURRENTINSTRUCTION);
  }
}

void MainWindow::toggleCodeMarker(int line) {
  if (line == -1)
    return;
  int address = assemblyMap.getObjectByLine(line).address;
  if (markedLineList.contains(line)) {
    markedLineList.removeOne(line);
    if (address >= 0) {
      markedAddressList.removeOne(address);
      processor->queueBookmarkData(markedAddressList);
      processor->addAction(Action{ActionType::UPDATEBOOKMARKS, 0});

      ColorType type = (address == runtimeMarkerAddress) ? ColorType::CURRENTINSTRUCTION : ColorType::NONE;
      colorMemory(address, type);
    }
  } else {
    markedLineList.append(line);
    if (address >= 0) {
      if (markedAddressList.count(address) == 0) {
        markedAddressList.append(address);
        processor->queueBookmarkData(markedAddressList);
        processor->addAction(Action{ActionType::UPDATEBOOKMARKS, 0});
      }
      ColorType type = (address == runtimeMarkerAddress) ? ColorType::MARKED_CURRENTINSTRUCTION : ColorType::MARKED;
      colorMemory(address, type);
    }
  }
  drawTextMarkers();
}

void MainWindow::setCurrentInstructionMarker(int address) {
  ColorType type = markedAddressList.contains(address) ? ColorType::MARKED_CURRENTINSTRUCTION : ColorType::CURRENTINSTRUCTION;
  colorMemory(address, type);

  if (address != runtimeMarkerAddress) {
    if (markedAddressList.contains(runtimeMarkerAddress)) {
      colorMemory(runtimeMarkerAddress, ColorType::MARKED);
    } else {
      colorMemory(runtimeMarkerAddress, ColorType::NONE);
    }
  }
  runtimeMarkerAddress = address;

  drawTextMarkers();
}

void MainWindow::setAssemblyErrorMarker(int charNum, int lineNum) {
  if (lineNum == -1) {
    return;
  }
  errorDisplayed = true;
  QList<QTextEdit::ExtraSelection> markers;
  markers.append(getLineMarker(ui->plainTextCode, lineNum, ColorType::ERROR));
  if (charNum != -1) {
    QTextEdit::ExtraSelection charMarker;
    charMarker.cursor = QTextCursor(ui->plainTextCode->document()->findBlockByLineNumber(lineNum));
    charMarker.cursor.setPosition(charMarker.cursor.position() + charNum);
    charMarker.cursor.setPosition(charMarker.cursor.position() + 1, QTextCursor::KeepAnchor);
    charMarker.format.setBackground(getBrushForType(ColorType::ERRORCHAR));

    markers.append(charMarker);
  }
  ui->plainTextCode->setExtraSelections(markers);

  int scrollValue = ui->plainTextCode->verticalScrollBar()->value();
  int scrollAdjustment = (lineNum > scrollValue + autoScrollUpLimit) ? autoScrollUpLimit : autoScrollDownLimit;

  ui->plainTextLines->verticalScrollBar()->setValue(lineNum - scrollAdjustment);
  ui->plainTextCode->verticalScrollBar()->setValue(lineNum - scrollAdjustment);
}

void MainWindow::clearCodeMarkers() {
  errorDisplayed = false;
  colorMemory(runtimeMarkerAddress, ColorType::NONE);
  if (memoryDisplayMode == MemoryDisplayMode::FULL) {
    colorMemory(runtimeMarkerAddress, ColorType::NONE);
    for (int i = 0; i < markedAddressList.length(); ++i) {
      colorMemory(markedAddressList[i], ColorType::NONE);
    }
  } else if (memoryDisplayMode == MemoryDisplayMode::SIMPLE) {
    for (int i = 0; i < 20; ++i) {
      ui->tableWidgetSM->item(i, 0)->setBackground(QBrush(Core::SMMemoryCellColor));
      ui->tableWidgetSM->item(i, 1)->setBackground(QBrush(Core::SMMemoryCellColor2));
      ui->tableWidgetSM->item(i, 2)->setBackground(QBrush(Core::SMMemoryCellColor));
    }
  }
  markedLineList.clear();
  markedAddressList.clear();
  processor->queueBookmarkData(markedAddressList);
  processor->addAction(Action{ActionType::UPDATEBOOKMARKS, 0});

  runtimeMarkerAddress = 0;
  ui->plainTextLines->setExtraSelections({});
  ui->plainTextCode->setExtraSelections({});
  drawMemoryMarkers();
}

void MainWindow::clearMarkers() {
  clearCodeMarkers();
  setCurrentInstructionMarker(processor->PC);
}
