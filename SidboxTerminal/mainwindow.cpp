#include "mainwindow.h"
#include "directoryboss.h"

#include "./ui_mainwindow.h"
#include <string.h>

#include "serialhandler.h"


#include <QMessageBox>
#include <QDir>
#include <QFileInfoList>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QMenu>
#include <QTextEdit>
#include <QKeyEvent>
#include <QWindow>
#include <QTimer>

void MainWindow::sendCurrentCommand()
{
    QString cmd = ui->txtCommandLine->text().trimmed();
    if (!cmd.isEmpty()) {
        serial->writeData(cmd);

        // Add to history if not duplicate
        if (commandHistory.isEmpty() || commandHistory.last() != cmd)
            commandHistory.append(cmd);

        historyIndex = commandHistory.size(); // reset index

        ui->txtCommandLine->clear();
        ui->txtCommandLine->setFocus();
    }
}

void MainWindow::sendCustomCommand(const QString &string){
    serial->writeData(string);
}

void MainWindow::on_actionAbout_triggered(){
    QMessageBox::information(this, "About Sidbox Terminal", "Sidbox Terminal V5.0\r\nDesigned for the sidbox V5.4\r\n\r\nDesigned by Wayne Holliday\r\nREALLY has been a long road!");
}

void MainWindow::on_mnuItemSDDirectory_triggered(){
    MainWindow::OpenDirectoryBoss();
}


void MainWindow::on_mnuItemFTP_triggered(){
    MainWindow::OpenFTP();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);



    // Get current folder
    QDir dir(QDir::currentPath());

    // List all files (not directories)
    QFileInfoList files = dir.entryInfoList(QDir::Files);

    // Add files to QListWidget
    for (const QFileInfo &fileInfo : files) {
        //ui->listWidget->addItem(fileInfo.fileName());

    }

    serial = new SerialHandler(ui->textBox, this);

    QString iniPath = QCoreApplication::applicationDirPath() + "/settings.ini";
    settings = new QSettings(iniPath, QSettings::IniFormat, this);




    //serial->listPorts();   // show in Application Output




    connect(ui->cmdOpenPort, &QPushButton::clicked, this, [=]() {
        MainWindow::OpenSerialPort();
    });


    connect(ui->txtCommandLine, &QLineEdit::returnPressed, this, [=]() { sendCurrentCommand(); });
    connect(ui->cmdSend, &QPushButton::clicked, this, [=]() { sendCurrentCommand(); });

    connect(ui->cmdClear, &QPushButton::clicked, this, [=]() {
        ui->txtCommandLine->clear();
    });

    connect(ui->cmdSendCustomCMDDir, &QPushButton::clicked, this, [=](){
        sendCustomCommand("dir\r\n");
    });

    connect(ui->cmdClrRam, &QPushButton::clicked, this, [this](){
        sendCustomCommand("clear\n");
    });

    connect(ui->cmdClearLog, &QPushButton::clicked, this, [=]() {
        ui->textBox->clear();
    });

    connect(ui->cmdSetTimeDate, &QPushButton::clicked, this, [=](){
        QDateTime dt = QDateTime::currentDateTime();

        /*
        -> setdate <day> <weekday> <month> <year (last two numbers only)
        -> settime <hour> <min> [second (default 0)]
        */

        char sendtimestr[128];
        sprintf(sendtimestr, "settime %u %u\r\n",
                dt.time().hour(),
                dt.time().minute());
        sendCustomCommand(sendtimestr);

        QTimer::singleShot(220, this, [this, dt]() {
            char senddatestr[128];
            sprintf(senddatestr, "setdate %u %u %u %u\r\n",
                    dt.date().day(),
                    dt.date().dayOfWeek(),
                    dt.date().month(),
                    dt.date().year() % 100);
            sendCustomCommand(senddatestr);
        });
    });

    connect(ui->cmdViewRam, &QPushButton::clicked, this, [=](){
        char senddatestr[128];
        sprintf(senddatestr, "view %s\r\n", ui->txtViewFrom->text().toUtf8().constData());
        sendCustomCommand(senddatestr);
    });


    // open the DirectoryBoos Window
    //connect(ui->mnuItemSDDirectory, &QMenu::clicked, this, [=](){  });
    connect(ui->cmdOpenDirectory,   &QPushButton::clicked, this, [=](){ MainWindow::OpenDirectoryBoss(); });

    /*
    connect(ui->cmdTest, &QPushButton::clicked, this, [=](){
        Dialog *TestDialog;
        TestDialog = new Dialog(nullptr);
        TestDialog->show();
    });
    */


    // open the FTP Window
    //connect(ui->mnuItemFTP, &QMenu::clicked, this, [=](){ MainWindow::OpenFTP(); });
    connect(ui->cmdOpenFTP, &QPushButton::clicked, this, [=](){ MainWindow::OpenFTP(); });

    //connect()
    //connect(dirDialog, &SerialHandler::requestOpenSerialPort, this, &MainWindow::OpenFTP);
    connect(serial, &SerialHandler::requestOpenSerialPort, this, &MainWindow::OpenSerialPort);

    ui->txtCommandLine->installEventFilter(this);

    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(1000);

    connect(m_saveTimer, &QTimer::timeout, this, [this](){
        settings->setValue("mainwin/ramoffset", ui->txtViewFrom->text().toUtf8());
    });

    connect(ui->txtViewFrom, &QLineEdit::textChanged, this, [this](){
        m_saveTimer->start();  // ← Magic: restarts every change
    });

}

