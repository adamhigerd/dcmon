#ifndef D_DCMONWINDOW_H
#define D_DCMONWINDOW_H

#include <QMainWindow>
class DcToolBar;
class DcLogView;
class DcPs;
class DcLog;
class QLabel;

class DcmonWindow : public QMainWindow {
Q_OBJECT
public:
  DcmonWindow(QWidget* parent = nullptr);

private slots:
  void reloadConfig();
  void filesUpdated();

private:
  DcToolBar* tb;
  DcLogView* view;
  DcPs* ps;
  DcLog* logger;
  QLabel* notify;
};
#endif
