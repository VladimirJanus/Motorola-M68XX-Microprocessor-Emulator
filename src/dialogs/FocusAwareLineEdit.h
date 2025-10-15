#ifndef FOCUSAWARELINEEDIT_H
#define FOCUSAWARELINEEDIT_H

#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
class FocusAwareLineEdit : public QLineEdit {
  Q_OBJECT

public:
  FocusAwareLineEdit(QWidget *parent, QWidget *focusTarget)
      : QLineEdit(parent), m_focusTarget(focusTarget) {
    setAttribute(Qt::WA_DeleteOnClose);
    installEventFilter(this);
  }
  bool eventFilter(QObject *obj, QEvent *event) override {
    if (obj == this) {
      if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key::Key_Escape) {
          emit escapePressed();
          return true;
        }
      }
    }
    return QLineEdit::eventFilter(obj, event);
  }
signals:
  void escapePressed();

protected:
  void closeEvent(QCloseEvent *event) override {
    if (m_focusTarget) {
      m_focusTarget->setFocus();
    }
    QLineEdit::closeEvent(event);
  }
  void focusOutEvent(QFocusEvent *event) override {
    QLineEdit::focusOutEvent(event);
    close();
  }

private:
  QWidget *m_focusTarget;
};

#endif // FOCUSAWARELINEEDIT_H