void MainWindow::showEvent(QShowEvent *event){
    //QMessageBox::information(this, "hello world", "hello");

    QString defaultViewFromRAM;
    restoreWindowState();

    defaultViewFromRAM = settings->value("mainwin/ramoffset").toString();

    if(defaultViewFromRAM.length() == 0)
        ui->txtViewFrom->setText("0x0000");
    else
        ui->txtViewFrom->setText(defaultViewFromRAM);

}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->txtCommandLine && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        // Handle UP key
        if (keyEvent->key() == Qt::Key_Up) {
            if (!commandHistory.isEmpty()) {
                if (historyIndex < 0)
                    historyIndex = commandHistory.size() - 1;
                else if (historyIndex > 0)
                    historyIndex--;

                ui->txtCommandLine->setText(commandHistory[historyIndex]);
            }
            return true; // stop event propagation
        }

        // Handle DOWN key
        if (keyEvent->key() == Qt::Key_Down) {
            if (!commandHistory.isEmpty()) {
                if (historyIndex < commandHistory.size() - 1)
                    historyIndex++;
                else
                    historyIndex = commandHistory.size(); // go past end

                if (historyIndex >= commandHistory.size())
                    ui->txtCommandLine->clear();
                else
                    ui->txtCommandLine->setText(commandHistory[historyIndex]);
            }
            return true;
        }
    }

    // Pass through everything else
    return QMainWindow::eventFilter(obj, event);
}


bool MainWindow::OpenSerialPort(){
    char portname[128];
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        if (info.vendorIdentifier() == 0x10c4 && info.productIdentifier() == 0xea60) {
            QString qstr = info.portName();    // QString
            QByteArray ba = qstr.toUtf8();    // Convert to UTF-8
            strcpy(portname, ba.constData()); // Copy into char array
        }
    }

    bool success = serial->openPort(portname, 256000);

    if (success) {
        statusBar()->showMessage("Connected to port " + QString(portname), 0);
    } else {
        statusBar()->showMessage("Failed to open port", 0);
        QMessageBox::critical(this, "Error", "Failed to open port!");
    }

    return success; // <-- return the result

}

void MainWindow::OpenDirectoryBoss(){
    if (!dirDialog) {  // only create if it doesn’t exist
        dirDialog = new DirectoryBoss(serial, nullptr);
        dirDialog->setAttribute(Qt::WA_DeleteOnClose);  // auto-delete on close

        // When dialog is destroyed, reset pointer
        connect(dirDialog, &QDialog::destroyed, this, [this]() {
            dirDialog = nullptr;
        });
        // connect the possible FTP click, so do that just now
        connect(dirDialog, &DirectoryBoss::reqeustOpenFTP, this, &MainWindow::OpenFTP);

        dirDialog->show();
    } else {
        dirDialog->raise();    // bring to front
        dirDialog->activateWindow();  // give focus
    }
}

void MainWindow::OpenFTP(){
    if(!ftpDialog){
        ftpDialog = new FrmFTP(serial);
        ftpDialog->setAttribute(Qt::WA_DeleteOnClose);

        connect(ftpDialog, &QDialog::destroyed, this, [this]() {
            ftpDialog = nullptr;
        });

        ftpDialog->show();
    } else {
        // already open, bring it to front
        ftpDialog->raise();
        ftpDialog->activateWindow();
    }
}

MainWindow::~MainWindow()
{

    delete ui;
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    if (dirDialog) {
        dirDialog->close();   // ✅ closes it cleanly before MainWindow dies
        dirDialog = nullptr;
    }
    if (ftpDialog) {
        ftpDialog->close();
        ftpDialog = nullptr;
    }

    saveWindowState();

    QMainWindow::closeEvent(event);
}


void MainWindow::saveWindowState()
{
    settings->setValue("mainwin/x", pos().x());
    settings->setValue("mainwin/y", pos().y());
    settings->setValue("mainwin/w", width());
    settings->setValue("mainwin/h", height());
}

void MainWindow::restoreWindowState(){

    const int x = settings->value("mainwin/x", -1).toInt();
    const int y = settings->value("mainwin/y", -1).toInt();
    const int w = settings->value("mainwin/w", -1).toInt();
    const int h = settings->value("mainwin/h", -1).toInt();

    if (x < 0 || y < 0 || w <= 0 || h <= 0) {
        QScreen *scr = QGuiApplication::primaryScreen();
        const QRect avail = scr->availableGeometry();   // respects panels
        const QSize  sz    = sizeHint();                // or a default size
        const QPoint centre(avail.center() - QPoint(sz.width()/2, sz.height()/2));
        resize(sz);
        move(centre);
        return;
    }


    resize(w, h);
    move(x, y);
    //this->move(x,y);

    printf("Restoring Settings here\rn");
}



