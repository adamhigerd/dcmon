#ifndef D_LUAFUNCTION_H
#define D_LUAFUNCTION_H

#include <QVariant>
#include <QMetaType>
#include <QMetaMethod>
#include <QSharedData>
#include <QPointer>
#include <lua.h>
class QObject;
class LuaVM;

class LuaFunction {
  friend class LuaVM;
public:
  template<typename T>
  static LuaFunction fromGadget(LuaVM* vm, T* gadget, const char* method) { return LuaFunction(vm, gadget->metaObject(), gadget, method); }

  LuaFunction();
  LuaFunction(LuaVM* vm, QObject* obj, const char* method);
  LuaFunction(LuaVM* vm, const QMetaObject* meta, void* gadget, const char* method);
  LuaFunction(LuaVM* vm, lua_CFunction fn);
  LuaFunction(const LuaFunction& other) = default;
  LuaFunction(LuaFunction&& other) = default;

  bool isValid() const;

  void addOverload(const char* method);

  LuaFunction& operator=(const LuaFunction& other) = default;
  LuaFunction& operator=(LuaFunction&& other) = default;
  QVariant operator()(const QVariantList& args) const;
  static QVariant firstResult(const QVariant& results);

private:
  LuaFunction(LuaVM* vm, int stackIndex);
  static int call(lua_State* L);

  void initMeta(const QMetaObject* meta, const char* method);
  void pushStack() const;

  struct LuaFunctionData : public QSharedData {
    LuaFunctionData(LuaVM* lua);
    ~LuaFunctionData();
    static int gc(lua_State* L);
    bool isValid() const;

    LuaVM* lua;
    int fnRef;
    QPointer<QObject> obj;
    const QMetaObject* meta;
    void* gadget;
    QList<QMetaMethod> methods;
  };
  using DPtr = QExplicitlySharedDataPointer<LuaFunctionData>;
  DPtr d;
};
Q_DECLARE_METATYPE(LuaFunction);

#endif
