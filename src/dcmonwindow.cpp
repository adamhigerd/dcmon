#include "dcmonwindow.h"
#include "dcmonconfig.h"
#include "dctoolbar.h"
#include "dcps.h"
#include "dclogview.h"
#include "dclog.h"
#include "fileutil.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QPalette>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

DcmonWindow::DcmonWindow(QWidget* parent) : QMainWindow(parent)
{
  setWindowIcon(style()->standardIcon(QStyle::SP_MediaPause));
  setWindowTitle(QString("dcmon - %1").arg(CONFIG->dcFile));

  tb = new DcToolBar(CONFIG->dcFile, this);
  addToolBar(tb);

  QMenuBar* menu = new QMenuBar(this);
  setMenuBar(menu);

  QMenu* file = menu->addMenu(tr("&File"));
  file->addAction(tr("&Open..."), this, SLOT(openDialog()));
  QMenu* recents = file->addMenu(tr("Recent Projects"));
  int i = 0;
  for (const QString& history : CONFIG->openHistory()) {
    ++i;
    QAction* action = recents->addAction(tr("[&%1] %2").arg(i).arg(history), this, SLOT(openHistory()));
    action->setData(history);
  }
  file->addSeparator();
  file->addAction(tr("&Reload"), this, SLOT(reloadConfig()));
  file->addSeparator();
  file->addAction(tr("E&xit"), qApp, SLOT(quit()));

  QMenu* ctr = menu->addMenu("&Containers");
  ctr->addAction(tb->aStartAll);
  ctr->addAction(tb->aRestartAll);
  ctr->addAction(tb->aStopAll);

  QMenu* help = menu->addMenu(tr("&Help"));
  help->addAction(tr("Visit &Website"), this, SLOT(visitWebsite()));
  help->addSeparator();
  help->addAction(tr("&About..."), this, SLOT(aboutDialog()));
  help->addAction(tr("About &Qt..."), qApp, SLOT(aboutQt()));

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

void DcmonWindow::openHistory()
{
  QAction* action = qobject_cast<QAction*>(sender());
  if (action) {
    open(action->data().toString());
  }
}

void DcmonWindow::openDialog()
{
  QString path = promptForDockerCompose();
  if (!path.isEmpty()) {
    open(path);
  }
}

void DcmonWindow::open(const QString& path)
{
  QMessageBox::warning(this, "dcmon", path + "\n\nSorry, loading files at runtime is unimplemented.");
}

void DcmonWindow::aboutDialog()
{
  QMessageBox::about(this, "dcmon", "dcmon \u00a9 2021 Flight Centre Travel Group");
}

void DcmonWindow::visitWebsite()
{
  QDesktopServices::openUrl(QUrl("https://github.com/wherefortravel/dcmon"));
}
