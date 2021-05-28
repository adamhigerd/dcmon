#ifndef D_LUATABLE_H
#define D_LUATABLE_H

#include <QVariant>
#include <QMetaType>
#include <QPointer>
#include "luafunction.h"
class LuaVM;

class LuaTableRef;
using LuaTable = QSharedPointer<LuaTableRef>;

class LuaTableRef {
friend class LuaVM;
friend class RefScope;
public:
  ~LuaTableRef();

  QVariantList keys() const;

  bool has(int key) const;
  QVariant get(int key) const;
  inline void set(int key, const LuaFunction& value);
  inline void set(int key, const LuaTable& value);
  void set(int key, const QVariant& value);

  bool has(const QString& key) const;
  QVariant get(const QString& key) const;
  inline void set(const QString& key, const LuaFunction& value);
  inline void set(const QString& key, const LuaTable& value);
  void set(const QString& key, const QVariant& value);
  QVariant call(const QString& key, const QVariantList& args) const;

  template <typename T>
  inline T get(int key) const { return get(key).value<T>(); }
  template <typename T>
  inline T get(const QString& key) const { return get(key).value<T>(); }

  // TODO: QVariant keys? Template keys?

private:
  LuaTableRef(LuaVM* lua);
  LuaTableRef(LuaVM* lua, int ref);

  void pushStack() const;

  QPointer<LuaVM> lua;
  int ref;
};

Q_DECLARE_METATYPE(LuaTable);

inline void LuaTableRef::set(int key, const LuaFunction& value) { set(key, QVariant::fromValue(value)); }
inline void LuaTableRef::set(int key, const LuaTable& value) { set(key, QVariant::fromValue(value)); }
inline void LuaTableRef::set(const QString& key, const LuaFunction& value) { set(key, QVariant::fromValue(value)); }
inline void LuaTableRef::set(const QString& key, const LuaTable& value) { set(key, QVariant::fromValue(value)); }

#endif
