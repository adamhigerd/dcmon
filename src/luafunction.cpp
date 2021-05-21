#include "luafunction.h"
#include "luavm.h"
#include <QMetaObject>
#include <QMetaType>
#include <lualib.h>
#include <lauxlib.h>
#include <cstring>

static const char functionTag[] = "\0LuaFunction";

static bool validateFunctionTag(lua_State* L, int stackIndex)
{
  if (!lua_isstring(L, stackIndex)) {
    return false;
  }
  size_t len = 0;
  const char* tag = lua_tolstring(L, stackIndex, &len);
  return len == sizeof(functionTag) && std::memcmp(tag, functionTag, sizeof(functionTag)) == 0;
}

LuaFunction::LuaFunctionData::LuaFunctionData(LuaVM* lua)
: QSharedData(), lua(lua), fnRef(LUA_NOREF), obj(nullptr), meta(nullptr), gadget(nullptr)
{
  // initializers only
}

LuaFunction::LuaFunctionData::~LuaFunctionData()
{
  luaL_unref(lua->L, LUA_REGISTRYINDEX, fnRef);
}

int LuaFunction::LuaFunctionData::gc(lua_State* L)
{
  auto p = reinterpret_cast<DPtr*>(lua_touserdata(L, -1));
  p->~DPtr();
  return 0;
}

LuaFunction::LuaFunction()
: d(nullptr)
{
  // initializers only
}

LuaFunction::LuaFunction(LuaVM* vm, QObject* obj, const char* method)
: d(new LuaFunctionData(vm))
{
  d->obj = obj;
  initMeta(obj->metaObject(), method);
}

LuaFunction::LuaFunction(LuaVM* vm, const QMetaObject* meta, void* gadget, const char* method)
: d(new LuaFunctionData(vm))
{
  d->gadget = gadget;
  initMeta(meta, method);
}

LuaFunction::LuaFunction(LuaVM* vm, lua_CFunction fn)
: d(new LuaFunctionData(vm))
{
  lua_pushcfunction(vm->L, fn);
  d->fnRef = luaL_ref(vm->L, LUA_REGISTRYINDEX);
}

LuaFunction::LuaFunction(LuaVM* vm, int stackIndex)
{
  const char* upvalue = lua_getupvalue(vm->L, stackIndex, 1);
  bool hasMeta = upvalue && validateFunctionTag(vm->L, -1);
  if (upvalue) {
    lua_pop(vm->L, 1);
  }
  if (hasMeta) {
    // This is a bound metaobject function
    upvalue = lua_getupvalue(vm->L, stackIndex, 2);
    auto p = reinterpret_cast<DPtr*>(lua_touserdata(vm->L, -1));
    if (!p) {
      throw LuaException("LuaFunction: corrupt function");
    }
    if (upvalue) {
      lua_pop(vm->L, 1);
    }
    d = *p;
  } else {
    // This is an opaque lua_CFunction
    d = new LuaFunctionData(vm);
    lua_pushvalue(vm->L, stackIndex);
    d->fnRef = luaL_ref(vm->L, LUA_REGISTRYINDEX);
  }
}

void LuaFunction::initMeta(const QMetaObject* meta, const char* method)
{
  d->meta = meta;

  static bool hasMetatable = false;
  if (!hasMetatable) {
    lua_newtable(d->lua->L);
    lua_pushcfunction(d->lua->L, LuaFunctionData::gc);
    lua_setfield(d->lua->L, -2, "__gc");
    lua_setfield(d->lua->L, LUA_REGISTRYINDEX, "LuaFunctionData::gc");
    hasMetatable = true;
  }

  lua_pushlstring(d->lua->L, functionTag, sizeof(functionTag));
  void* buf = lua_newuserdata(d->lua->L, sizeof(DPtr));
  new (buf) DPtr(d);
  lua_getfield(d->lua->L, LUA_REGISTRYINDEX, "LuaFunctionData::gc");
  lua_setmetatable(d->lua->L, -2);

  lua_pushcclosure(d->lua->L, LuaFunction::call, 2);

  d->fnRef = luaL_ref(d->lua->L, LUA_REGISTRYINDEX);

  addOverload(method);
}

