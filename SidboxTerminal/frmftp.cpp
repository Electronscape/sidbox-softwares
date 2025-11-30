#include "frmftp.h"
#include "ui_frmftp.h"

#include <QCoreApplication>
#include <QSettings>
#include <QRadioButton>
#include <QLineEdit>
#include <QSerialPort>
#include <QFileDialog>
#include <QDir>
#include <QThread>
#include <QElapsedTimer>


#include <QProgressBar>

// Simple min/max for SIDBOX FTP
inline int sidboxMin(int a, int b) { return (a < b) ? a : b; }
inline int sidboxMax(int a, int b) { return (a > b) ? a : b; }

QElapsedTimer ftpTimer;       // Tracks total elapsed time
quint32 bytesSent = 0;        // Bytes sent so far
bool abortTransfer = 0;

static quint8 crc8(const QByteArray &data)
{
    quint8 crc = 0;
    for (auto ch : data) {
        crc ^= static_cast<quint8>(ch);
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x07;
            else crc <<= 1;
        }
    }
    return crc;
}

//////////////////// PACKET BUILDERS ////////////////////

QByteArray FrmFTP::buildStartPacket(const QString &filename, quint32 fileSize, quint8 fileCrc, quint8 destType, bool overwrite)
{
    QByteArray pkt;
    //quint8 type = overwrite ? FTP_TYPE_FILEOW : destType;
    quint8 type = destType;// ? FTP_TYPE_FILEOW : destType;

    pkt.append(static_cast<char>(FTP_H1));
    pkt.append(static_cast<char>(FTP_H2));
    pkt.append(static_cast<char>(FTP_CMDS));
    pkt.append(static_cast<char>(type));

    QByteArray fnameBytes = filename.toUtf8();
    int fnameLen = qMin(fnameBytes.size(), 64);
    pkt.append(static_cast<char>(fnameLen));

    QByteArray fnameField(64, '\0');
    if (fnameLen > 0) memcpy(fnameField.data(), fnameBytes.constData(), fnameLen);
    pkt.append(fnameField);

    pkt.append(static_cast<char>(fileSize & 0xFF));
    pkt.append(static_cast<char>((fileSize >> 8) & 0xFF));
    pkt.append(static_cast<char>((fileSize >> 16) & 0xFF));
    pkt.append(static_cast<char>((fileSize >> 24) & 0xFF));

    pkt.append(static_cast<char>(fileCrc));
    pkt.append(static_cast<char>(FTP_CMDF));

    pkt.append(static_cast<char>(crc8(pkt)));
    return pkt;
}

QByteArray FrmFTP::buildChunkPacket(quint32 offset, quint16 size, const QByteArray &dataChunk)
{
    QByteArray pkt;

    // HEADER
    pkt.append(static_cast<char>(FTP_H1));
    pkt.append(static_cast<char>(FTP_H2));

    // CMD
    pkt.append(static_cast<char>(FTP_CMDC));

    // OFFSET (u32 little-endian)
    pkt.append(static_cast<char>(offset & 0xFF));
    pkt.append(static_cast<char>((offset >> 8) & 0xFF));
    pkt.append(static_cast<char>((offset >> 16) & 0xFF));
    pkt.append(static_cast<char>((offset >> 24) & 0xFF));

    // SIZE (u16 little-endian)
    pkt.append(static_cast<char>(size & 0xFF));
    pkt.append(static_cast<char>((size >> 8) & 0xFF));

    // FOOT
    pkt.append(static_cast<char>(FTP_CMDF));

    // HEAD CRC (CRC8 of header..FOOT)
    quint8 headCrc = crc8(pkt);
    pkt.append(static_cast<char>(headCrc));

    // DATA
    pkt.append(dataChunk);

    // DATA CRC
    quint8 dataCrc = crc8(dataChunk);
    pkt.append(static_cast<char>(dataCrc));

    // TERMINATOR
    pkt.append(static_cast<char>(0xFF));

    return pkt;
}



//////////////////// CONSTRUCTOR ////////////////////

