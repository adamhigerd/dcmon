#ifndef D_DCLOGVIEW_H
#define D_DCLOGVIEW_H

#include <QTabWidget>
#include <QDateTime>
#include <QHash>
#include <QTimer>
#include <QSignalMapper>
#include "treelogmodel.h"
class FilterProxyModel;
class QTreeView;
class QLineEdit;
class LuaVM;
class DcLogTab;

class DcLogView : public QTabWidget {
Q_OBJECT
public:
  DcLogView(QWidget* parent = nullptr);

  QString currentContainer() const;

signals:
  void currentContainerChanged(const QString& name);

public slots:
  void addContainer(const QString& container);
  void logMessage(const QDateTime& timestamp, const QString& container, const QString& message);
  void logMessage(const QString& container, const QString& message);
  void statusChanged(const QString& container, const QString& status);
  void clearCurrent();
  void copySelected();

private slots:
  void tabActivated(int index);
  void onTimer();

protected:
  void showEvent(QShowEvent* event);
  void keyPressEvent(QKeyEvent* event);
  void copySelected(QTreeView* view);

private:
  struct LogTab {
    QString container;
    QLineEdit* search;
    QAction* caseAction;
    QAction* regexpAction;
    QTreeView* view;
    FilterProxyModel* filterModel;
    QList<QPair<QDateTime, QString>> queue;
    QDateTime lastTimestamp;
  };
  QSignalMapper searchUpdatedMapper, searchFinishedMapper;
  QHash<QString, DcLogTab*> logs;
  QStringList names;
  QTimer throttle;
  TreeLogModel model;
  LuaVM* lua;
};

#endif
