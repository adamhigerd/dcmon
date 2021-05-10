#include "dcps.h"

DcPs::DcPs(const QString& dcFile, QObject* parent) : QTimer(parent), dcFile(dcFile), wasStopped(true)
{
  process.setProcessChannelMode(QProcess::MergedChannels);
  QObject::connect(&process, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
  process.setProgram("docker");
  process.setArguments(QStringList() << "ps" << "-a" << "--format" << "{{.Names}}|{{.State}}|{{.Status}}");
  setInterval(5000);
  setSingleShot(true);
  QObject::connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(start()));
  QObject::connect(this, SIGNAL(timeout()), this, SLOT(poll()));

  reload();
}

void DcPs::terminate()
{
  if (process.state() != QProcess::NotRunning) {
    process.terminate();
    if (!process.waitForFinished()) {
      process.kill();
      process.waitForFinished();
    }
  }
  QTimer::stop();
}

void DcPs::reload()
{
  QProcess dc;
  dc.start("docker-compose", QStringList() << "-f" << dcFile << "ps");
  dc.waitForFinished();
  while (dc.canReadLine()) {
    QString line = dc.readLine().trimmed();
    if (line[0] == '-') {
      continue;
    }
    if (line.startsWith("Name ")) {
      continue;
    }
    int spacePos = line.indexOf(" ");
    QString container = line.left(spacePos);
    statuses[container] = "";
  }
  process.start();
}

void DcPs::poll()
{
  if (process.state() != QProcess::NotRunning) {
    start(500);
  } else {
    QTimer::stop();
    setInterval(5000);
    process.start();
  }
}

void DcPs::onReadyRead()
{
  int numRunning = 0;
  while (process.canReadLine()) {
    QList<QByteArray> line = process.readLine().trimmed().split('|');
    QString container = line[0];
    if (!statuses.contains(container)) {
      continue;
    }
    QString status = line[1];
    if (status == "exited") {
      status = line[2].replace("Exited (", "").split(')')[0];
    } else {
      numRunning++;
    }
    if (statuses[container] != status) {
      statuses[container] = status;
      emit statusChanged(container, status);
    }
  }
  if (numRunning == 0) {
    emit allStopped();
    wasStopped = true;
  } else if (wasStopped) {
    wasStopped = false;
    emit started();
  }
}