FrmFTP::FrmFTP(SerialHandler* sh, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::FrmFTP), serialHandler(sh)
{
    ui->setupUi(this);

    QString iniPath = QCoreApplication::applicationDirPath() + "/settings.ini";
    settings = new QSettings(iniPath, QSettings::IniFormat, this);

    restoreWindowState();

    // Buttons
    connect(ui->cmdCloseFTP, &QPushButton::clicked, this, [=](){ this->close(); });
    connect(ui->cmdClearLog, &QPushButton::clicked, this, [=](){ ui->txtFTPLog->clear(); });

    // Radio buttons for chunk size
    connect(ui->opt2Chunk1, &QRadioButton::clicked, this, &FrmFTP::saveSettings);
    connect(ui->opt2Chunk2, &QRadioButton::clicked, this, &FrmFTP::saveSettings);
    connect(ui->opt2Chunk3, &QRadioButton::clicked, this, &FrmFTP::saveSettings);
    connect(ui->opt2Chunk4, &QRadioButton::clicked, this, &FrmFTP::saveSettings);
    connect(ui->opt2Chunk5, &QRadioButton::clicked, this, &FrmFTP::saveSettings);

    // LineEdit for memory offset
    connect(ui->txtStartAddress, &QLineEdit::textChanged, this, [=](){
        bool ok = false;
        uint value = ui->txtStartAddress->text().trimmed().toUInt(&ok, 16);
        if (ok && value <= 0xFFFFFF) settings->setValue("serial/memoffset", static_cast<int>(value));
    });

    // File browser
    connect(ui->cmdBrowseFile, &QPushButton::clicked, this, [=](){
        QString lastDir = settings->value("lastDir", QDir::homePath()).toString();
        QString filename = QFileDialog::getOpenFileName(this, "Select a File", lastDir, "All Files (*.*)");
        if (filename.isEmpty()) return;

        QFileInfo fi(filename);
        settings->setValue("lastDir", fi.absolutePath());
        settings->setValue("serial/fileurl", filename);
        ui->txtFileselected->setText(filename);
        printf("Selected file: '%s'\n", filename.toUtf8().constData());
    });

    // Send file button
    connect(ui->cmdSendFile, &QPushButton::clicked, this, &FrmFTP::startUpload);

    connect(ui->cmdAbort, &QPushButton::clicked, this, [=](){
        abortTransfer = true;
    });

    loadSettings();

    ui->pbFileupload->setValue(0);
    ui->pbFileupload->valueChanged(0);
    ui->pbFileupload->repaint();  // force immediate paint
    QCoreApplication::processEvents();

    connect(ui->cmdRunInRam, &QPushButton::clicked, this, [=](){
        bool ok = false;
        quint32 offset = ui->txtStartAddress->text().toUInt(&ok, 16);

        sh->writeData(QString("usr %1\n").arg(offset));
    });

    connect(ui->cmdViewRam, &QPushButton::clicked, this, [=](){
        char senddatestr[128];
        sprintf(senddatestr, "view 0x%s\n", ui->txtStartAddress->text().toUtf8().constData());
        //sendCustomCommand(senddatestr);
        sh->writeData(senddatestr);
    });

    connect(ui->cmdClearRam, &QPushButton::clicked, this, [=](){
        sh->writeData("clear");
    });

}

//////////////////// SETTINGS ////////////////////

void FrmFTP::saveSettings()
{
    if (ui->opt2Chunk1->isChecked()) settings->setValue("serial/bps", 1);
    if (ui->opt2Chunk2->isChecked()) settings->setValue("serial/bps", 2);
    if (ui->opt2Chunk3->isChecked()) settings->setValue("serial/bps", 3);
    if (ui->opt2Chunk4->isChecked()) settings->setValue("serial/bps", 4);
    if (ui->opt2Chunk5->isChecked()) settings->setValue("serial/bps", 5);
}



void FrmFTP::loadSettings()
{
    int bps = settings->value("serial/bps", 1).toInt();
    switch (bps) {
    case 1: ui->opt2Chunk1->setChecked(true); break;
    case 2: ui->opt2Chunk2->setChecked(true); break;
    case 3: ui->opt2Chunk3->setChecked(true); break;
    case 4: ui->opt2Chunk4->setChecked(true); break;
    case 5: ui->opt2Chunk5->setChecked(true); break;
    }

    int stored = settings->value("serial/memoffset", 0).toInt();
    ui->txtStartAddress->setText(QString::number(stored, 16).toUpper());

    ui->txtFileselected->setText(settings->value("serial/fileurl").toString());
}

//////////////////// FTP LOGGING ////////////////////