void LuaFunction::addOverload(const char* method)
{
  if (!d->meta) {
    throw LuaException("LuaFunction: cannot overload non-QMetaObject functions");
  }
  QByteArray signature = d->meta->normalizedSignature(method);
  if (signature[0] >= '0' && signature[0] <= '9') {
    signature = signature.mid(1);
  }
  int index = d->meta->indexOfMethod(signature);
  if (index < 0) {
    throw LuaException("LuaFunction: unknown method");
  }
  QMetaMethod mmethod = d->meta->method(index);
  if (d->methods.length() > 0) {
    if (d->methods.first().name() != mmethod.name()) {
      throw LuaException("LuaFunction: overloads do not refer to the same method");
    }
  }
  d->methods << mmethod;
}

QVariant LuaFunction::operator()(const QVariantList& args) const
{
  if (!d) {
    throw LuaException("LuaFunction::operator(): no function");
  }
  int stackTop = lua_gettop(d->lua->L);
  pushStack();
  for (const QVariant& arg : args) {
    d->lua->pushStack(arg);
  }
  return d->lua->call(stackTop);
}

void LuaFunction::pushStack() const
{
  lua_geti(d->lua->L, LUA_REGISTRYINDEX, d->fnRef);
}

int LuaFunction::call(lua_State* L)
{
  LuaVM* lua = LuaVM::instance(L);
  int n = lua_gettop(L);
  auto p = reinterpret_cast<DPtr*>(lua_touserdata(L, lua_upvalueindex(2)));
  if (!p || !validateFunctionTag(L, lua_upvalueindex(1))) {
    throw LuaException("LuaFunction: call to corrupted function");
  }
  const LuaFunctionData* d = p->constData();
  QVariantList args;
  for (int i = n - 1; i >= 0; --i) {
    args.insert(0, lua->popStack());
  }
  QVariantList convertedArgs;
  QList<QByteArray> argData;
  QMetaMethod method;
  QList<QByteArray> argTypes;
  bool found = false;
  for (const QMetaMethod& candidate : d->methods) {
    method = candidate;
    found = true;
    for (int i = 0; i < args.length(); i++) {
      convertedArgs << args[i];
      QVariant& arg = convertedArgs.back();
      if (!arg.convert(method.parameterType(i))) {
        found = false;
        break;
      }
      convertedArgs << arg;
      QMetaType argType(method.parameterType(i));
      QByteArray data(reinterpret_cast<const char*>(arg.constData()), argType.sizeOf());
      argTypes << QMetaType::typeName(method.parameterType(i));
      argData << data;
    }
    if (found) {
      break;
    } else {
      convertedArgs.clear();
      argTypes.clear();
      argData.clear();
    }
  }
  if (!found) {
    throw LuaException("LuaFunction::call: no matching overload found");
  }
  bool ok = false;
  bool hasReturn = method.returnType() != QMetaType::Void;

#define ARG(i) (i < n) ? QGenericArgument(argTypes[i].constData(), argData[i].constData()) : QGenericArgument()

  if (hasReturn) {
    QMetaType returnType(method.returnType());
    const char* returnTypeName = QMetaType::typeName(method.returnType());
    QVariant returnValue;
    std::vector<char> buffer(returnType.sizeOf());
    if (d->obj) {
      ok = method.invoke(d->obj, QGenericReturnArgument(returnTypeName, buffer.data()),
          ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9));
    } else {
      ok = method.invokeOnGadget(d->gadget, QGenericReturnArgument(returnTypeName, buffer.data()),
          ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9));
    }
    lua->pushStack(returnValue);
  } else {
    if (d->obj) {
      ok = method.invoke(d->obj, ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9));
    } else {
      ok = method.invokeOnGadget(d->gadget, ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9));
    }
  }

  if (!ok) {
    throw LuaException("LuaFunction::call: QMetaMethod::invoke failed");
  }

  return hasReturn ? 1 : 0;
}
