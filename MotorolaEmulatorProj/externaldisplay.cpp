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
#include "externaldisplay.h"
#include <QScrollBar>
#include "ui_externaldisplay.h"

ExternalDisplay::ExternalDisplay(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExternalDisplay)
{
    ui->setupUi(this);
    connect(ui->plainTextDisplay->horizontalScrollBar(), &QScrollBar::valueChanged, this, &ExternalDisplay::handleDisplayScrollHorizontal);
    connect(ui->plainTextDisplay->verticalScrollBar(), &QScrollBar::valueChanged, this, &ExternalDisplay::handleDisplayScrollVertical);

    QWidget::setWindowTitle("Display");
    //this->installEventFilter(this);
}

ExternalDisplay::~ExternalDisplay()
{
    delete ui;
}

/*/bool ExternalDisplay::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::Resize) {
        QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
        qDebug() << "Window resized to: " << resizeEvent->size();

        int newWidth = resizeEvent->size().width();
        int newHeight = (5 * newWidth) / 7;

        this->resize(newWidth, newHeight);
    }
    return true;
}/*/
void ExternalDisplay::handleDisplayScrollHorizontal(){
    ui->plainTextDisplay->horizontalScrollBar()->setValue(0);
}
void ExternalDisplay::handleDisplayScrollVertical(){
    ui->plainTextDisplay->verticalScrollBar()->setValue(0);
}






