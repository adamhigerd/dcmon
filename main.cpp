#include <QApplication>
#include <QMainWindow>
#include <QStyle>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include "dclog.h"
#include "dcps.h"
#include "dclogview.h"
#include "dctoolbar.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  QString dcFile = ".";
  {
    QStringList args(app.arguments());
    if (args.length() > 1) {
      dcFile = args[1];
    }
    dcFile = QDir(dcFile).canonicalPath();
    if (QFileInfo(dcFile).isDir()) {
      QDir base(dcFile);
      do {
        dcFile = base.absoluteFilePath("docker-compose.yml");
        if (QFileInfo::exists(dcFile)) {
          break;
        }
        dcFile = base.absoluteFilePath("docker-compose.yaml");
        if (QFileInfo::exists(dcFile)) {
          break;
        }
      } while (base.cdUp());
    }
    if (!QFileInfo::exists(dcFile)) {
      qWarning("%s: could not find docker-compose file", argv[0]);
      return 1;
    }

    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    p.start("docker-compose", QStringList() << "-f" << dcFile << "config");
    p.waitForFinished();
    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
      return p.exitCode();
    }
  }

  QMainWindow win;
  win.setWindowIcon(win.style()->standardIcon(QStyle::SP_MediaPause));

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

  win.resize(1024, 768);
  win.show();
  return app.exec();
}
