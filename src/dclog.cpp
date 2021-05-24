#include "dclog.h"
#include "dcmonconfig.h"
#include <QRegularExpression>
#include <QtDebug>

static QRegularExpression timestampRE("^\\s*(?:\\[?\\d{4}-\\d{2}-\\d{2}[T ]\\d{2}:\\d{2}(?::\\d{2}(?:[.,]\\d+)?)? ?(?:Z|UTC)?]?\\s?)+");

static QString stripColor(QString msg) {
  int s = msg.length();
  for (int i = 0; i < s - 2; i++) {
    if (msg[i] != char(27) || msg[i + 1] != '[') {
      continue;
    }
    for (int j = i + 2; j < s; j++) {
      QChar ch = msg[j];
      if (ch == 'm') {
        msg.replace(i, j - i + 1, "");
        i -= 1;
        s = msg.length();
        break;
      } else if (!ch.isDigit() && ch != ';') {
        msg.replace(i, 1, "<ESC>");
        s += 4;
        i += 4;
        break;
      }
    }
    continue;
  }
  return msg;
}

DcLog::DcLog(QObject* parent) : QObject(parent), shutDown(false), paused(false)
{
  process.setProcessChannelMode(QProcess::MergedChannels);
  QObject::connect(&process, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
  QObject::connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(relaunch()));
  start(100);
}

void DcLog::pause() {
  paused = true;
}

void DcLog::relaunch() {
  if (shutDown || paused) {
    return;
  }
  start();
}

void DcLog::start(int tail)
{
  if (process.state() != QProcess::NotRunning) {
    return;
  }
  paused = false;
  process.start("docker-compose", QStringList() << "-f" << CONFIG->dcFile << "logs" << "--no-color" << "--follow" << QString("--tail=%1").arg(tail) << "--timestamps");
}

void DcLog::terminate()
{
  shutDown = true;
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
  bool doRestart = false;
  while (process.canReadLine()) {
    QString line = process.readLine();
    int pipePos = line.indexOf('|');
    if (pipePos < 0) {
      continue;
    }
    QString container = line.left(pipePos).trimmed();
    int zPos = line.indexOf("Z ", pipePos + 2);
    QDateTime timestamp;
    if (zPos < 0) {
      timestamp = QDateTime::currentDateTime();
      zPos = pipePos;
    } else {
      QString timeString = line.mid(pipePos + 2, 30);
      timestamp = QDateTime::fromString(timeString, Qt::ISODateWithMs);
    }
    QString message = line.mid(zPos + 2);
    if (message.contains("Error grabbing logs: unexpected EOF")) {
      doRestart = true;
      continue;
    }
    if (CONFIG->hiddenContainers.contains(container)) {
      continue;
    }
    message = stripColor(message);
    while (message.length() > 0 && message[message.length() - 1].isSpace()) {
      message.chop(1);
    }
    message = message.remove(timestampRE);
    if (!message.isEmpty()) {
      LuaFunction filter = CONFIG->logFilter(container);
      if (filter.isValid()) {
        try {
          QVariant filtered = filter({ message });
          if (!filtered.isValid()) {
            continue;
          } else if (filtered.canConvert<QByteArray>()) {
            message = QString::fromUtf8(filtered.toByteArray());
          }
        } catch (LuaException& e) {
          emit logMessage(timestamp, container, tr("Error in filter: %1").arg(QString::fromUtf8(e.what())));
        }
      }
      emit logMessage(timestamp, container, message);
    }
  }
  if (doRestart) {
    process.terminate();
  }
}
