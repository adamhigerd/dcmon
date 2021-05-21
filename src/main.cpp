#include <QApplication>
#include <QMainWindow>
#include <QStyle>
#include <QTreeView>
#include "dclog.h"
#include "dcps.h"
#include "dclogview.h"
#include "dctoolbar.h"
#include "fileutil.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  QString dcFile;
  {
    QStringList args(app.arguments());
    bool hasFilename = args.length() > 1;
    dcFile = findDockerCompose(hasFilename ? args[1] : ".");
    if (!hasFilename && dcFile.isEmpty()) {
      dcFile = getLastOpenedFile();
      if (!dcFile.isEmpty()) {
        int err = validateDockerCompose(dcFile, true);
        if (err) {
          dcFile.clear();
        }
      }
      if (dcFile.isEmpty()) {
        dcFile = promptForDockerCompose();
        if (dcFile.isEmpty()) {
          // User cancelled
          return 0;
        }
      }
    }
    if (dcFile.isEmpty()) {
      qWarning("%s: could not find docker-compose file", argv[0]);
      return 1;
    }

    int err = validateDockerCompose(dcFile);
    if (err) {
      return err;
    }

    rememberFile(dcFile);
  }

  QMainWindow win;
  win.setWindowIcon(win.style()->standardIcon(QStyle::SP_MediaPause));
  win.setWindowTitle(QString("dcmon - %1").arg(dcFile));
  // TODO: menu bar

  DcToolBar tb(dcFile);
  win.addToolBar(&tb);

  DcLogView view;
  win.setCentralWidget(&view);
  QObject::connect(&tb, SIGNAL(clearOne()), &view, SLOT(clearCurrent()));
  QObject::connect(&view, SIGNAL(currentContainerChanged(QString)), &tb, SLOT(setCurrentContainer(QString)));

  DcPs ps(dcFile);
  QObject::connect(&ps, SIGNAL(statusChanged(QString,QString)), &view, SLOT(statusChanged(QString,QString)));
  QObject::connect(&ps, SIGNAL(statusChanged(QString,QString)), &tb, SLOT(statusChanged(QString,QString)));
  QObject::connect(&tb, SIGNAL(pollStatus()), &ps, SLOT(poll()));
  QObject::connect(&app, SIGNAL(aboutToQuit()), &ps, SLOT(terminate()));

  DcLog logger(dcFile);
  QObject::connect(&tb, SIGNAL(logMessage(QDateTime,QString,QString)), &view, SLOT(logMessage(QDateTime,QString,QString)));
  QObject::connect(&logger, SIGNAL(logMessage(QDateTime,QString,QString)), &view, SLOT(logMessage(QDateTime,QString,QString)));
  QObject::connect(&app, SIGNAL(aboutToQuit()), &logger, SLOT(terminate()));
  QObject::connect(&ps, SIGNAL(allStopped()), &logger, SLOT(pause()));
  QObject::connect(&ps, SIGNAL(started()), &logger, SLOT(start()));

  win.resize(1024, 768);
  win.show();
  return app.exec();
}
