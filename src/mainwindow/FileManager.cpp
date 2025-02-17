#include "MainWindow.h"
#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include "ui_mainwindow.h"

void MainWindow::saveMemory() {
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
void MainWindow::loadMemory() {
  QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Binary Files (*.bin);;All Files (*)"));

  if (!filePath.isEmpty()) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
      QByteArray byteArray = file.readAll();
      if (byteArray.size() == sizeof(processor.Memory)) {
        processor.stopExecution();
        std::memcpy(processor.Memory.data(), byteArray.constData(), sizeof(processor.Memory));
        std::memcpy(processor.backupMemory.data(), processor.Memory.data(), processor.Memory.size() * sizeof(uint8_t));
        setAssemblyStatus(false);

      } else {
        PrintConsole("Error: File size doesn't match Memory size", MsgType::ERROR);
      }

      file.close();
    } else {
      PrintConsole("Error loading memory", MsgType::ERROR);
    }
  }
}
void MainWindow::newFile() {
  QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Save Changes"), tr("Do you want to save the current file?"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

  if (reply == QMessageBox::Save) {
    saveFile();
  } else if (reply == QMessageBox::Cancel) {
    return;
  }

  fileSavePath = "";
  PrintConsole("New file opened\n", MsgType::DEBUG);

  ui->plainTextCode->clear();
}
void MainWindow::openFile() {
  QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Assembly Files (*.txt *.asm);;All Files (*)"));
  if (!filePath.isEmpty()) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      fileSavePath = filePath;
      QTextStream in(&file);
      ui->plainTextCode->setPlainText(in.readAll());
      file.close();
      PrintConsole("File opened successfully: " + filePath + '\n', MsgType::DEBUG);
    } else {
      PrintConsole("Error loading script\n", MsgType::ERROR);
    }
  }
}
void MainWindow::saveFile() {
  if (fileSavePath.isEmpty()) {
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("Assembly Files (*.txt *.asm);;All Files (*)"));
    fileSavePath = filePath;
  }
  if (!fileSavePath.isEmpty()) {
    QFile file(fileSavePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream out(&file);
      out << ui->plainTextCode->toPlainText();
      file.close();
      PrintConsole("File saved successfully: " + fileSavePath + '\n', MsgType::DEBUG);
    } else {
      PrintConsole("Error saving script\n", MsgType::ERROR);
    }
  }
}
void MainWindow::saveAsFile() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("Assembly Files (*.txt *.asm);;All Files (*)"));

  if (!filePath.isEmpty()) {
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      fileSavePath = filePath;
      QTextStream out(&file);
      out << ui->plainTextCode->toPlainText();
      file.close();
      PrintConsole("File saved successfully as: " + filePath + '\n', MsgType::DEBUG);
    } else {
      PrintConsole("Error saving script\n", MsgType::ERROR);
    }
  }
}

void MainWindow::exit() {
  if (!fileSavePath.isEmpty()) {
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Save Changes"), tr("Do you want to save the current file?"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (reply == QMessageBox::Save) {
      saveFile();
    } else if (reply == QMessageBox::Cancel) {
      return;
    }
  }
  QApplication::exit(0);
}
