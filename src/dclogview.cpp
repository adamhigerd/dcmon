#include "dclogview.h"
#include "luavm.h"
#include <QApplication>
#include <QTreeView>
#include <QHeaderView>
#include <QScrollBar>
#include <QFontDatabase>
#include <QStyle>
#include <QKeyEvent>
#include <QClipboard>
#include <algorithm>

class LogTreeView : public QTreeView
{
public:
  LogTreeView(QWidget* parent) : QTreeView(parent) {}

  void scrollTo(const QModelIndex& idx, QAbstractItemView::ScrollHint hint) {
    QScrollBar* hs = horizontalScrollBar();
    int hpos = (hint == EnsureVisible && idx.column() != 0) ? hs->value() : 0;
    QTreeView::scrollTo(idx, hint);
    hs->setValue(hpos);
  }
};

DcLogView::DcLogView(QWidget* parent) : QTabWidget(parent), lua(nullptr)
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
  QTreeView* view = new LogTreeView(this);
  view->installEventFilter(this);
  view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  view->header()->setStretchLastSection(false);
  view->setHeaderHidden(true);
  view->setSelectionMode(QTreeView::ExtendedSelection);
  view->setTextElideMode(Qt::ElideNone);
  view->setHorizontalScrollMode(QTreeView::ScrollPerPixel);
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
    QScrollBar* vs = log.view->verticalScrollBar();
    bool scrollToBottom = vs->value() == vs->maximum();
    int hscroll = hs->value();
    while (!log.queue.isEmpty()) {
      auto msg = log.queue.takeFirst();
      model.logMessage(msg.first, log.container, msg.second);
    }
    if (!log.view->rootIndex().isValid()) {
      log.view->setRootIndex(model.rootForContainer(log.container));
    }
    hs->setValue(hscroll);
    if (scrollToBottom) {
      QTimer::singleShot(0, vs, [vs]{ vs->triggerAction(QAbstractSlider::SliderToMaximum); });
    }
  }
}

void DcLogView::tabActivated(int index)
{
  QAbstractScrollArea* browser = qobject_cast<QAbstractScrollArea*>(widget(index));
  if (browser) {
    QTimer::singleShot(0, browser, [browser]{
      browser->horizontalScrollBar()->triggerAction(QAbstractSlider::SliderToMinimum);
      browser->verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
    });
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

static bool compareTreeIndex(const QModelIndex& lhs, const QModelIndex& rhs)
{
  QModelIndexList lp, rp;

  QModelIndex t = lhs;
  while (t.isValid()) {
    lp << t;
    t = t.parent();
  }

  t = rhs;
  while (t.isValid()) {
    rp << t;
    t = t.parent();
  }

  while (lp.size() && rp.size() && lp.back().row() == rp.back().row()) {
    lp.pop_back();
    rp.pop_back();
  }

  if (lp.size() && rp.size()) {
    return lp.back().row() < rp.back().row();
  }
  return rp.size() && !lp.size();
}

static void addRecursive(QModelIndexList& idxs, const QModelIndex& idx)
{
  const QAbstractItemModel* model = idx.model();
  int rc = model->rowCount(idx);
  for (int i = 0; i < rc; i++) {
    QModelIndex child = model->index(i, 1, idx);
    idxs << child;
    addRecursive(idxs, child);
  }
}

bool DcLogView::eventFilter(QObject* watched, QEvent* event)
{
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent* ke = static_cast<QKeyEvent*>(event);
    if (ke == QKeySequence::Copy) {
      QTreeView* view = qobject_cast<QTreeView*>(watched);
      if (view) {
        copySelected(view);
        return true;
      }
    }
  }
  return QTabWidget::eventFilter(watched, event);
}

void DcLogView::copySelected()
{
  copySelected(logs[currentContainer()].view);
}

void DcLogView::copySelected(QTreeView* view)
{
  QModelIndexList idxs;
  for (const QModelIndex& idx : view->selectionModel()->selectedIndexes()) {
    if (idx.column() == 1) {
      idxs << idx;
      if (!view->isExpanded(idx)) {
        addRecursive(idxs, idx);
      }
    }
  }
  std::sort(idxs.begin(), idxs.end(), compareTreeIndex);
  QString toCopy;
  for (const QModelIndex& idx : idxs) {
    toCopy += idx.data(Qt::DisplayRole).toString() + "\n";
  }
  qApp->clipboard()->setText(toCopy);
}
