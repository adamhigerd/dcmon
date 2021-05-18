#include "luavm.h"
#include <QFileDevice>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

LuaException::LuaException(const QVariant& what)
: std::runtime_error(what.toString().toUtf8().constData()), errorObject(what)
{
  // initializers only
}

LuaException::LuaException(LuaVM* lua)
: LuaException(lua->popStack())
{
  // forwarded constructor only
}

int LuaVM::atPanic(lua_State* L)
{
  int typeID = lua_getfield(L, LUA_REGISTRYINDEX, "LuaVM_atPanic");
  if (typeID == LUA_TLIGHTUSERDATA) {
    LuaVM* vm = reinterpret_cast<LuaVM*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    throw LuaException(vm);
  }
  lua_pop(L, 1);
  throw LuaException(QString::fromUtf8(lua_tostring(L, -1)));
}

LuaVM::LuaVM(QObject* parent)
: QObject(parent), LuaTableRef(this, LUA_RIDX_GLOBALS), registry(this, LUA_REGISTRYINDEX)
{
  L = luaL_newstate();
  luaL_openlibs(L);
  lua_atpanic(L, &LuaVM::atPanic);
  registry.set("LuaVM_atPanic", QVariant::fromValue<void*>(this));
}

LuaVM::~LuaVM()
{
  lua_close(L);
  L = nullptr;
}

QVariant LuaVM::getStack(int stackSlot) const
{
  return getStack(stackSlot, lua_type(L, stackSlot));
}

QVariant LuaVM::getStack(int stackSlot, int typeID) const
{
  switch (typeID) {
    case LUA_TNIL:
      return QVariant();
    case LUA_TNUMBER:
      if (lua_isinteger(L, stackSlot)) {
        return lua_tointeger(L, stackSlot);
      } else {
        return lua_tonumber(L, stackSlot);
      }
    case LUA_TBOOLEAN:
      return bool(lua_toboolean(L, stackSlot));
    case LUA_TSTRING:
      return QByteArray(lua_tostring(L, stackSlot));
    case LUA_TUSERDATA:
    case LUA_TLIGHTUSERDATA:
      // TODO: distinguish heavy userdata
      return QVariant::fromValue(lua_touserdata(L, stackSlot));
    case LUA_TTABLE:
      lua_pushvalue(L, stackSlot);
      return QVariant::fromValue(LuaTable(new LuaTableRef(const_cast<LuaVM*>(this))));
    case LUA_TFUNCTION:
    case LUA_TTHREAD:
    default:
      qFatal("LuaVM::getStack: unknown/unsupported Lua type");
  }
}

QVariant LuaVM::popStack()
{
  return popStack(lua_type(L, -1));
}

QVariant LuaVM::popStack(int typeID)
{
  QVariant result = getStack(-1, typeID);
  lua_pop(L, 1);
  return result;
}

void LuaVM::pushStack(const QVariant& value)
{
  int type = value.type();
  switch (type) {
    case QMetaType::Nullptr:
    case QMetaType::UnknownType:
      lua_pushnil(L);
      break;
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
      lua_pushinteger(L, value.value<int>());
      break;
    case QMetaType::Float:
    case QMetaType::Double:
      lua_pushnumber(L, value.value<double>());
      break;
    case QMetaType::Bool:
      lua_pushboolean(L, value.value<bool>());
      break;
    case QMetaType::QString:
      lua_pushstring(L, value.value<QString>().toUtf8().constData());
      break;
    case QMetaType::QByteArray:
      lua_pushstring(L, value.value<QByteArray>().constData());
      break;
    case QMetaType::VoidStar:
      // TODO: heavy userdata
      lua_pushlightuserdata(L, value.value<void*>());
      break;
    default:
      if (type == qMetaTypeId<LuaTable>()) {
        value.value<LuaTable>()->pushStack();
      } else {
        qFatal("LuaVM::pushStack: unsupported Qt type");
      }
  }
}

QVariant LuaVM::evaluate(QIODevice* source)
{
  QString filename = QString("(%1)").arg(source->metaObject()->className());
  QFileDevice* file = qobject_cast<QFileDevice*>(source);
  if (file) {
    filename = file->fileName();
  }
  int stackTop = lua_gettop(L);
  QByteArray buf = source->readAll();
  return call(stackTop, luaL_loadbuffer(L, buf, buf.size(), filename.toUtf8().constData()));
}

QVariant LuaVM::evaluate(const char* source)
{
  int stackTop = lua_gettop(L);
  return call(stackTop, luaL_loadstring(L, source));
}

QVariant LuaVM::call(int stackTop, int err)
{
  switch (err) {
    case LUA_ERRSYNTAX:
      throw LuaException("Syntax error during compilation");
    case LUA_ERRMEM:
      throw LuaException("Memory allocation error");
    case LUA_ERRGCMM:
      throw LuaException("Error during garbage collection");
  }

  lua_call(lua->L, lua_gettop(lua->L) - stackTop - 1, LUA_MULTRET);
  int numResults = lua_gettop(lua->L) - stackTop;
  if (numResults == 0) {
    return QVariant();
  } else if (numResults == 1) {
    return lua->popStack();
  }
  QVariantList results;
  while (numResults) {
    results.insert(0, lua->popStack());
    --numResults;
  }
  return results;
}
