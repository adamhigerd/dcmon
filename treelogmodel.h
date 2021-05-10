#ifndef D_TREELOGMODEL_H
#define D_TREELOGMODEL_H

#include <QAbstractItemModel>
#include <QDateTime>
#include <QHash>
#include <QFont>
#include <vector>

class TreeLogModel : public QAbstractItemModel
{
Q_OBJECT
public:
  TreeLogModel(QObject* parent = nullptr);
  ~TreeLogModel();

  int maxLines() const;
  void setMaxLines(int lines);
  QModelIndex rootForContainer(const QString& name) const;

  QFont logFont() const;
  void setLogFont(const QFont& font);

  //  index(), parent(), rowCount(), columnCount(), and data().
  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex& parent) const;
  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  int columnCount(const QModelIndex& parent = QModelIndex()) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  QVariant data(const QModelIndex& index, int role) const;

public slots:
  void logMessage(const QDateTime& timestamp, const QString& container, const QString& message);

private:
  void flushOldest(const QString& container);

  struct LogLine {
    LogLine();
    LogLine(LogLine* parent, const QDateTime& datetime, const QString& msg);
    LogLine(LogLine* parent, const QString& msg, int indent);
    ~LogLine();
    QDateTime datetime;
    LogLine* parent;
    QString line;
    int indent;
    std::vector<LogLine*> children;
  };
  int _maxLines;
  QStringList names;
  QHash<QString, LogLine*> roots;
  QFont _logFont;

  int find(const LogLine* parent, const LogLine* child) const;
  QModelIndex index(LogLine* line, int column) const;
  LogLine* lineForIndex(const QModelIndex& parent) const;
};

#endif
