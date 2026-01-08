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
#ifndef INSTRUCTIONINFODIALOG_H
#define INSTRUCTIONINFODIALOG_H

#include <QDialog>
#include <QTreeWidget>

namespace Ui {
  class InstructionInfoDialog;
}

class InstructionInfoDialog final : public QDialog {
  Q_OBJECT

public:
  explicit InstructionInfoDialog(QTreeWidgetItem item, QWidget *parent = nullptr);
  ~InstructionInfoDialog() final;

private:
  Ui::InstructionInfoDialog *ui;
};

#endif // INSTRUCTIONINFODIALOG_H
