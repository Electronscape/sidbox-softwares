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

    QString platform = QGuiApplication::platformName();
    printf("Current platform:");;
    printf(platform.toUtf8());
    printf("\n");


    if (platform != QLatin1String("xcb")) {
        printf("Relaunching with XWayland...\n");

        QStringList args = QCoreApplication::arguments();
        args.removeFirst();  // remove program name

        // Build environment
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("QT_QPA_PLATFORM", "xcb");

        // Create process
        QProcess *p = new QProcess;
        p->setProcessEnvironment(env);
        p->setProgram(QCoreApplication::applicationFilePath());
        p->setArguments(args);
        p->setWorkingDirectory(QCoreApplication::applicationDirPath());

        // Launch and exit
        bool launched = p->startDetached();
        if (launched) {
            printf("Relaunched successfully!");
            return 0;
        } else {
            printf("!Relaunch failed!");
            delete p;
        }
    } else
        printf("already bubbling with fanny\n");


    MainWindow w;

    setvbuf(stdout, NULL, _IONBF, 0);

    QIcon icon(":/icons/sidterm.ico");
    w.setWindowIcon(icon);

    printf("Start Sidbox\r\n"); //fflush(stdout);

    w.show();
    return a.exec();
}
