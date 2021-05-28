#ifndef D_DCTOOLBAR_H
#define D_DCTOOLBAR_H

#include <QToolBar>
#include <QLabel>
class QProcess;

class DcToolBar : public QToolBar {
Q_OBJECT
friend class DcmonWindow;
public:
  DcToolBar(const QString& dcFile, QWidget* parent = nullptr);

public slots:
  void setCurrentContainer(const QString& name);
  void statusChanged(const QString& name, const QString& status);
  void startAll();
  void restartAll();
  QProcess* stopAll(bool isRestart = false);
  void startOne(const QString& name = QString());
  void restartOne(const QString& name = QString());
  void stopOne(const QString& name = QString());

signals:
  void pollStatus();
  void clearOne();
  void logMessage(const QDateTime& timestamp, const QString& container, const QString& message);

private slots:
  void onPollFinished();

private:
  void pollOn(QProcess* p);
  void runThenPoll(const QStringList& args);

  QString dcFile, container;
  QLabel* label;
  QAction* aStartAll;
  QAction* aRestartAll;
  QAction* aStopAll;
  QAction* aStartOne;
  QAction* aRestartOne;
  QAction* aStopOne;
  QProcess* pollProcess;
  QHash<QString, QString> statuses;
};

#endif
