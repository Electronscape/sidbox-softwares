#include "dialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog w;

    setvbuf(stdout, NULL, _IONBF, 0);

    w.show();
    return a.exec();
}
