#include <QApplication>
#include <QtDebug>
#include "dcmonconfig.h"
#include "dcmonwindow.h"

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

  DcmonWindow win;
  win.resize(1024, 768);
  win.show();
  return app.exec();
}