void FrmFTP::submitFTPLog(const QString &msg)
{
    ui->txtFTPLog->append(msg);// + "\n");
}

//////////////////// FTP ASYNC UPLOAD ////////////////////

void FrmFTP::startUpload()
{
    QString path = ui->txtFileselected->text();
    QFile file(path);

    if(ui->chkClearLogAuto->isChecked()){
        ui->txtFTPLog->clear();
    }

    if (!file.open(QIODevice::ReadOnly)) { submitFTPLog("Failed to open file"); return; }

    fileData = file.readAll();
    file.close();

    filename = QFileInfo(path).fileName();
    fileSize = fileData.size();
    fileCRC = crc8(fileData);


    if (ui->opt2Chunk1->isChecked()) chunkSize = 256;
    if (ui->opt2Chunk2->isChecked()) chunkSize = 512;
    if (ui->opt2Chunk3->isChecked()) chunkSize = 1024;
    if (ui->opt2Chunk4->isChecked()) chunkSize = 2048;
    if (ui->opt2Chunk5->isChecked()) chunkSize = 4000;

    quint8 ftp_upload_mode = FTP_TYPE_RAM;
    currentOffset = 0;
    ramOffset=0;

    bool ok = false;
    quint32 offset = ui->txtStartAddress->text().toUInt(&ok, 16);

    if (!ok) {
        // Invalid hex input
        submitFTPLog("Invalid hex offset!");

    } else {
        ramOffset = offset;
    }

    if (ui->opt1Ram->isChecked()) {
        ftp_upload_mode = FTP_TYPE_RAM;
        submitFTPLog(QString("upload to ram: 0x%1")
                .arg(QString::number(ramOffset, 16).toUpper()));
    }

    if (ui->opt1File->isChecked()){
        ftp_upload_mode = FTP_TYPE_FILE;
        submitFTPLog("file upload");
    }

    if (ui->opt1FileOverwrite->isChecked()){
        ftp_upload_mode = FTP_TYPE_FILEOW;
        submitFTPLog("file upload overwrite");
    }

    ftpState = FTP_WAIT_START_ACK;

    bytesSent = 0;
    ftpTimer.start();   // start tracking time

    ui->pbFileupload->setValue(0);

    ftpSerial = serialHandler->getSerial();
    disconnect(ftpSerial, &QSerialPort::readyRead, serialHandler, &SerialHandler::readData);
    connect(ftpSerial, &QSerialPort::readyRead, this, &FrmFTP::processFtpBuffer);

    if (ftpSerial->isOpen()) {
        submitFTPLog("STARTING FTP...");
        submitFTPLog(
            QString("Begin FTP: '%1' size:%2 kb")
                .arg(filename)
                .arg(fileSize)
            );
        ftpSerial->write(buildStartPacket(filename, fileSize, fileCRC, ftp_upload_mode, true));
    } else {
        submitFTPLog("Serial port not open! Can't send data.");
    }


}

