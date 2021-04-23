#include "dclogview.h"
#include <QScrollBar>
#include <QFontDatabase>
#include <QStyle>

DcLogView::DcLogView(QWidget* parent) : QTabWidget(parent)
{
  setTabPosition(QTabWidget::South);
  QObject::connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabActivated(int)));
}

void DcLogView::addContainer(const QString& container)
{
  QPlainTextEdit* browser = new QPlainTextEdit(this);
  browser->setLineWrapMode(QPlainTextEdit::NoWrap);
  browser->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  browser->setReadOnly(true);
  // TODO: configure
  browser->setMaximumBlockCount(10000);
  logs[container] = browser;
  names << container;
  addTab(browser, container);
}

void DcLogView::statusChanged(const QString& container, const QString& status)
{
  if (!logs.contains(container)) {
    addContainer(container);
  }
  int tabIndex = indexOf(logs[container]);
  if (status == "running") {
    setTabText(tabIndex, container);
    setTabIcon(tabIndex, style()->standardIcon(QStyle::SP_MediaPlay));
  } else {
    setTabText(tabIndex, QString("%1 (%2)").arg(container).arg(status));
    setTabIcon(tabIndex, style()->standardIcon(QStyle::SP_MediaStop));
  }
}

void DcLogView::logMessage(const QDateTime& /*timestamp*/, const QString& container, const QString& message)
{
  if (!logs.contains(container)) {
    addContainer(container);
  }
  QPlainTextEdit* browser = logs[container];
  QString msg = message;
  int s = msg.length();
  for (int i = 0; i < s - 2; i++) {
    if (msg[i] != 27 || msg[i + 1] != '[') {
      continue;
    }
    for (int j = i + 2; j < s; j++) {
      QChar ch = msg[j];
      if (ch == 'm') {
        msg.replace(i, j - i + 1, "");
        i -= 1;
        s = msg.length();
        break;
      } else if (!ch.isDigit() && ch != ';') {
        msg.replace(i, 1, "<ESC>");
        s += 4;
        i += 4;
        break;
      }
    }
    continue;
  }
  QScrollBar* hs = browser->horizontalScrollBar();
  int hscroll = hs->value();
  browser->appendPlainText(msg);
  hs->setValue(hscroll);
}

void DcLogView::tabActivated(int index)
{
  QPlainTextEdit* browser = qobject_cast<QPlainTextEdit*>(widget(index));
  if (browser) {
    browser->horizontalScrollBar()->triggerAction(QAbstractSlider::SliderToMinimum);
    browser->verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
    emit currentContainerChanged(currentContainer());
  }
}

QString DcLogView::currentContainer() const
{
  int index = currentIndex();
  if (index < 0 || index >= names.length()) {
    return QString();
  }
  return names[index];
}

void DcLogView::showEvent(QShowEvent* event)
{
  QTabWidget::showEvent(event);
  tabActivated(currentIndex());
}

void DcLogView::clearCurrent()
{
  QPlainTextEdit* browser = qobject_cast<QPlainTextEdit*>(currentWidget());
  if (browser) {
    browser->clear();
  }
}
