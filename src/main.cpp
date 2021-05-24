#include <QApplication>
#include <QMainWindow>
#include <QStyle>
#include <QTreeView>
#include <QtDebug>
#include "dcmonconfig.h"
#include "dclog.h"
#include "dcps.h"
#include "dclogview.h"
#include "dctoolbar.h"
#include "fileutil.h"

int main(int argc, char** argv) {
  QApplication::setApplicationName("dcmon");
  QApplication::setApplicationVersion("0.0.1");
  QApplication::setOrganizationName("Alkahest");
  QApplication::setOrganizationDomain("com.alkahest");
  QApplication app(argc, argv);
  DcmonConfig config;

  try {
    config.parseArgs(app.arguments());
  } catch (std::exception& e) {
    qWarning("%s: %s", argv[0], e.what());
    return 1;
  }

  QMainWindow win;
  win.setWindowIcon(win.style()->standardIcon(QStyle::SP_MediaPause));
  win.setWindowTitle(QString("dcmon - %1").arg(config.dcFile));
  // TODO: menu bar

  DcToolBar tb(config.dcFile);
  win.addToolBar(&tb);

  DcLogView view;
  win.setCentralWidget(&view);
  QObject::connect(&tb, SIGNAL(clearOne()), &view, SLOT(clearCurrent()));
  QObject::connect(&view, SIGNAL(currentContainerChanged(QString)), &tb, SLOT(setCurrentContainer(QString)));

  DcPs ps;
  QObject::connect(&ps, SIGNAL(statusChanged(QString,QString)), &view, SLOT(statusChanged(QString,QString)));
  QObject::connect(&ps, SIGNAL(statusChanged(QString,QString)), &tb, SLOT(statusChanged(QString,QString)));
  QObject::connect(&tb, SIGNAL(pollStatus()), &ps, SLOT(poll()));
  QObject::connect(&app, SIGNAL(aboutToQuit()), &ps, SLOT(terminate()));

  DcLog logger;
  QObject::connect(&tb, SIGNAL(logMessage(QDateTime,QString,QString)), &view, SLOT(logMessage(QDateTime,QString,QString)));
  QObject::connect(&logger, SIGNAL(logMessage(QDateTime,QString,QString)), &view, SLOT(logMessage(QDateTime,QString,QString)));
  QObject::connect(&app, SIGNAL(aboutToQuit()), &logger, SLOT(terminate()));
  QObject::connect(&ps, SIGNAL(allStopped()), &logger, SLOT(pause()));
  QObject::connect(&ps, SIGNAL(started()), &logger, SLOT(start()));

  win.resize(1024, 768);
  win.show();
  return app.exec();
}
