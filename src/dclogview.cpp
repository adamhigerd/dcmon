#include "dclogview.h"
#include "dclogtab.h"
#include "dcmonconfig.h"
#include "luavm.h"
#include <QApplication>
#include <QTreeView>
#include <QHeaderView>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <QFontDatabase>
#include <QStyle>
#include <QKeyEvent>
#include <QClipboard>
#include <algorithm>

DcLogView::DcLogView(QWidget* parent) : QTabWidget(parent), lua(nullptr)
{
  setTabPosition(QTabWidget::South);
  QObject::connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabActivated(int)));

  throttle.setSingleShot(true);
  throttle.setInterval(100);
  QObject::connect(&throttle, SIGNAL(timeout()), this, SLOT(onTimer()));

  model.setLogFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

  QObject::connect(CONFIG, SIGNAL(configChanged()), this, SLOT(configChanged()));
  configChanged();
}

void DcLogView::configChanged()
{
  QStringList newViews = CONFIG->filterViews.keys();
  for (int i = names.length() - 1; i >= 0; --i) {
    QString name = names[i];
    if (!filterViews.contains(name)) {
      continue;
    } else if (newViews.contains(name)) {
      newViews.removeAll(name);
    } else {
      destroyTab(i);
    }
  }
  for (const QString& view : newViews) {
    filterViews << view;
    statusChanged(view, "filter");
  }
}

void DcLogView::containerListChanged(const QStringList& containerList)
{
  QStringList newNames = containerList;
  for (int i = names.length() - 1; i >= 0; --i) {
    QString name = names[i];
    if (filterViews.contains(name)) {
      continue;
    } else if (newNames.contains(name)) {
      newNames.removeAll(name);
    } else {
      destroyTab(i);
    }
  }
  for (const QString& name : newNames) {
    addContainer(name, false);
  }
}

void DcLogView::destroyTab(int index)
{
  removeTab(index);
  QString name = names.takeAt(index);
  logs[name]->deleteLater();
  logs.remove(name);
}

void DcLogView::addContainer(const QString& container, bool isFilter)
{
  model.addContainer(container);
  DcLogTab* pane = new DcLogTab(&model, container, this);
  pane->setRootIndex(model.rootForContainer(container));
  logs[container] = pane;
  if (isFilter) {
    names.insert(0, container);
    insertTab(0, pane, container);
  } else {
    names << container;
    addTab(pane, container);
  }
}

void DcLogView::statusChanged(const QString& container, const QString& status)
{
  if (!logs.contains(container)) {
    addContainer(container, status == "filter");
  }
  int tabIndex = indexOf(logs[container]);
  if (status == "filter") {
    setTabText(tabIndex, container);
    setTabIcon(tabIndex, style()->standardIcon(QStyle::SP_FileDialogContentsView));
  } else if (status == "running") {
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
  DcLogTab* log = logs[container];
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
  for (DcLogTab* log : logs) {
    QPoint scrollPos = log->scrollPos();
    while (!log->queue.isEmpty()) {
      auto msg = log->queue.takeFirst();
      model.logMessage(msg.first, log->container, msg.second);
    }
    log->setRootIndex(model.rootForContainer(log->container));
    log->setScrollPos(scrollPos);
  }
}

void DcLogView::tabActivated(int index)
{
  DcLogTab* tab = qobject_cast<DcLogTab*>(widget(index));
  if (tab) {
    tab->setScrollPos(QPoint(0, -1));
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
  model.clear(currentContainer());
}

void DcLogView::keyPressEvent(QKeyEvent* event)
{
  if (event == QKeySequence::Find || event->key() == Qt::Key_Escape) {
    static_cast<QObject*>(logs[currentContainer()])->event(event);
  } else {
    QTabWidget::keyPressEvent(event);
  }
}

void DcLogView::copySelected()
{
  logs[currentContainer()]->copySelected();
}
