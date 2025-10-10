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
#include "src/dialogs/ExternalDisplay.h"
#include <QEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include "ui_ExternalDisplay.h"

int defaultW = 498;
int defaultH = 350;

ExternalDisplay::ExternalDisplay(
  QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::ExternalDisplay) {
  ui->setupUi(this);
  setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);

  connect(ui->plainTextDisplay->horizontalScrollBar(), &QScrollBar::valueChanged, this, &ExternalDisplay::handleDisplayScrollHorizontal);
  connect(ui->plainTextDisplay->verticalScrollBar(), &QScrollBar::valueChanged, this, &ExternalDisplay::handleDisplayScrollVertical);

  QWidget::setWindowTitle("Display");

  resize(defaultW, defaultH);     // Initial size
  this->installEventFilter(this); // Install event filter on the dialog itself
}

ExternalDisplay::~ExternalDisplay() {
  delete ui;
}

QPlainTextEdit *ExternalDisplay::getPlainTextEdit() {
  return ui->plainTextDisplay;
}

void ExternalDisplay::handleDisplayScrollHorizontal() {
  ui->plainTextDisplay->horizontalScrollBar()->setValue(0);
}

void ExternalDisplay::handleDisplayScrollVertical() {
  ui->plainTextDisplay->verticalScrollBar()->setValue(0);
}

inline int GetFontSizePerCharSize ( int w, int h) {
  const static float WidthRatio = QFontMetricsF(QFont("Courier New", 11, QFont::Bold)).averageCharWidth() / 11.0;
  const static float HeightRatio = QFontMetricsF(QFont("Courier New", 11, QFont::Bold)).height() / 11.0;
  int optimalByWidth = w / WidthRatio;
  int optimalByHeight = h / HeightRatio;
  return qMax(11, qMin(optimalByWidth, optimalByHeight));
}

bool ExternalDisplay::eventFilter(
  QObject *obj, QEvent *event) {
  if (obj == this && event->type() == QEvent::Resize) {
    QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
    QSize fullSize = resizeEvent->size();
    QSize availableSize = fullSize - QSize(20, 20);

    QFont font = ui->plainTextDisplay->font();

    int desiredWidth = 54;
    int desiredLines = 20;

    int fontWidth = availableSize.width() / desiredWidth;
    int fontHeight = availableSize.height() / desiredLines;

    int newFontSize = GetFontSizePerCharSize(fontWidth, fontHeight);

    font.setPointSize(newFontSize);
    ui->plainTextDisplay->setFont(font);

    QFontMetrics fm(font);
    int charWidth = fm.averageCharWidth();
    int lineHeight = fm.height();

    int displayWidth = charWidth * desiredWidth + ui->plainTextDisplay->document()->documentMargin() * 2;
    int displayHeight = lineHeight * desiredLines + ui->plainTextDisplay->document()->documentMargin() * 2;

    ui->plainTextDisplay->resize(displayWidth, displayHeight);
    // -2 for widget borders
    ui->plainTextDisplay->move((availableSize.width() - displayWidth - 2) / 2 + 10, (availableSize.height() - displayHeight - 2) / 2 + 10);


  }

  return QDialog::eventFilter(obj, event);
}
