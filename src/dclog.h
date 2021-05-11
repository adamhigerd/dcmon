#ifndef D_DCLOG_H
#define D_DCLOG_H

#include <QProcess>
#include <QDateTime>

class DcLog : public QObject {
Q_OBJECT
public:
  DcLog(const QString& dcFile, QObject* parent = nullptr);

public slots:
  void terminate();
  void pause();
  void start(int tail = 100);

signals:
  void logMessage(const QDateTime& timestamp, const QString& container, const QString& message);

private slots:
  void relaunch();
  void onReadyRead();

private:
  QProcess process;
  QString dcFile;
  bool shutDown, paused;
};

#endif
