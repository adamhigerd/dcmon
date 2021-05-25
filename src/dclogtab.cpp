#include "dclogtab.h"
#include <QApplication>
#include <QClipboard>
#include <QScrollBar>
#include <QHeaderView>
#include <QTreeView>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include <QMenu>
#include <QKeyEvent>
#include <QTimer>

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

class FilterProxyModel : public QSortFilterProxyModel
{
public:
  FilterProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent), enabled(false) {}

  bool enabled;

  bool filterAcceptsRow(int row, const QModelIndex& parent) const {
    if (!enabled || !parent.isValid()) {
      return true;
    }
    QAbstractItemModel* model = sourceModel();
    QModelIndex idx = model->index(row, 0, parent);
    while (idx.isValid()) {
      QString text = model->data(idx.siblingAtColumn(1), filterRole()).toString();
      if (filterRegularExpression().match(text).hasMatch()) {
        return true;
      } else {
        idx = idx.parent();
      }
    }
    return false;
  }
};


DcLogTab::DcLogTab(TreeLogModel* model, const QString& containerName, QWidget* parent)
: QWidget(parent), container(containerName)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(1);

  search = new QLineEdit(this);
  QAction* action = search->addAction(style()->standardIcon(QStyle::SP_FileDialogContentsView, nullptr, search), QLineEdit::LeadingPosition);
  search->setClearButtonEnabled(true);
  search->setPlaceholderText("Search");
  layout->addWidget(search, 0);
  search->hide();

  actionMenu = new QMenu(search);
  action->setMenu(actionMenu);
  QObject::connect(action, SIGNAL(triggered()), this, SLOT(showSearchMenu()));

  caseAction = actionMenu->addAction(tr("Case sensitive"));
  caseAction->setCheckable(true);
  caseAction->setChecked(true);
  QObject::connect(caseAction, SIGNAL(toggled(bool)), this, SLOT(searchUpdated()));

  regexpAction = actionMenu->addAction(tr("Regular expression"));
  regexpAction->setCheckable(true);
  regexpAction->setChecked(false);
  QObject::connect(regexpAction, SIGNAL(toggled(bool)), this, SLOT(searchUpdated()));

  QObject::connect(search, SIGNAL(textEdited(QString)), this, SLOT(searchUpdated()));
  QObject::connect(search, SIGNAL(editingFinished()), this, SLOT(searchFinished()));

  filterModel = new FilterProxyModel(this);
  filterModel->setSourceModel(model);

  view = new LogTreeView(this);
  view->installEventFilter(this);
  view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  view->header()->setStretchLastSection(false);
  view->setHeaderHidden(true);
  view->setSelectionMode(QTreeView::ExtendedSelection);
  view->setTextElideMode(Qt::ElideNone);
  view->setHorizontalScrollMode(QTreeView::ScrollPerPixel);
  view->setModel(filterModel);
  layout->addWidget(view, 1);
}

void DcLogTab::showSearchMenu()
{
  QPoint pos = search->mapToGlobal(search->rect().bottomLeft());
  actionMenu->exec(pos);
}

void DcLogTab::searchUpdated()
{
  QString text = search->text();
  if (text.isEmpty()) {
    if (filterModel->enabled) {
      filterModel->enabled = false;
      filterModel->invalidate();
    }
    if (!search->hasFocus()) {
      search->hide();
    }
    return;
  }
  filterModel->enabled = true;
  if (!regexpAction->isChecked()) {
    text = QRegularExpression::escape(text);
  }
  QRegularExpression re(text);
  if (caseAction->isChecked()) {
    re.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
  } else {
    re.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption | QRegularExpression::CaseInsensitiveOption);
  }
  if (re.isValid()) {
    filterModel->setFilterRegularExpression(re);
  }
}

void DcLogTab::searchFinished()
{
  if (search->text().isEmpty()) {
    search->hide();
  } else {
    view->setFocus();
  }
}

void DcLogTab::setRootIndex(const QModelIndex& index)
{
  if (!view->rootIndex().isValid()) {
    view->setRootIndex(filterModel->mapFromSource(index));
  }
}

void DcLogTab::keyPressEvent(QKeyEvent* event)
{
  if (event == QKeySequence::Find) {
    search->show();
    search->setFocus();
  } else if (event->key() == Qt::Key_Escape) {
    search->clear();
    search->hide();
    searchUpdated();
  } else {
    QWidget::keyPressEvent(event);
  }
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

void DcLogTab::copySelected()
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

bool DcLogTab::eventFilter(QObject* watched, QEvent* event)
{
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent* ke = static_cast<QKeyEvent*>(event);
    if (ke == QKeySequence::Copy) {
      QTreeView* view = qobject_cast<QTreeView*>(watched);
      if (view) {
        copySelected();
        return true;
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

QPoint DcLogTab::scrollPos() const
{
  QScrollBar* hs = view->horizontalScrollBar();
  QScrollBar* vs = view->verticalScrollBar();
  bool scrollToBottom = vs->value() == vs->maximum();
  if (scrollToBottom) {
    return QPoint(hs->value(), -1);
  } else {
    return QPoint(hs->value(), vs->value());
  }
}

void DcLogTab::setScrollPos(const QPoint& pos)
{
  QScrollBar* hs = view->horizontalScrollBar();
  QScrollBar* vs = view->verticalScrollBar();
  hs->setValue(pos.x());
  if (pos.y() < 0) {
    QTimer::singleShot(0, vs, [vs]{ vs->triggerAction(QAbstractSlider::SliderToMaximum); });
  } else {
    vs->setValue(pos.y());
  }
}
