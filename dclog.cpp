#include "dclog.h"

DcLog::DcLog(const QString& dcFile, QObject* parent) : QObject(parent)
{
  process.setProcessChannelMode(QProcess::MergedChannels);
  QObject::connect(&process, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
  process.start("docker-compose", QStringList() << "-f" << dcFile << "logs" << "--no-color" << "--follow" << "--tail=100" << "--timestamps");
}

void DcLog::terminate()
{
  if (process.state() != QProcess::NotRunning) {
    process.terminate();
    if (!process.waitForFinished()) {
      process.kill();
      process.waitForFinished();
    }
  }
}

void DcLog::onReadyRead()
{
  while (process.canReadLine()) {
    QString line = process.readLine();
    int pipePos = line.indexOf('|');
    if (pipePos < 0) {
      continue;
    }
    QString container = line.left(pipePos).trimmed();
    QString timeString = line.mid(pipePos + 2, 30);
    QString message = line.mid(pipePos + 33);
    while (message[message.length() - 1].isSpace()) {
      message.chop(1);
    }
    QDateTime timestamp = QDateTime::fromString(timeString, Qt::ISODateWithMs);
    emit logMessage(timestamp, container, message);
  }
}
