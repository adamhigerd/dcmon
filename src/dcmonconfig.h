#ifndef D_DCMONCONFIG_H
#define D_DCMONCONFIG_H

#include <QObject>
#include <QSet>
#include <functional>
#include "luavm.h"
class QFileSystemWatcher;

#define MAX_FILE_HISTORY 4
#define CONFIG DcmonConfig::instance()

class DcmonConfig : public QObject {
Q_OBJECT
public:
  static DcmonConfig* instance();

  DcmonConfig();

  void parseArgs(const QStringList& args);

  QStringList openHistory() const;

  QSet<QString> hiddenContainers;
  QHash<QString, LuaFunction> filterViews;
  LuaFunction logFilter(const QString& container) const;

  QString dcFile, luaFile;

signals:
  void filesUpdated();
  void configChanged();

public slots:
  void reloadConfig();

private:
  void loadFileByExtension(const QString& path, bool quiet = false);
  void loadDcFile(const QString& path, bool quiet = false);
  void loadLuaFile(const QString& path, bool quiet = false);
  void rememberFile(const QString& dcFile);
  void initConfig();

#ifdef D_USE_LUA
  LuaVM lua;
  QHash<QString, LuaFunction> filters;
#endif

  QFileSystemWatcher* watcher;
};

#endif
