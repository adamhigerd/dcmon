#include "luafunction.h"
#include "luavm.h"
#include <QMetaObject>
#include <QMetaType>
#include <lualib.h>
#include <lauxlib.h>

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
  auto p = reinterpret_cast<QSharedDataPointer<LuaFunctionData>*>(lua_touserdata(L, -1));
  p->~QSharedDataPointer<LuaFunctionData>();
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
: d(new LuaFunctionData(vm))
{
  lua_pushvalue(vm->L, stackIndex);
  d->fnRef = luaL_ref(vm->L, LUA_REGISTRYINDEX);
}

void LuaFunction::initMeta(const QMetaObject* meta, const char* method)
{
  d->meta = meta;
  QByteArray signature = d->meta->normalizedSignature(method);
  if (signature[0] >= '0' && signature[0] <= '9') {
    signature = signature.mid(1);
  }
  int index = d->meta->indexOfMethod(signature);
  if (index < 0) {
    throw LuaException("LuaFunction: unknown method");
  }
  d->method = d->meta->method(index);

  static bool hasMetatable = false;
  if (!hasMetatable) {
    lua_newtable(d->lua->L);
    lua_pushcfunction(d->lua->L, LuaFunctionData::gc);
    lua_setfield(d->lua->L, -2, "__gc");
    lua_setfield(d->lua->L, LUA_REGISTRYINDEX, "LuaFunctionData::gc");
    hasMetatable = true;
  }

  void* buf = lua_newuserdata(d->lua->L, sizeof(QSharedDataPointer<LuaFunctionData>));
  new (buf) QSharedDataPointer<LuaFunctionData>(d);
  lua_getfield(d->lua->L, LUA_REGISTRYINDEX, "LuaFunctionData::gc");
  lua_setmetatable(d->lua->L, -2);

  lua_pushcclosure(d->lua->L, LuaFunction::call, 1);

  d->fnRef = luaL_ref(d->lua->L, LUA_REGISTRYINDEX);
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
  auto p = reinterpret_cast<QSharedDataPointer<LuaFunctionData>*>(lua_touserdata(L, lua_upvalueindex(1)));
  const LuaFunctionData* d = p->constData();
  QVariantList args;
  QList<QByteArray> argData;
  for (int i = n - 1; i >= 0; --i) {
    QVariant arg = lua->popStack();
    if (!arg.convert(d->method.parameterType(i))) {
      throw LuaException("LuaFunction::call: parameter type mismatch");
    }
    QMetaType argType(d->method.parameterType(i));
    QByteArray data(reinterpret_cast<const char*>(arg.constData()), argType.sizeOf());
    args.insert(0, arg);
    argData.insert(0, data);
  }
  bool ok = false;
  bool hasReturn = d->method.returnType() != QMetaType::Void;
  QList<QByteArray> argTypes = d->method.parameterTypes();

#define ARG(i) (i < n) ? QGenericArgument(argTypes[i].constData(), argData[i].constData()) : QGenericArgument()

  if (hasReturn) {
    QMetaType returnType(d->method.returnType());
    const char* returnTypeName = QMetaType::typeName(d->method.returnType());
    QVariant returnValue;
    std::vector<char> buffer(returnType.sizeOf());
    if (d->obj) {
      ok = d->method.invoke(d->obj, QGenericReturnArgument(returnTypeName, buffer.data()),
          ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9));
    } else {
      ok = d->method.invokeOnGadget(d->gadget, QGenericReturnArgument(returnTypeName, buffer.data()),
          ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9));
    }
    lua->pushStack(returnValue);
  } else {
    if (d->obj) {
      ok = d->method.invoke(d->obj, ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9));
    } else {
      ok = d->method.invokeOnGadget(d->gadget, ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9));
    }
  }

  if (!ok) {
    throw LuaException("LuaFunction::call: QMetaMethod::invoke failed");
  }

  return hasReturn ? 1 : 0;
}
