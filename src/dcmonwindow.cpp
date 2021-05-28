#include "dcmonwindow.h"
#include "dcmonconfig.h"
#include "dctoolbar.h"
#include "dcps.h"
#include "dclogview.h"
#include "dclog.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QPalette>

DcmonWindow::DcmonWindow(QWidget* parent) : QMainWindow(parent)
{
  setWindowIcon(style()->standardIcon(QStyle::SP_MediaPause));
  setWindowTitle(QString("dcmon - %1").arg(CONFIG->dcFile));
  // TODO: menu bar

  tb = new DcToolBar(CONFIG->dcFile, this);
  addToolBar(tb);

  QWidget* main = new QWidget(this);
  setCentralWidget(main);
  QVBoxLayout* layout = new QVBoxLayout(main);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(1);

  notify = new QLabel(this);
  QPalette p = notify->palette();
  p.setColor(QPalette::Window, QColor(255, 255, 222));
  p.setColor(QPalette::WindowText, Qt::black);
  notify->setAutoFillBackground(true);
  notify->setPalette(p);
  notify->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  notify->setText(tr("The configuration has been modified. <a href='#'>Reload</a>"));
  notify->setMargin(style()->pixelMetric(QStyle::PM_ButtonMargin, nullptr, notify));
  layout->addWidget(notify, 0);
  QObject::connect(notify, SIGNAL(linkActivated(QString)), this, SLOT(reloadConfig()));
  notify->hide();

  view = new DcLogView(this);
  layout->addWidget(view, 1);
  QObject::connect(tb, SIGNAL(clearOne()), view, SLOT(clearCurrent()));
  QObject::connect(view, SIGNAL(currentContainerChanged(QString)), tb, SLOT(setCurrentContainer(QString)));

  ps = new DcPs(this);
  QObject::connect(ps, SIGNAL(statusChanged(QString,QString)), view, SLOT(statusChanged(QString,QString)));
  QObject::connect(ps, SIGNAL(statusChanged(QString,QString)), tb, SLOT(statusChanged(QString,QString)));
  QObject::connect(ps, SIGNAL(containerListChanged(QStringList)), view, SLOT(containerListChanged(QStringList)));
  QObject::connect(tb, SIGNAL(pollStatus()), ps, SLOT(poll()));
  QObject::connect(qApp, SIGNAL(aboutToQuit()), ps, SLOT(terminate()));

  logger = new DcLog(this);
  QObject::connect(tb, SIGNAL(logMessage(QDateTime,QString,QString)), view, SLOT(logMessage(QDateTime,QString,QString)));
  QObject::connect(logger, SIGNAL(logMessage(QDateTime,QString,QString)), view, SLOT(logMessage(QDateTime,QString,QString)));
  QObject::connect(qApp, SIGNAL(aboutToQuit()), logger, SLOT(terminate()));
  QObject::connect(ps, SIGNAL(allStopped()), logger, SLOT(pause()));
  QObject::connect(ps, SIGNAL(started()), logger, SLOT(start()));

  QObject::connect(CONFIG, SIGNAL(filesUpdated()), this, SLOT(filesUpdated()));
}

void DcmonWindow::reloadConfig()
{
  CONFIG->reloadConfig();
  notify->hide();
}

void DcmonWindow::filesUpdated()
{
  notify->show();
}
