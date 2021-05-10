#ifndef D_DCLOGVIEW_H
#define D_DCLOGVIEW_H

#include <QTabWidget>
#include <QDateTime>
#include <QHash>
#include <QTimer>
#include "treelogmodel.h"
class QTreeView;

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

private slots:
  void tabActivated(int index);
  void onTimer();

protected:
  void showEvent(QShowEvent* event);

private:
  struct LogTab {
    QString container;
    //QPlainTextEdit* view;
    QTreeView* view;
    QList<QPair<QDateTime, QString>> queue;
    QDateTime lastTimestamp;
  };
  QHash<QString, LogTab> logs;
  QStringList names;
  QTimer throttle;
  TreeLogModel model;
};

#endif
