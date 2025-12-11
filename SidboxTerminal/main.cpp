#include "mainwindow.h"

#include <QApplication>
#include <QMainWindow>
#include <QIcon>
#include <QDebug>
#include <QFile>

#include <QProcess>
#include <QProcessEnvironment>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


#ifdef Q_OS_LINUX
    QString platform = QGuiApplication::platformName();
    printf("Current platform: %s\n", platform.toUtf8().constData());

    // If not already running under X11/XWayland
    if (!platform.contains("xcb", Qt::CaseInsensitive)) {
        printf("Relaunching with XWayland...\n");

        QStringList args = QCoreApplication::arguments();
        args.removeFirst();  // remove program name

        // Build environment without Wayland variables
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.remove("WAYLAND_DISPLAY");
        env.remove("WAYLAND_SOCKET");
        env.remove("XDG_SESSION_TYPE");
        env.insert("XDG_SESSION_TYPE", "x11");
        env.insert("QT_QPA_PLATFORM", "xcb");

        // Launch the process
        QProcess *p = new QProcess;
        p->setProcessEnvironment(env);
        p->setProgram(QCoreApplication::applicationFilePath());
        p->setArguments(args);
        p->setWorkingDirectory(QCoreApplication::applicationDirPath());

        if (p->startDetached()) {
            printf("Relaunched successfully!\n");
            return 0;
        } else {
            printf("!Relaunch failed!\n");
            delete p;
        }
    } else {
        printf("already bubbling !!!\n");
    }
#endif

    MainWindow w;

    setvbuf(stdout, NULL, _IONBF, 0);

    QIcon icon(":/icons/sidterm.ico");
    w.setWindowIcon(icon);

    printf("Start Sidbox\r\n"); //fflush(stdout);

    w.show();
    return a.exec();
}
