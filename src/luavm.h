#if defined(D_USE_LUA) && !defined(D_LUAVM_H)
#define D_LUAVM_H

#include <QObject>
#include <QSharedPointer>
#include <QVariant>
#include <QIODevice>
#include <QMetaObject>
#include <stdexcept>
#include "luatable.h"
#include "luafunction.h"
class QIODevice;
struct lua_State;

class LuaException : public std::runtime_error {
public:
  LuaException(const QVariant& what);
  LuaException(LuaVM* lua);

  QVariant errorObject;
};

class LuaVM : public QObject, public LuaTableRef {
Q_OBJECT
friend class LuaException;
friend class LuaTableRef;
friend class LuaFunction;
friend class RefScope;
public:
  static LuaVM* instance(lua_State* L);
  LuaVM(QObject* parent = nullptr);
  ~LuaVM();

  QVariant evaluate(const char* source);
  inline QVariant evaluate(const QByteArray& source) { return evaluate(source.constData()); }
  inline QVariant evaluate(const QString& source) { return evaluate(source.toUtf8()); }
  QVariant evaluate(QIODevice* source);

  LuaTableRef registry;
  LuaTable newTable();
  LuaTable bindObject(QObject* obj);
  template <typename T> LuaTable bindGadget(T* gadget) { return bindGadget(gadget->metaObject(), gadget); }

private:
  LuaTable bindGadget(const QMetaObject* meta, void* gadget);
  QVariant getStack(int stackSlot = -1) const;
  QVariant getStack(int stackSlot, int typeID) const;
  QVariant popStack();
  QVariant popStack(int typeID);
  void pushStack(const QVariant& value);
  QVariant call(int stackTop, int err = 0);

  lua_State* L;

  static int atPanic(lua_State* L);
};

#endif
