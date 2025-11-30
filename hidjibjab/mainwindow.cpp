#include "mainwindow.h"
#include "keyboardhidemu.h"

#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    KeyboardHidEmu *kbd = new KeyboardHidEmu(this);

    connect(kbd, &KeyboardHidEmu::hidReportReady, this, [this](const std::array<uint8_t,8> &report){
        QString s;
        for(auto b: report)
            s += QString("%1 ").arg(b,2,16,QChar('0'));
        ui->txtHIDBuffer->append(s);
    });

    if (!kbd->start()) {
        ui->txtHIDBuffer->append("No keyboard detected!");
    }
}



MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::handleHidReadyRead()
{
    QByteArray data = hidFile.readAll();
    QString hex = data.toHex(' ').toUpper();

    ui->txtHIDBuffer->append(hex);
}
