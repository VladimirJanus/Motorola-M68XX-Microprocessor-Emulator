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
#include "instructioninfodialog.h"
#include "qtreewidget.h"
#include "ui_instructioninfodialog.h"

InstructionInfoDialog::InstructionInfoDialog(QTreeWidgetItem item, int version, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InstructionInfoDialog)
{
    ui->setupUi(this);

    ui->labelInstruction->setText(item.text(0));
    ui->labelOperation->setText(item.text(1));
    ui->labelDescription->setText(item.text(11) + "\nFlags: HINZVC:" + item.text(8));
    QStringList g{"INH", "IMM", "DIR", "IND", "EXT", "REL"};
    int count = 0;
     for (int i = 2; i < 8; ++i) {
        if(item.text(i) != ""){
             ui->tableWidget->insertColumn(count);
            ui->tableWidget->setHorizontalHeaderItem(ui->tableWidget->columnCount() - 1, new QTableWidgetItem(g[i - 2]));
             QTableWidgetItem *newItem = new QTableWidgetItem(item.text(i));
             ui->tableWidget->setItem(0, ui->tableWidget->columnCount() - 1, newItem);
             newItem = new QTableWidgetItem(item.text(9).split(",")[count]);
             ui->tableWidget->setItem(1, ui->tableWidget->columnCount() - 1, newItem);
             newItem = new QTableWidgetItem(item.text(10).split(",")[count]);
             ui->tableWidget->setItem(2, ui->tableWidget->columnCount() - 1, newItem);
            count++;
        }
    }

     if(count == 0) ui->tableWidget->setVisible(false);
}


InstructionInfoDialog::~InstructionInfoDialog()
{
    delete ui;
}