void FrmFTP::processFtpBuffer()
{
    ftpBuffer.append(ftpSerial->readAll());
    bool ok = false;
    quint32 offset = ui->txtStartAddress->text().toUInt(&ok, 16);

    while (true) {
        int ackIndex = ftpBuffer.indexOf(FTP_ACK);
        int progress = 0;
        int lastChunkSize;
        if (ackIndex == -1) break;

        ftpBuffer.remove(0, ackIndex + 1);

        switch (ftpState) {
        case FTP_WAIT_START_ACK:
            ftpState = FTP_WAIT_CHUNK_ACK;
            //submitFTPLog("START ACK received, sending first chunk...");
            sendNextChunk();
            break;

        case FTP_WAIT_CHUNK_ACK: {
            //currentOffset += sidboxMin(chunkSize, fileSize - currentOffset);
            lastChunkSize = sidboxMin(chunkSize, fileSize - currentOffset);
            currentOffset += lastChunkSize;
            ramOffset += lastChunkSize;
            bytesSent += lastChunkSize;


            // Calculate speed (bytes per second)
            double elapsedSec = ftpTimer.elapsed() / 1000.0;
            double speed = bytesSent / elapsedSec;  // B/s

            // Estimate remaining time
            double remainingBytes = fileSize - bytesSent;
            double etaSec = remainingBytes / speed;

            //ui->lblSpeed->setText(QString("Speed: %1 KB/s").arg(speed / 1024, 0, 'f', 1));
            //ui->lblETA->setText(QString("ETA: %1 s").arg(static_cast<int>(etaSec)));

            ui->lblProgress->setText(
                QString("Speed: %1 KB/s (%2s)")
                .arg(speed / 1024, 0, 'f', 1)
                .arg(static_cast<int>(etaSec))
            );


            progress = (int)(((double)currentOffset / (double)fileSize) * 100.0);

            ui->pbFileupload->setValue(progress);
            ui->pbFileupload->repaint();  // forces immediate repaint

            QCoreApplication::processEvents();   // <-- THIS MAKES IT REFRESH

            if (currentOffset < fileSize) {
                //submitFTPLog(QString("Chunk ACK received, sending next chunk at offset %1...").arg(currentOffset));
                sendNextChunk();
            } else {
                ftpState = FTP_WAIT_END_ACK;
                //submitFTPLog(QString("All chunks sent, sending END packet... last chunk size(%1) byte(s)").arg(lastChunkSize));
                ftpSerial->write(buildEndPacket());  // <-- this one works,
            }
        }   break;


        case FTP_WAIT_END_ACK: {
            ftpState = FTP_IDLE;



            //submitFTPLog("END ACK received, FTP upload complete!");
            submitFTPLog("FTP upload complete!");
            if(ui->chkAutoRun->isChecked()){

                ftpSerial->write(QString("usr %1\n").arg(offset).toUtf8()); // <-- this doesnt, but its the same function!?
            }

            disconnect(ftpSerial, &QSerialPort::readyRead, this, &FrmFTP::processFtpBuffer);
            connect(ftpSerial, &QSerialPort::readyRead, serialHandler, &SerialHandler::readData);
            ftpBuffer.clear();
        }   break;

        default: break;
        }

        if (abortTransfer){
            abortTransfer=0;
            submitFTPLog("FTP Aborted!");
            ftpState = FTP_IDLE;

            disconnect(ftpSerial, &QSerialPort::readyRead, this, &FrmFTP::processFtpBuffer);
            connect(ftpSerial, &QSerialPort::readyRead, serialHandler, &SerialHandler::readData);
            ftpBuffer.clear();
            break;
        }

    }
}

void FrmFTP::sendNextChunk()
{
    int size = sidboxMin(chunkSize, fileSize - currentOffset);
    QByteArray chunkData = fileData.mid(currentOffset, size);
    ftpSerial->write(buildChunkPacket(ramOffset, size, chunkData));
}


QByteArray FrmFTP::buildEndPacket()
{
    QByteArray pkt;

    // HEADER
    pkt.append(static_cast<char>(FTP_H1));
    pkt.append(static_cast<char>(FTP_H2));

    // CMD = END
    pkt.append(static_cast<char>(FTP_CMDE));

    // CRC = CRC8 of first 3 bytes only (H1,H2,CMDE)
    quint8 crc = crc8(pkt); // pkt.size() == 3 here
    pkt.append(static_cast<char>(crc));

    return pkt;
}


//////////////////// CLEANUP ////////////////////

void FrmFTP::closeEvent(QCloseEvent *event)
{
    if (ftpSerial && ftpState != FTP_IDLE) {
        disconnect(ftpSerial, &QSerialPort::readyRead, this, &FrmFTP::processFtpBuffer);
        connect(ftpSerial, &QSerialPort::readyRead, serialHandler, &SerialHandler::readData);
    }
    saveWindowState();
    QMainWindow::closeEvent(event);
}

void FrmFTP::saveWindowState()
{
    settings->setValue("ftpwin/x", this->pos().x());
    settings->setValue("ftpwin/y", this->pos().y());
}

void FrmFTP::restoreWindowState(){

    int x = settings->value("ftpwin/x").toInt();
    int y = settings->value("ftpwin/y").toInt();

    if(x==0 && y==0){
        QScreen *primary = QGuiApplication::primaryScreen();
        QRect screenGeom = primary->availableGeometry();  // avoids taskbar
        QPoint center = screenGeom.center() - QPoint(this->width()/2, this->height()/2);
        this->move(center);
    } else

    this->move(x,y);

    printf("Restoring Settings here\rn");
}












void FrmFTP::updateProgress(int value)
{
    ui->pbFileupload->setValue(value);
}


FrmFTP::~FrmFTP()
{
    delete ui;
}
