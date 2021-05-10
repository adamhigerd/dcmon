#include "dclogview.h"
#include <QTreeView>
#include <QScrollBar>
#include <QFontDatabase>
#include <QStyle>
#include <QtDebug>

DcLogView::DcLogView(QWidget* parent) : QTabWidget(parent)
{
  setTabPosition(QTabWidget::South);
  QObject::connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabActivated(int)));

  throttle.setSingleShot(true);
  throttle.setInterval(100);
  QObject::connect(&throttle, SIGNAL(timeout()), this, SLOT(onTimer()));

  model.setLogFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
}

void DcLogView::addContainer(const QString& container)
{
  QTreeView* view = new QTreeView(this);
  view->setModel(&model);
  view->setRootIndex(model.rootForContainer(container));
  logs[container] = (LogTab){
    container,
    view,
    QList<QPair<QDateTime, QString>>(),
    QDateTime(),
  };
  names << container;
  addTab(view, container);
}

void DcLogView::statusChanged(const QString& container, const QString& status)
{
  if (!logs.contains(container)) {
    addContainer(container);
  }
  int tabIndex = indexOf(logs[container].view);
  if (status == "running") {
    setTabText(tabIndex, container);
    setTabIcon(tabIndex, style()->standardIcon(QStyle::SP_MediaPlay));
  } else {
    setTabText(tabIndex, QString("%1 (%2)").arg(container).arg(status));
    setTabIcon(tabIndex, style()->standardIcon(QStyle::SP_MediaStop));
  }
}

void DcLogView::logMessage(const QString& container, const QString& message)
{
  logMessage(QDateTime(), container, message);
}

void DcLogView::logMessage(const QDateTime& timestamp, const QString& container, const QString& message)
{
  if (!logs.contains(container)) {
    addContainer(container);
  }
  LogTab* log = &logs[container];
  if (!timestamp.isNull()) {
    if (log->lastTimestamp > timestamp) {
      // Old log message
      return;
    }
    log->lastTimestamp = timestamp;
  }
  log->queue << QPair<QDateTime, QString>(timestamp, message);
  if (!throttle.isActive()) {
    throttle.start();
  }
}

void DcLogView::onTimer()
{
  for (LogTab& log : logs) {
    QScrollBar* hs = log.view->horizontalScrollBar();
    int hscroll = hs->value();
    while (!log.queue.isEmpty()) {
      auto msg = log.queue.takeFirst();
      model.logMessage(msg.first, log.container, msg.second);
    }
    if (!log.view->rootIndex().isValid()) {
      log.view->setRootIndex(model.rootForContainer(log.container));
    }
    hs->setValue(hscroll);
    log.view->verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
  }
}

void DcLogView::tabActivated(int index)
{
  QAbstractScrollArea* browser = qobject_cast<QAbstractScrollArea*>(widget(index));
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
  /*
  QPlainTextEdit* browser = qobject_cast<QPlainTextEdit*>(currentWidget());
  if (browser) {
    browser->clear();
  }
  */
}
