#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QString>
#include <QList>
#include <QSettings>

#include "directoryboss.h"
#include "frmftp.h"
#include "serialhandler.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    void closeEvent(QCloseEvent *event);
    SerialHandler* serial;
    void sendCurrentCommand();
    void sendCustomCommand(const QString &string);
    DirectoryBoss *dirDialog = nullptr;
    FrmFTP *ftpDialog = nullptr;

    QSettings *settings;
    QTimer *m_saveTimer;

    QStringList commandHistory;
    int historyIndex = -1;
    bool eventFilter(QObject *obj, QEvent *event);
    void showEvent(QShowEvent *event);

    void restoreWindowState();
    void saveWindowState();


private slots:
    // menu items!
    void on_actionAbout_triggered();
    void on_mnuItemSDDirectory_triggered();
    void on_mnuItemFTP_triggered();

    void OpenFTP();
    void OpenDirectoryBoss();
    bool OpenSerialPort();


};
#endif // MAINWINDOW_H
