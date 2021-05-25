#ifndef D_DCLOGTAB_H
#define D_DCLOGTAB_H

#include <QWidget>
#include "treelogmodel.h"
class QTreeView;
class QLineEdit;
class QMenu;
class FilterProxyModel;

class DcLogTab : public QWidget {
Q_OBJECT
public:
  DcLogTab(TreeLogModel* model, const QString& containerName, QWidget* parent);

  const QString container;
  QDateTime lastTimestamp;
  QList<QPair<QDateTime, QString>> queue;

  QPoint scrollPos() const;
  void setScrollPos(const QPoint& pos);

  bool eventFilter(QObject* watched, QEvent* event);

public slots:
  void copySelected();
  void searchUpdated();
  void searchFinished();
  void setRootIndex(const QModelIndex& index);

protected:
  void keyPressEvent(QKeyEvent* event);

private slots:
  void showSearchMenu();

private:
  QLineEdit* search;
  QMenu* actionMenu;
  QAction* caseAction;
  QAction* regexpAction;
  QTreeView* view;
  FilterProxyModel* filterModel;
};

#endif
