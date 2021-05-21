#include "fileutil.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>

QString promptForDockerCompose()
{
  static QStringList filters({
    QString("%1 (docker-compose*.yml docker-compose*.yaml)").arg(FileUtil::tr("docker-compose files")),
    QString("%1 (**.yml *.yaml)").arg(FileUtil::tr("YAML files")),
    QString("%1 (*)").arg(FileUtil::tr("All files")),
  });
  return QFileDialog::getOpenFileName(nullptr, FileUtil::tr("Select docker-compose file"), QString(), filters.join(";;"));
}

QString findDockerCompose(const QString& relativeTo)
{
  QString dcFile = QDir(relativeTo).canonicalPath();
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
    return QString();
  }
  return dcFile;
}

int validateDockerCompose(const QString& dcFile, bool quiet)
{
  QProcess p;
  if (!quiet) {
    p.setProcessChannelMode(QProcess::ForwardedErrorChannel);
  }
  p.start("docker-compose", QStringList() << "-f" << dcFile << "config");
  p.waitForFinished();
  int exitCode = p.exitCode();
  if (p.exitStatus() != QProcess::NormalExit || exitCode != 0) {
    return exitCode == 0 ? 1 : exitCode;
  }
  return 0;
}

void rememberFile(const QString& dcFile)
{
  QSettings settings;
  settings.beginGroup("history");
  QStringList history({ dcFile });
  for (int i = 0; i < MAX_FILE_HISTORY; i++) {
    QString key = QString("file%1").arg(i);
    if (settings.contains(key)) {
      QString path = settings.value(key).toString();
      if (path != dcFile) {
        history << path;
      }
    }
  }
  for (int i = 0; i < MAX_FILE_HISTORY && i < history.length(); i++) {
    QString key = QString("file%1").arg(i);
    settings.setValue(key, history[i]);
  }
}

QString getLastOpenedFile()
{
  QSettings settings;
  settings.beginGroup("history");
  if (settings.contains("file0")) {
    return settings.value("file0").toString();
  }
  return QString();
}
