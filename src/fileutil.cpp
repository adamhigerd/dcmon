#include "fileutil.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QProcess>
#ifdef D_USE_LUA
#include "luavm.h"
#endif

QString promptForDockerCompose()
{
  static QStringList filters({
    QString("%1 (dcmon.lua docker-compose*.yml docker-compose*.yaml)").arg(FileUtil::tr("docker-compose files")),
    QString("%1 (*.yml *.yaml)").arg(FileUtil::tr("YAML files")),
    QString("%1 (*.lua)").arg(FileUtil::tr("Lua scripts")),
    QString("%1 (*)").arg(FileUtil::tr("All files")),
  });
  return QFileDialog::getOpenFileName(nullptr, FileUtil::tr("Select docker-compose file"), QString(), filters.join(";;"));
}

QString findDockerCompose(const QString& relativeTo)
{
  QString dcFile = QDir(relativeTo).canonicalPath();
  if (dcFile.endsWith(".lua")) {
    QDir luaPath(dcFile);
    luaPath.cdUp();
    dcFile = luaPath.canonicalPath();
  }
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

#ifdef D_USE_LUA
QString findDcmonLua(const QString& relativeTo)
{
  QString luaFile = QDir(relativeTo).canonicalPath();
  if (QFileInfo(luaFile).isFile() && !luaFile.endsWith(".lua")) {
    QDir dcPath(luaFile);
    dcPath.cdUp();
    luaFile = dcPath.canonicalPath();
  }
  if (QFileInfo(luaFile).isDir()) {
    QDir base(luaFile);
    do {
      luaFile = base.absoluteFilePath("dcmon.lua");
      if (QFileInfo::exists(luaFile)) {
        break;
      }
    } while (!QFileInfo::exists(luaFile) && base.cdUp());
  }
  if (!QFileInfo::exists(luaFile)) {
    return QString();
  }
  return luaFile;
}

bool loadDcmonLua(LuaVM* lua, const QString& luaFile, QString* dcFile)
{
  QFile script(luaFile);
  if (!script.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }
  lua->evaluate(&script);
  QString yml = lua->get("yml").toString();
  if (!yml.isEmpty()) {
    QDir base(luaFile);
    base.cdUp();
    *dcFile = base.absoluteFilePath(yml);
  } else {
    *dcFile = findDockerCompose(luaFile);
  }
  return true;
}
#endif
