#ifndef D_DCLOGVIEW_H
#define D_DCLOGVIEW_H

#include <QTabWidget>
#include <QPlainTextEdit>
#include <QHash>

class DcLogView : public QTabWidget {
Q_OBJECT
public:
  DcLogView(QWidget* parent = nullptr);

  QString currentContainer() const;

signals:
  void currentContainerChanged(const QString& name);

public slots:
  void addContainer(const QString& container);
  void logMessage(const QDateTime& timestamp, const QString& container, const QString& message);
  void statusChanged(const QString& container, const QString& status);
  void clearCurrent();

private slots:
  void tabActivated(int index);

protected:
  void showEvent(QShowEvent* event);

private:
  QHash<QString, QPlainTextEdit*> logs;
  QStringList names;
};

#endif
