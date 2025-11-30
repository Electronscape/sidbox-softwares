#ifndef FRMFTP_H
#define FRMFTP_H

#include <QMainWindow>
#include <QSettings>
#include <QScreen>
#include <QSerialPort>
#include "serialhandler.h"

// --------- SIDBOX FTP PROTOCOL BYTES -----------------------------
#define FTP_H1          0xF1    // HEAD1
#define FTP_H2          0xF2    // HEAD2

// CMD
#define FTP_CMDS        0xC1    // START
#define FTP_CMDC        0xC2    // DATA CHUNK
#define FTP_CMDE        0xC3    // END
#define FTP_CMDZ        0xCF    // SYNC

#define FTP_CMDF        0xFF    // FOOT

// TYPE
#define FTP_TYPE_FILEOW 0xB0    // Binary File - Overwrite
#define FTP_TYPE_FILE   0xBF    // Binary File
#define FTP_TYPE_RAM    0xBD    // Binary RAM

// CRC
#define FTP_CRC_OK      0x77
#define FTP_CRC_ERR     0xAE

// Control Messages
#define FTP_ACK         0xAC
#define FTP_ERR         FTP_CRC_ERR
/////////////////////////////////////////////////////////////////////

// FTP states
enum FTPState {
    FTP_IDLE,
    FTP_WAIT_START_ACK,
    FTP_WAIT_CHUNK_ACK,
    FTP_WAIT_END_ACK
};

namespace Ui {
class FrmFTP;
}

class FrmFTP : public QMainWindow
{
    Q_OBJECT

public:
    explicit FrmFTP(SerialHandler* sh, QWidget *parent = nullptr);
    ~FrmFTP();

    // Starts an async FTP upload
    void startUpload();

private:
    Ui::FrmFTP *ui;
    SerialHandler* serialHandler;    // Shared serial instance

    // FTP engine
    QSerialPort* ftpSerial = nullptr;
    QByteArray ftpBuffer;
    QByteArray fileData;
    QString filename;
    quint32 fileSize = 0;
    quint8 fileCRC = 0;
    int chunkSize = 2048;
    int currentOffset = 0;
    int ramOffset = 0;
    FTPState ftpState = FTP_IDLE;

    // Settings
    QSettings* settings;
    void loadSettings();
    void saveSettings();

    // GUI
    void closeEvent(QCloseEvent* event) override;
    void saveWindowState();
    void restoreWindowState();

    // FTP helpers
    void submitFTPLog(const QString &msg);
    void sendNextChunk();

    // Packet builders
    QByteArray buildStartPacket(const QString &filename, quint32 fileSize, quint8 fileCrc, quint8 destType, bool overwrite);
    QByteArray buildChunkPacket(quint32 offset, quint16 size, const QByteArray &dataChunk);
    QByteArray buildEndPacket();

private slots:
    void processFtpBuffer();
    void updateProgress(int value);

};

#endif // FRMFTP_H
