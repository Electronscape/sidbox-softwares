#include "mainwindow.h"

#include <QApplication>
#include <QMainWindow>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    setvbuf(stdout, NULL, _IONBF, 0);


    w.show();
    return a.exec();
}
