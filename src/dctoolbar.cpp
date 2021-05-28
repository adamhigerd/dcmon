#include "dctoolbar.h"
#include <QProcess>
#include <QStyle>
#include <QtDebug>
#include <QDateTime>

DcToolBar::DcToolBar(const QString& dcFile, QWidget* parent) : QToolBar(parent), dcFile(dcFile), pollProcess(nullptr)
{
  aStartAll = addAction(style()->standardIcon(QStyle::SP_MediaPlay), tr("&Up All"), this, SLOT(startAll()));
  aRestartAll = addAction(style()->standardIcon(QStyle::SP_BrowserReload), tr("&Restart All"), this, SLOT(restartAll()));
  aStopAll = addAction(style()->standardIcon(QStyle::SP_MediaStop), tr("&Stop All"), this, SLOT(stopAll()));
  addSeparator();
  label = new QLabel(this);
  addWidget(label);
  aStartOne = addAction(style()->standardIcon(QStyle::SP_MediaPlay), "Up", this, SLOT(startOne()));
  aRestartOne = addAction(style()->standardIcon(QStyle::SP_BrowserReload), "Restart", this, SLOT(restartOne()));
  aStopOne = addAction(style()->standardIcon(QStyle::SP_MediaStop), "Stop", this, SLOT(stopOne()));
  addAction(style()->standardIcon(QStyle::SP_LineEditClearButton), "Clear", this, SIGNAL(clearOne()));
}

void DcToolBar::setCurrentContainer(const QString& name)
{
  container = name;
  label->setText("   " + name);
  statusChanged(container, statuses.contains(container) ? statuses[container] : "filter");
}

void DcToolBar::startAll()
{
  QDateTime now = QDateTime::currentDateTimeUtc();
  for (const QString& container : statuses.keys()) {
    if (statuses[container] == "running") {
      continue;
    }
    emit logMessage(now, container, "*** Start requested ***");
  }
  QProcess* p = new QProcess(this);
  pollOn(p);
  p->start("docker-compose", QStringList() << "-f" << dcFile << "up" << "--no-recreate" << "-d");
}

void DcToolBar::restartAll()
{
  QProcess* p = stopAll(true);
  QObject::connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this, p]{
    startAll();
  });
}

QProcess* DcToolBar::stopAll(bool isRestart)
{
  QDateTime now = QDateTime::currentDateTimeUtc();
  for (const QString& container : statuses.keys()) {
    if (statuses[container] != "running") {
      continue;
    }
    if (isRestart) {
      emit logMessage(now, container, "*** Restart requested ***");
    } else {
      emit logMessage(now, container, "*** Stop requested ***");
    }
  }
  QProcess* p = new QProcess(this);
  pollOn(p);
  p->start("docker-compose", QStringList() << "-f" << dcFile << "stop");
  return p;
}

void DcToolBar::startOne(const QString& name)
{
  QString container = name.isEmpty() ? this->container : name;
  emit logMessage(QDateTime::currentDateTimeUtc(), container, "*** Start requested ***");
  runThenPoll(QStringList() << "-f" << dcFile << "up" << "--no-recreate" << "-d" << container);
}

void DcToolBar::restartOne(const QString& name)
{
  QString container = name.isEmpty() ? this->container : name;
  emit logMessage(QDateTime::currentDateTimeUtc(), container, "*** Restart requested ***");

  QProcess* p = new QProcess(this);
  p->start("docker-compose", QStringList() << "-f" << dcFile << "stop" << container);
  QObject::connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this, container]{
    startOne(container);
  });
}

void DcToolBar::stopOne(const QString& name)
{
  QString container = name.isEmpty() ? this->container : name;
  emit logMessage(QDateTime::currentDateTimeUtc(), container, "*** Stop requested ***");
  runThenPoll(QStringList() << "-f" << dcFile << "stop" << container);
}

void DcToolBar::statusChanged(const QString& name, const QString& status)
{
  if (name == container && status == "filter") {
    aStartOne->setVisible(false);
    aRestartOne->setVisible(false);
    aStopOne->setVisible(false);
    aStartOne->setEnabled(false);
    aRestartOne->setEnabled(false);
    aStopOne->setEnabled(false);
    return;
  }
  statuses[name] = status;
  if (name == container) {
    aStartOne->setVisible(true);
    aRestartOne->setVisible(true);
    aStopOne->setVisible(true);
    aStartOne->setEnabled(status != "running");
    aRestartOne->setEnabled(status == "running");
    aStopOne->setEnabled(status == "running");
  }
  if (pollProcess) {
    aStartAll->setEnabled(false);
    aRestartAll->setEnabled(false);
    aStopAll->setEnabled(false);
  } else {
    bool someRunning = false;
    bool someStopped = false;
    for (const auto& status : statuses) {
      if (status == "running") {
        someRunning = true;
      } else {
        someStopped = true;
      }
      if (someRunning && someStopped) {
        break;
      }
    }
    aStartAll->setEnabled(someStopped);
    aRestartAll->setEnabled(someRunning);
    aStopAll->setEnabled(someRunning);
  }
}

void DcToolBar::pollOn(QProcess* p)
{
  if (pollProcess) {
    qDebug() << "XXX: pollOn while polling";
  }
  aStartAll->setEnabled(false);
  aRestartAll->setEnabled(false);
  aStopAll->setEnabled(false);
  p->setProcessChannelMode(QProcess::MergedChannels);
  pollProcess = p;
  QObject::connect(p, SIGNAL(readyRead()), this, SIGNAL(pollStatus()));
  QObject::connect(p, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onPollFinished()));
}

void DcToolBar::onPollFinished()
{
  pollProcess->deleteLater();
  pollProcess = nullptr;
}

void DcToolBar::runThenPoll(const QStringList& args)
{
  QProcess* p = new QProcess(this);
  QObject::connect(p, SIGNAL(finished(int,QProcess::ExitStatus)), this, SIGNAL(pollStatus()));
  QObject::connect(p, SIGNAL(finished(int,QProcess::ExitStatus)), p, SLOT(deleteLater()));
  p->start("docker-compose", args);
}
