#include "treelogmodel.h"
#include <QtDebug>

TreeLogModel::LogLine::LogLine()
: parent(nullptr)
{
  // initializers only
}

TreeLogModel::LogLine::LogLine(LogLine* parent, const QString& msg, int indent)
: parent(parent), line(msg), indent(indent)
{
  // initializers only
}

TreeLogModel::LogLine::LogLine(LogLine* parent, const QDateTime& dt, const QString& msg)
: datetime(dt), parent(parent), line(msg), indent(0)
{
  // initializers only
}

TreeLogModel::TreeLogModel(QObject* parent)
: QAbstractItemModel(parent), _maxLines(10000)
{
  // initializers only
}

TreeLogModel::~TreeLogModel()
{
  qDeleteAll(roots);
}

TreeLogModel::LogLine::~LogLine()
{
  qDeleteAll(children);
}

int TreeLogModel::maxLines() const
{
  return _maxLines;
}

void TreeLogModel::setMaxLines(int lines)
{
  _maxLines = lines;
  for (const QString& name : names) {
    flushOldest(name);
  }
}

void TreeLogModel::flushOldest(const QString& container)
{
  LogLine& root = *roots[container];
  int row = names.indexOf(container);
  int ct = root.children.size() - _maxLines;
  if (ct <= 0) {
    return;
  }
  beginRemoveRows(index(row, 0, QModelIndex()), 0, ct - 1);
  for (int i = 0; i < ct; i++) {
    delete root.children[i];
  }
  root.children.erase(root.children.begin(), root.children.begin() + ct);
  endRemoveRows();
}

void TreeLogModel::addContainer(const QString& container)
{
  if (roots.contains(container)) {
    return;
  }
  beginInsertRows(QModelIndex(), names.size(), names.size());
  names << container;
  roots[container] = new LogLine();
  endInsertRows();
}

void TreeLogModel::logMessage(const QDateTime& timestamp, const QString& container, const QString& message)
{
  addContainer(container);
  LogLine& root = *roots[container];
  int indent = 0;
  while (indent < message.size() && message[indent].isSpace()) {
    ++indent;
  }
  if (indent == 0 || !root.children.size()) {
    beginInsertRows(index(&root, 0), root.children.size(), root.children.size());
    root.children.push_back(new LogLine(&root, timestamp, message));
    endInsertRows();
  } else {
    LogLine* parent = root.children.back();
    LogLine* child = parent;
    while (child->indent < indent) {
      parent = child;
      if (!parent->children.size()) {
        break;
      }
      child = parent->children.back();
    }
    if (child->indent < indent) {
      parent = child;
    }
    beginInsertRows(index(parent, 0), parent->children.size(), parent->children.size());
    parent->children.push_back(new LogLine(parent, message, indent));
    endInsertRows();
  }
  flushOldest(container);
}

#define line_cast(p) const_cast<void*>((void*)static_cast<const TreeLogModel::LogLine*>(p))
#define idx_cast(p) reinterpret_cast<TreeLogModel::LogLine*>(p.internalPointer())

QModelIndex TreeLogModel::index(int row, int column, const QModelIndex& parent) const
{
  if (!parent.isValid()) {
    if (row < names.size()) {
      return createIndex(row, column, nullptr);
    }
    return QModelIndex();
  }
  LogLine* line = lineForIndex(parent);
  if (!line || (quint64)row >= line->children.size()) {
    return QModelIndex();
  }
  return createIndex(row, column, line_cast(line->children[row]));
}

QModelIndex TreeLogModel::parent(const QModelIndex& idx) const
{
  if (!idx.isValid() || !idx.internalPointer()) {
    return QModelIndex();
  }
  LogLine* line = lineForIndex(idx);
  return index(line->parent, 0);
}

QModelIndex TreeLogModel::index(LogLine* line, int column) const
{
  if (!line->parent) {
    return createIndex(names.indexOf(roots.key(line)), 0, nullptr);
  }
  int row = find(line->parent, line);
  if (row < 0) {
    return QModelIndex();
  }
  return createIndex(row, column, line);
}

int TreeLogModel::find(const LogLine* parent, const LogLine* child) const
{
  for (int i = parent->children.size() - 1; i >= 0; --i) {
    if (parent->children[i] == child) {
      return i;
    }
  }
  return -1;
}

int TreeLogModel::rowCount(const QModelIndex& parent) const
{
  if (!parent.isValid()) {
    return names.size();
  }
  LogLine* line = lineForIndex(parent);
  return line->children.size();
}

int TreeLogModel::columnCount(const QModelIndex& parent) const
{
  return rowCount(parent) ? 2 : 0;
}

QVariant TreeLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Vertical || role != Qt::DisplayRole) {
    return QAbstractItemModel::headerData(section, orientation, role);
  }
  if (section == 0) {
    return "Timestamp";
  } else if (section == 1) {
    return "Message";
  } else {
    return QVariant();
  }
}

QVariant TreeLogModel::data(const QModelIndex& index, int role) const
{
  if (role == Qt::FontRole && index.column() == 1) {
    return _logFont;
  }
  LogLine* line = lineForIndex(index);
  if (index.column() == 0) {
    if (!index.parent().isValid()) {
      if (role == Qt::DisplayRole) {
        return names[index.row()];
      }
      return QVariant();
    }
    if (!line->parent || role != Qt::DisplayRole) {
      return QVariant();
    }
    return line->datetime.toString("hh:mm:ss");
  }
  if (role == Qt::DisplayRole) {
    return line->line;
  }
  return QVariant();
}

TreeLogModel::LogLine* TreeLogModel::lineForIndex(const QModelIndex& parent) const
{
  if (!parent.internalPointer()) {
    return roots[names[parent.row()]];
  } else {
    return idx_cast(parent);
  }
}

QModelIndex TreeLogModel::rootForContainer(const QString& name) const
{
  int row = names.indexOf(name);
  if (row < 0) {
    return QModelIndex();
  }
  return createIndex(row, 0, nullptr);
}

QFont TreeLogModel::logFont() const
{
  return _logFont;
}

void TreeLogModel::setLogFont(const QFont& font)
{
  _logFont = font;
  emit dataChanged(createIndex(0, 0, nullptr), createIndex(names.size(), 0, nullptr), QVector<int>() << Qt::FontRole);
}

void TreeLogModel::clear()
{
  for (const QString& name : names) {
    clear(name);
  }
}

void TreeLogModel::clear(const QString& container)
{
  int row = names.indexOf(container);
  if (row < 0) {
    return;
  }
  LogLine* line = roots[container];
  beginRemoveRows(index(row, 0, QModelIndex()), 0, line->children.size() - 1);
  qDeleteAll(line->children);
  line->children.clear();
  endRemoveRows();
}
