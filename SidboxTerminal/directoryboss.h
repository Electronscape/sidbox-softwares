#ifndef DIRECTORYBOSS_H
#define DIRECTORYBOSS_H

#include <QMainWindow>
#include <QModelIndex>
#include <QSettings>
#include <QList>
#include <QCheckBox>
#include "serialhandler.h"


namespace Ui {
class DirectoryBoss;
}

class DirectoryBoss : public QMainWindow
{
    Q_OBJECT

public:
    explicit DirectoryBoss(SerialHandler* sh, QWidget *parent = nullptr);

    ~DirectoryBoss();

private:
    Ui::DirectoryBoss *ui;
    SerialHandler* serial;    // store the shared serial instance

    QList<QCheckBox*> chkAudioConfigs;
    void procCheckAudioConf(const QList<QCheckBox*>& box);
    bool capturingDir = false;
    QString dirBuffer;
    QString leftoverLine; // class member

    QSettings* settings;
    bool eqReady = false;
    QTimer* eqUpdateTimer;  // Throttle timer for EQ sliders
    QTimer* eqStartupTimer = nullptr;


    void onDirListDoubleClicked(const QModelIndex &index);
    void closeEvent(QCloseEvent *event);
    void sendNewDirectory();

    void saveWindowState();
    void restoreWindowState();

    void setupEQTimer();

    void saveKFM();
    void loadKFM();

    void updateEQ();
    uint8_t getPickedGates();
    void sendNoteCMD(uint8_t type);

private slots:
    void onSerialInput(const QString &text);


signals:
    void reqeustOpenFTP();

};

#endif // DIRECTORYBOSS_H
