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
#ifndef EXTERNALDISPLAY_H
#define EXTERNALDISPLAY_H

#include <QDialog>
#include <QPlainTextEdit>
#include "ui_externaldisplay.h"

namespace Ui {
  class ExternalDisplay;
}
class ExternalDisplay : public QDialog {
  Q_OBJECT

public:
  explicit ExternalDisplay(QWidget *parent = nullptr);
  ~ExternalDisplay();
  QPlainTextEdit *getPlainTextEdit() { return ui->plainTextDisplay; }
  void checkMousePos();

private:
  Ui::ExternalDisplay *ui;
private slots:
  void handleDisplayScrollVertical();
  void handleDisplayScrollHorizontal();
};

#endif // EXTERNALDISPLAY_H
