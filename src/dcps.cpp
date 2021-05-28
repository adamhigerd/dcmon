#include "dcps.h"
#include "dcmonconfig.h"

DcPs::DcPs(QObject* parent) : QTimer(parent), wasStopped(true)
{
  process.setProcessChannelMode(QProcess::MergedChannels);
  QObject::connect(&process, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
  process.setProgram("docker");
  process.setArguments(QStringList() << "ps" << "-a" << "--format" << "{{.Names}}|{{.State}}|{{.Status}}");
  setInterval(5000);
  setSingleShot(true);
  QObject::connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(start()));
  QObject::connect(this, SIGNAL(timeout()), this, SLOT(poll()));
  QObject::connect(CONFIG, SIGNAL(configChanged()), this, SLOT(reload()));

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
  terminate();
  QStringList oldNames = containerList();
  bool hasNew = false;
  statuses.clear();
  QProcess dc;
  dc.start("docker-compose", QStringList() << "-f" << CONFIG->dcFile << "ps");
  dc.waitForFinished();
  while (dc.canReadLine()) {
    QString line = dc.readLine();
    if (line[0] == '-' || line[0] == ' ' || line.startsWith("Name ")) {
      continue;
    }
    int spacePos = line.indexOf(" ");
    QString container = line.left(spacePos);
    if (CONFIG->hiddenContainers.contains(container)) {
      continue;
    }
    if (oldNames.contains(container)) {
      oldNames.removeAll(container);
    } else {
      hasNew = true;
    }
    statuses[container] = "";
  }
  process.start();
  if (hasNew || !oldNames.isEmpty()) {
    emit containerListChanged(containerList());
  }
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

QStringList DcPs::containerList() const
{
  return statuses.keys();
}
