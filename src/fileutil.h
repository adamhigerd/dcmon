#ifndef D_FILEUTIL_H
#define D_FILEUTIL_H

#include <QCoreApplication>
#include <QString>
#include <QDir>
class LuaVM;

#define MAX_FILE_HISTORY 4

class FileUtil {
  Q_DECLARE_TR_FUNCTIONS(FileUtil)

private:
  // This is a utility class for translation purposes.
  // It cannot be instantiated.
  FileUtil() = delete;
};

QString promptForDockerCompose();
QString findDockerCompose(const QString& relativeTo);
int validateDockerCompose(const QString& dcFile, bool quiet = false);

#ifdef D_USE_LUA
QString findDcmonLua(const QString& relativeTo);
bool loadDcmonLua(LuaVM* lua, const QString& luaFile, QString* dcFile);
#endif

#endif
