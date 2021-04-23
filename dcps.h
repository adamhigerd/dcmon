#ifndef D_DCPS_H
#define D_DCPS_H

#include <QProcess>
#include <QTimer>
#include <QHash>

class DcPs : public QTimer {
Q_OBJECT
public:
  DcPs(const QString& dcFile, QObject* parent = nullptr);

public slots:
  void reload();
  void poll();
  void terminate();

signals:
  void statusChanged(const QString& container, const QString& status);

private slots:
  void onReadyRead();

private:
  QString dcFile;
  QProcess process;
  QHash<QString, QString> statuses;
};

#endif
