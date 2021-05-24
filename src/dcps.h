#ifndef D_DCPS_H
#define D_DCPS_H

#include <QProcess>
#include <QTimer>
#include <QHash>

class DcPs : public QTimer {
Q_OBJECT
public:
  DcPs(QObject* parent = nullptr);

public slots:
  void reload();
  void poll();
  void terminate();

signals:
  void statusChanged(const QString& container, const QString& status);
  void allStopped();
  void started();

private slots:
  void onReadyRead();

private:
  QProcess process;
  QHash<QString, QString> statuses;
  bool wasStopped;
};

#endif
