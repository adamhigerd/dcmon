#ifndef D_DCLOGVIEW_H
#define D_DCLOGVIEW_H

#include <QTabWidget>
#include <QDateTime>
#include <QHash>
#include <QTimer>
#include "treelogmodel.h"
class QTreeView;
class LuaVM;

class DcLogView : public QTabWidget {
Q_OBJECT
public:
  DcLogView(QWidget* parent = nullptr);

  QString currentContainer() const;

  bool eventFilter(QObject* watched, QEvent* event);
  void setLuaVM(LuaVM* lua);

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
  void copySelected(QTreeView* view);

private:
  struct LogTab {
    QString container;
    QTreeView* view;
    QList<QPair<QDateTime, QString>> queue;
    QDateTime lastTimestamp;
  };
  QHash<QString, LogTab> logs;
  QStringList names;
  QTimer throttle;
  TreeLogModel model;
  LuaVM* lua;
};

#endif
