#include "dcmonconfig.h"
#include "fileutil.h"
#include "luavm.h"
#include <QSettings>

template <typename T = std::runtime_error>
static inline void throwString(const QString& what)
{
  throw T(what.toUtf8().constData());
}

static DcmonConfig* DcmonConfig_instance = nullptr;

DcmonConfig* DcmonConfig::instance()
{
  return DcmonConfig_instance;
}

DcmonConfig::DcmonConfig()
: QObject(nullptr)
{
  DcmonConfig_instance = this;
}

void DcmonConfig::parseArgs(const QStringList& args)
{
  QString relativeTo = ".";
  bool hasFilename = false;
  bool positionalOnly = false;
  for (int i = 1; i < args.length(); i++) {
    const QString& arg = args[i];
    if (!positionalOnly && arg == "--") {
      positionalOnly = true;
    } else if (!positionalOnly && arg.startsWith("-")) {
      if (arg == "-p" || arg == "--prompt") {
        hasFilename = true;
        loadFileByExtension(promptForDockerCompose());
      } else {
        throwString(tr("Unknown flag: %1").arg(arg));
      }
    } else {
      if (hasFilename) {
        throwString(tr("Unexpected parameter: %1").arg(arg));
      }
      relativeTo = arg;
      hasFilename = true;
    }
  }
  if (luaFile.isEmpty()) {
    loadLuaFile(relativeTo);
  }
  if (dcFile.isEmpty()) {
    loadDcFile(relativeTo);
  }
  if (!hasFilename) {
    if (dcFile.isEmpty() && luaFile.isEmpty()) {
      QStringList history = openHistory();
      if (history.length() > 0) {
        loadFileByExtension(history[0], true);
      }
    }
    if (dcFile.isEmpty()) {
      loadFileByExtension(promptForDockerCompose());
    }
  }
  if (dcFile.isEmpty()) {
    throw std::runtime_error("could not find docker-compose.yml file");
  }
  rememberFile(luaFile.isEmpty() ? dcFile : luaFile);
}

void DcmonConfig::loadFileByExtension(const QString& path, bool quiet)
{
  if (path.endsWith(".lua")) {
    loadLuaFile(path, quiet);
  } else {
    loadDcFile(path, quiet);
  }
}

void DcmonConfig::loadDcFile(const QString& path, bool quiet)
{
  dcFile = findDockerCompose(path);
  if (dcFile.isEmpty()) {
    return;
  }
  int err = validateDockerCompose(dcFile, quiet);
  if (err) {
    dcFile.clear();
    if (!quiet) {
      throwString(tr("error loading %1").arg(path));
    }
  }
}

void DcmonConfig::loadLuaFile(const QString& path, bool quiet)
{
#ifdef D_USE_LUA
  luaFile = findDcmonLua(path);
  if (luaFile.isEmpty()) {
    return;
  }
  QString luaDcFile;
  if (!loadDcmonLua(&lua, luaFile, &luaDcFile)) {
    if (quiet) {
      luaFile.clear();
      return;
    }
    throw LuaException("could not read dcmon.lua file");
  }
  if (!luaDcFile.isEmpty()) {
    if (!dcFile.isEmpty() && luaDcFile != dcFile) {
      if (quiet) {
        luaFile.clear();
        return;
      }
      throw LuaException("dcmon.lua \"yml\" does not match");
    }
    try {
      loadDcFile(luaDcFile);
    } catch (...) {
      if (quiet) {
        luaFile.clear();
        dcFile.clear();
        return;
      }
      throw;
    }
  }

  LuaTable containers = lua.get("containers").value<LuaTable>();
  for (const QVariant& keyVariant : containers->keys()) {
    QString key = keyVariant.toString();
    LuaTable container = containers->get<LuaTable>(key);
    QVariant hide = container->get("hide");
    if (hide.toBool()) {
      hiddenContainers << key;
      continue;
    }
    QVariant filter = container->get("filter");
    if (filter.userType() == qMetaTypeId<LuaFunction>()) {
      filters[key] = filter.value<LuaFunction>();
    }
  }
#endif
}

QStringList DcmonConfig::openHistory() const
{
  QSettings settings;
  settings.beginGroup("history");
  QStringList history;
  for (int i = 0; i < MAX_FILE_HISTORY; i++) {
    QString key = QString("file%1").arg(i);
    if (settings.contains(key)) {
      QString path = settings.value(key).toString();
      if (path != dcFile) {
        history << path;
      }
    }
  }
  return history;
}

void DcmonConfig::rememberFile(const QString& dcFile)
{
  QStringList history = openHistory();
  history.removeAll(dcFile);
  history.insert(0, dcFile);

  QSettings settings;
  settings.beginGroup("history");
  for (int i = 0; i < MAX_FILE_HISTORY && i < history.length(); i++) {
    QString key = QString("file%1").arg(i);
    settings.setValue(key, history[i]);
  }
}

LuaFunction DcmonConfig::logFilter(const QString& container) const
{
#ifdef D_USE_LUA
  if (filters.contains(container)) {
    return filters[container];
  }
#endif
  return LuaFunction();
}
