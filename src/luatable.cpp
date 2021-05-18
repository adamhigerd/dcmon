#include "luatable.h"
#include "luavm.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

struct RefScope {
  RefScope(const LuaTableRef* t) : L(t->lua->L) {
    if (t->ref != LUA_REGISTRYINDEX && t->ref != LUA_RIDX_GLOBALS && t->ref != LUA_RIDX_MAINTHREAD) {
      lua_geti(L, LUA_REGISTRYINDEX, t->ref);
      pos = lua_gettop(L);
    } else {
      pos = t->ref;
    }
  }
  ~RefScope() {
    if (pos >= 0) {
      if (lua_gettop(L) != pos) {
        qFatal("RefScope: unbalanced stack %d %d", pos, lua_gettop(L));
      }
      lua_pop(L, 1);
    }
  }

  operator int() const { return pos; }

  lua_State* L;
  int pos;
};

LuaTableRef::LuaTableRef(LuaVM* lua)
: lua(lua)
{
  ref = luaL_ref(lua->L, LUA_REGISTRYINDEX);
}

LuaTableRef::LuaTableRef(LuaVM* lua, int ref)
: lua(lua), ref(ref)
{
  // initializers only
}

LuaTableRef::~LuaTableRef()
{
  if (ref != LUA_REGISTRYINDEX && ref != LUA_RIDX_GLOBALS && ref != LUA_RIDX_MAINTHREAD) {
    luaL_unref(lua->L, LUA_REGISTRYINDEX, ref);
  }
}

QVariantList LuaTableRef::keys() const
{
  RefScope pos(this);
  lua_pushnil(lua->L);
  QVariantList keys;
  while (lua_next(lua->L, pos)) {
    QVariant key = lua->getStack(-2);
    if (int(key.type()) == QMetaType::QByteArray) {
      keys << QString::fromUtf8(key.toByteArray());
    } else {
      keys << lua->getStack(-2);
    }
    lua_pop(lua->L, 1);
  }
  return keys;
}

QVariant LuaTableRef::get(int key) const
{
  RefScope pos(this);
  int typeID = lua_geti(lua->L, pos, key);
  return lua->popStack(typeID);
}

bool LuaTableRef::has(int key) const
{
  RefScope pos(this);
  int typeID = lua_geti(lua->L, pos, key);
  lua_pop(lua->L, 1);
  return typeID != LUA_TNONE && typeID != LUA_TNIL;
}

void LuaTableRef::set(int key, const QVariant& value)
{
  RefScope pos(this);
  lua->pushStack(value);
  lua_seti(lua->L, pos, key);
}

QVariant LuaTableRef::get(const QString& key) const
{
  RefScope pos(this);
  int typeID = lua_getfield(lua->L, pos, key.toUtf8().constData());
  return lua->popStack(typeID);
}

bool LuaTableRef::has(const QString& key) const
{
  RefScope pos(this);
  int typeID = lua_getfield(lua->L, pos, key.toUtf8().constData());
  lua_pop(lua->L, 1);
  return typeID != LUA_TNONE && typeID != LUA_TNIL;
}

void LuaTableRef::set(const QString& key, const QVariant& value)
{
  RefScope pos(this);
  lua->pushStack(value);
  lua_setfield(lua->L, pos, key.toUtf8().constData());
}

QVariant LuaTableRef::call(const QString& key, const QVariantList& args) const
{
  RefScope pos(this);
  int typeID = lua_getfield(lua->L, pos, key.toUtf8().constData());
  if (typeID == LUA_TFUNCTION) {
    int stackTop = lua_gettop(lua->L);
    for (const QVariant& arg : args) {
      lua->pushStack(arg);
    }
    return lua->call(lua_gettop(lua->L) - stackTop);
  }
  throw LuaException(QString("LuaTableRef::call: function not found"));
}

void LuaTableRef::pushStack() const
{
  lua_geti(lua->L, LUA_REGISTRYINDEX, ref);
}
