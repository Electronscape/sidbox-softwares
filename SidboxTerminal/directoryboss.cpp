#include "directoryboss.h"
#include "kfmparser.h"
#include "ui_directoryboss.h"

#include <QFileDialog>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QRegularExpression>
#include <QListView>
#include <QModelIndex>
#include <QMessageBox>
#include <QMap>
#include <QScrollBar>
#include <QDesktopServices>
#include <QUrl>
#include <QTextEdit>

#include <QList>


// this is where needs to be changed per OS
//#define console_width_space 59 // or 62 in windows
#define console_width_space 67 // or 62 in windows



//sid,wav,mod,s3m,ym,prg,tpz,tap,pls,tzx,scr,sap,tmc,cmc,dmc,fc,dlt,mpt,cmr,rmt,tm2,cm3,ay,rgb,stp,tfx,tpa,vgm,vgp,iff,gg,sms,med,app

DirectoryBoss::DirectoryBoss(SerialHandler *sh, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DirectoryBoss)
    , serial(sh)
{
    ui->setupUi(this);

    //QIcon icon(":/icons/sidbox-terminal.ico");
    //setWindowIcon(icon);
    QString iniPath = QCoreApplication::applicationDirPath() + "/settings.ini";
    settings = new QSettings(iniPath, QSettings::IniFormat, this);

    restoreWindowState();

    chkAudioConfigs = {
        ui->chkAudConf0, ui->chkAudConf1, ui->chkAudConf2, ui->chkAudConf3, ui->chkAudConf4,
        ui->chkAudConf5, ui->chkAudConf6, ui->chkAudConf7, ui->chkAudConf8, ui->chkAudConf9,
        ui->chkAudConf10, ui->chkAudConf11, ui->chkAudConf12, ui->chkAudConf13, ui->chkAudConf14,
        ui->chkAudConf15, ui->chkAudConf16, ui->chkAudConf17, ui->chkAudConf18, ui->chkAudConf19,
        ui->chkAudConf20, ui->chkAudConf21
    };

    int loadBits = 0;
    loadBits = settings->value("audioconfig/channelconf").toUInt();
    for (int i = 0; i < chkAudioConfigs.size(); ++i) {
        bool bitSet = (loadBits >> i) & 1;
        chkAudioConfigs[i]->setChecked(bitSet);
    }


    //this->setGeometry(0,0,100,100);
    this->show();



    printf("DirectoryBoss Opened()\r\n");

    // this works
    connect(ui->scrSubSong, &QScrollBar::valueChanged, this, [=](int value){
        ui->lblSubSong->setText(QString::number(value));
    });

    // this works
    connect(ui->scrSubSong, &QSlider::sliderReleased, this, [=]() {
        int value = ui->scrSubSong->value();
        ui->lblSubSong->setText(QString::number(value));

        // Send to serial port only when released
        if (serial) {
            serial->writeData(QStringLiteral("subtrack %1\r\n").arg(value).toUtf8());
        }
    });

    // this returns the old number (when clicking the left and right arrows)
    connect(ui->scrSubSong, &QSlider::actionTriggered, this, [=](int action){
        if (action == QSlider::SliderSingleStepAdd || action == QSlider::SliderSingleStepSub) {
            QTimer::singleShot(0, this, [=]() {
                int value = ui->scrSubSong->value();
                ui->lblSubSong->setText(QString::number(value));

                if (serial)
                    serial->writeData(QStringLiteral("subtrack %1\r\n").arg(value).toUtf8());
            });
        }
    });

    connect(ui->cmdParentDir, &QPushButton::clicked, this, [=](){
        QDateTime dt = QDateTime::currentDateTime();
        if(serial){
            serial->writeData("cd ..\r\n");
            QTimer::singleShot(220, this, [this, dt]() {
                char senddatestr[128];
                serial->writeData("dir\r\n");
            });

        }else
            printf("SerialPort::Null :(\r\n");

    });

    connect(serial, &SerialHandler::rawSerial, this, &DirectoryBoss::onSerialInput);

    connect(ui->lstDirList, &QListView::doubleClicked, this, &DirectoryBoss::onDirListDoubleClicked);

    connect(ui->cmdStopAud, &QPushButton::clicked, this, [=](){
        serial->writeData("audstop\r\n");
    });

    connect(ui->cmdGetDir, &QPushButton::clicked, this, [=](){
        serial->writeData("dir\r\n");
    });


    connect(ui->txtFilePath, &QLineEdit::returnPressed, this, [=]() { sendNewDirectory(); });
    connect(ui->cmdChangeDir, &QPushButton::clicked, this, [=]() { sendNewDirectory(); });

    connect(ui->cmdFiltersetDefaults, &QPushButton::clicked, this, [=](){
        ui->txtAcceptFiles->setText("sid,wav,mod,s3m,ym,prg,tpz,tap,pls,tzx,scr,sap,tmc,cmc,dmc,fc,dlt,mpt,cmr,rmt,tm2,cm3,ay,rgb,stp,tfx,tpa,vgm,vgp,iff,gg,sms,med,app");
    });

    connect(ui->cmdFiltersetChipTunes, &QPushButton::clicked, this, [=](){
        ui->txtAcceptFiles->setText("sid,ym,sap,tmc,cmc,dmc,fc,dlt,mpt,cmr,rmt,tm2,cm3,ay,vgm");
    });

    connect(ui->cmdFiltersetAppsAndDatas, &QPushButton::clicked, this, [=](){
        ui->txtAcceptFiles->setText("prg,tpz,tap,pls,tzx,scr,rgb,tpa,iff,gg,sms,app");
    });

    connect(ui->cmdFiltersetNone, &QPushButton::clicked, this, [=](){
        ui->txtAcceptFiles->setText("");
    });

    connect(ui->cmdOpenFTP, &QPushButton::clicked, this, [=](){
        emit reqeustOpenFTP();
    });


    for(int i = 0; i < chkAudioConfigs.size(); i++){
        QCheckBox* chk = chkAudioConfigs[i];
        connect(chk, &QCheckBox::clicked, this, [this, i](){
            //printf("Checkbox %u was clicked\n", i);
            procCheckAudioConf(chkAudioConfigs);
        });
    };

    eqStartupTimer = new QTimer(this);
    eqStartupTimer->setSingleShot(true);
    connect(eqStartupTimer, &QTimer::timeout, this, [this]() {
        eqReady = true;
    });
    eqStartupTimer->start(1000);  // 1 second safety buffer

    eqUpdateTimer = new QTimer(this);
    eqUpdateTimer->setSingleShot(true);
    connect(eqUpdateTimer, &QTimer::timeout, this, &DirectoryBoss::updateEQ);

    connect(ui->sldEQLP, &QSlider::valueChanged, this, [this](int value){
        ui->lblEQLP->setText(QString("%1%").arg(value));
        if(eqReady) eqUpdateTimer->start(60);
    });

    connect(ui->sldEQMP, &QSlider::valueChanged, this, [this](int value){
        ui->lblEQMP->setText(QString("%1%").arg(value));
        if(eqReady) eqUpdateTimer->start(60);
    });

    connect(ui->sldEQHP, &QSlider::valueChanged, this, [this](int value){
        ui->lblEQHP->setText(QString("%1%").arg(value));
        if(eqReady) eqUpdateTimer->start(60);
    });

    connect(ui->sldEQLFQ, &QSlider::valueChanged, this, [this](int value){
        ui->lblEQLFQ->setText(QString("%1hz").arg(value));
        if(eqReady) eqUpdateTimer->start(60);
    });

    connect(ui->sldEQHFQ, &QSlider::valueChanged, this, [this](int value){
        float fraction = value / 1000.0f;  // <- floating point division
        ui->lblEQHFQ->setText(QString::number(fraction, 'f', 3) + "khz"); // 2 decimal places
        if(eqReady) eqUpdateTimer->start(60);
    });


    ui->sldEQLP->setValue(settings->value("sidinterface/EQ_LP").toUInt());
    ui->sldEQMP->setValue(settings->value("sidinterface/EQ_MP").toUInt());
    ui->sldEQHP->setValue(settings->value("sidinterface/EQ_HP").toUInt());
    ui->sldEQLFQ->setValue(settings->value("sidinterface/EQ_LFQ").toUInt());

    ui->lblEQLP->setText(QString("%1%").arg(ui->sldEQLP->value()));
    ui->lblEQMP->setText(QString("%1%").arg(ui->sldEQMP->value()));
    ui->lblEQHP->setText(QString("%1%").arg(ui->sldEQHP->value()));
    ui->lblEQLFQ->setText(QString("%1hz").arg(ui->sldEQLFQ->value()));

    // Set slider from settings
    ui->sldEQHFQ->setValue(settings->value("sidinterface/EQ_HFQ").toUInt());
    float fraction = ui->sldEQHFQ->value() / 1000.0f;
    ui->lblEQHFQ->setText(QString::number(fraction, 'f', 3) + "khz");

    ////////////////////////// KFM COMPILER STUFF ///////////////////////////////////////////////////////
    QTimer* kfmCompileTimer = new QTimer(this);
    kfmCompileTimer->setSingleShot(true);

    connect(kfmCompileTimer, &QTimer::timeout, this, [=](){
        QString sourceText = ui->txtKFMProgram->toPlainText();
        bool autoCompiler = ui->chkLiveCompile->isChecked();

        if(autoCompiler){
            KFM::ParseResult res = KFM::parseKFMProgram(sourceText, QDir::homePath() + "/kfm_buffer.txt", false);
            ui->txtKFMBytecode->setPlainText(res.compiledText);
        }
    });


    connect(ui->txtKFMProgram, &QPlainTextEdit::textChanged, this, [=](){
        kfmCompileTimer->start(300);  // compile 300ms after last keystroke
    });

    connect(ui->cmdCompileKFM, &QPushButton::clicked, this, [=](){
        QString sourceText = ui->txtKFMProgram->toPlainText();   // textbox with program
        QString outputPath = QDir::homePath() + "/kfm_output.txt"; // or any path you choose
        bool writeToe = ui->chkCompileToFile->isChecked(); // your checkbox

        KFM::ParseResult res = KFM::parseKFMProgram(sourceText, outputPath, writeToe);
        ui->txtKFMBytecode->setPlainText(res.compiledText);

        if (res.wroteFile) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath));
        }
    });

    connect(ui->cmdSendKFMByteCode, &QPushButton::clicked, this, [=](){
        QString sourceByteCode = ui->txtKFMBytecode->toPlainText();

        int8_t channelID;
        channelID = ui->txtKFMChannel->text().toInt();
        channelID --;     // take 1 away, since sidbox undersands it as 0..7

        sourceByteCode.replace("\n","");
        sourceByteCode.insert(0, QString("%1").arg(channelID) + " ");
        sourceByteCode.insert(0, "kfmvmbc ");
        //QMessageBox::information(this, "CODE",  QString(sourceByteCode));
        serial->writeData(sourceByteCode + "\n");
    });

    //chkLiveCompile
    connect(ui->cmdChanLess, &QPushButton::clicked, this, [=](){
        int8_t channelID;
        channelID = ui->txtKFMChannel->text().toInt();
        channelID --;     // take 1 away, since sidbox undersands it as 0..7
        if(channelID < 1) channelID = 1;

        ui->txtKFMChannel->setText(QString("%1").arg(channelID));
    });

    connect(ui->cmdChanMore, &QPushButton::clicked, this, [=](){
        int8_t channelID;
        channelID = ui->txtKFMChannel->text().toInt();
        channelID ++;     // take 1 away, since sidbox undersands it as 0..7
        if(channelID >8) channelID = 8;

        ui->txtKFMChannel->setText(QString("%1").arg(channelID));
    });

    connect(ui->cmdKFMToneOn, &QPushButton::clicked, this, [=](){
        sendNoteCMD(0); // "note"
    });

    connect(ui->cmdKFMToneRetrig, &QPushButton::clicked, this, [=](){
        sendNoteCMD(1); // "tnote"
    });

    connect(ui->cmdKFMToneOff, &QPushButton::clicked, this, [=](){
        sendNoteCMD(2); // "noteoff"
    });


    connect(ui->cmdClearKFM, &QPushButton::clicked, this, [this](){
        //int res = QConfirm::show(this, "Are you sure you want to clear this program?", WA_YesNo | WA_CAUTION);
        if (QMessageBox::question(this,
                                  "Confirm",
                                  "Are you sure you want to clear this program?",
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) == QMessageBox::Yes)
        {
            ui->txtKFMProgram->clear();
        }
    });



    connect(ui->cmdSendAdsr, &QPushButton::clicked, this, [=](){

        //txtADSR_Att
        //txtADSR_Dec
        //txtADSR_Sus
        //txtADSR_Rel

        QString cmdToSend = "";
        int8_t channelID;
        channelID = ui->txtKFMChannel->text().toInt();
        channelID --;

        cmdToSend.append("adsr ");
        cmdToSend.append(QString("%1").arg(channelID) + " ");
        cmdToSend.append(ui->txtADSR_Att->text() + " ");
        cmdToSend.append(ui->txtADSR_Dec->text() + " ");
        cmdToSend.append(ui->txtADSR_Sus->text() + " ");
        cmdToSend.append(ui->txtADSR_Rel->text() + " ");
        cmdToSend.append("\n");

        serial->writeData(cmdToSend);
    });

    connect(ui->cmdSaveKFM, &QPushButton::clicked,this, [this] (){
        saveKFM() ;
    });

    connect(ui->cmdLoadKFM, &QPushButton::clicked,this, [this] (){
        loadKFM() ;
    });


    connect(ui->cmdCloseMe, &QPushButton::clicked, this, [=](){ this->close(); });

}


void DirectoryBoss::loadKFM()
{
    QString filter = "KFM Programs (*.kfm);;All Files (*)";
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Load KFM Program",
        QDir::homePath(),
        filter
        );

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot read file!");
        return;
    }

    QTextStream in(&file);

    // === OPTION A: Raw text ===
    ui->txtKFMProgram->setPlainText(in.readAll());


    file.close();
    //QMessageBox::information(this, "Loaded", "KFM program loaded!");
}

void DirectoryBoss::saveKFM()
{
    QString filter = "KFM Programs (*.kfm);;All Files (*)";
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save KFM Program",
        QDir::homePath() + "/myprogram.kfm",  // default
        filter
        );

    if (fileName.isEmpty()) return;

    // Ensure .kfm extension
    if (!fileName.endsWith(".kfm", Qt::CaseInsensitive))
        fileName += ".kfm";

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot write file!");
        return;
    }

    QTextStream out(&file);

    // === OPTION A: Raw text (one big string) ===
    out << ui->txtKFMProgram->toPlainText();

    file.close();
    //QMessageBox::information(this, "Saved", "KFM program saved!");
}


void DirectoryBoss::sendNoteCMD(uint8_t type){
    uint8_t bits;
    int8_t channelID;
    QString toSend = "";
    bits = getPickedGates();

    bits |= 0x01;   // gate on bit

    channelID = ui->txtKFMChannel->text().toInt();
    channelID -= 1;     // take 1 away, since sidbox undersands it as 0..7
    if(channelID < 0) channelID = 0;

    if(type==0) toSend.append("note ");
    if(type==1) toSend.append("tnote ");
    if(type==2) {
        toSend.append("note ");
        bits &= 0xFE;   // keep every bit except the last Gate ON bit ;)
    }


    toSend.append(QString("%1 %2 %3")
                      .arg(channelID) // channel
                      .arg(bits)  // control bits
                      .arg(ui->txtKFMNoteFrequency->text())
                  );
    //QMessageBox::information(this, "Test bits", toSend.toUtf8());
    serial->writeData(toSend + "\n");
}

uint8_t DirectoryBoss::getPickedGates(){
    uint8_t bits;

    bits = 0;
    if(ui->chkToneGateTypePWM->isChecked()) bits += 2;
    if(ui->chkToneGateTypeSAW->isChecked()) bits += 4;
    if(ui->chkToneGateTypeTRI->isChecked()) bits += 8;
    if(ui->chkToneGateTypeNSE->isChecked()) bits += 16;
    return(bits);
}


// Function to build bitmask
void DirectoryBoss::procCheckAudioConf(const QList<QCheckBox*>& box) {
    int bitmask = 0;
    for (int i = 0; i < box.size(); ++i) {
        if (box[i]->isChecked()) {       // <- must use '->' for pointer
            bitmask |= (1 << i);         // set the i-th bit
        }
    }
    //printf("Current bitmask: %d\n", bitmask);
    serial->writeData(QString("audconf 0x%1\r\n").arg(bitmask, 0, 16).toUtf8());
    settings->setValue("audioconfig/channelconf", bitmask);

}

void DirectoryBoss::sendNewDirectory(){
    QString string;

    string = "cd " + ui->txtFilePath->text() + "\r\n";
    //QMessageBox::information(this, "HELLO", string);

    if(serial){
        serial->writeData(string);

        QDateTime dt = QDateTime::currentDateTime();
        QTimer::singleShot(220, this, [this, dt]() {
            char senddatestr[128];
            serial->writeData("dir\r\n");
        });
    } else
        printf("SerialPort NOT open :(\r\n");
}

void DirectoryBoss::onDirListDoubleClicked(const QModelIndex &index)
{
    if (!serial) {
        qDebug() << "SerialPort::Null :(";
        return;
    }

    QString folderName = index.data(Qt::DisplayRole).toString().trimmed();
    if (folderName.isEmpty() || folderName == "." || folderName == "..")
        return;

    // Extract whatever comes after "[DIR]"
    if (folderName.startsWith("[DIR]")) {
        folderName = folderName.mid(5).trimmed(); // remove "[DIR]" (5 chars) and trim spaces
    } else {
        // If it doesn't start with "[DIR]", do nothing

        //QStringList parts = folderName.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        int lastSpace = folderName.lastIndexOf(QRegularExpression("\\s+"));

        //if (!parts.isEmpty()) {
        if (lastSpace != -1) {

            //folderName = parts[0]; // now contains only "filename.app"
            folderName = folderName.left(lastSpace).trimmed();


            // Show filename in message box
            //QMessageBox::information(this, "File Selected", QString("Filename: %1").arg(folderName));



            // Playable music files
            QMap<QString, std::function<void(const QString&)>> handlers;

            QString txForPlay, txForPics, txForEmulator, txForApps, txForMica;
            QString txForC64Tape, txForZXTapeTZX, txForZXTapeTPA;

            txForPlay = QString("play \"%1/%2\"\n").arg(ui->txtFilePath->text()).arg(folderName);
            txForPlay.replace("//","/");

            txForPics = QString("piciff \"%1/%2\"\n").arg(ui->txtFilePath->text()).arg(folderName);
            txForPics.replace("//","/");

            txForEmulator = QString("sega \"%1/%2\"\n").arg(ui->txtFilePath->text()).arg(folderName);
            txForEmulator.replace("//","/");

            txForApps = QString("\"%1/%2\"\n").arg(ui->txtFilePath->text()).arg(folderName);
            txForApps.replace("//","/");

            txForMica = QString("disk \"%1/%2\"\n").arg(ui->txtFilePath->text()).arg(folderName);
            txForMica.replace("//","/");

            txForC64Tape = QString("tape_tap \"%1/%2\"\n").arg(ui->txtFilePath->text()).arg(folderName);
            txForC64Tape.replace("//","/");

            txForZXTapeTZX = QString("tape_tzx \"%1/%2\"\n").arg(ui->txtFilePath->text()).arg(folderName);
            txForZXTapeTZX.replace("//","/");

            txForZXTapeTPA = QString("tape_tzx \"%1/%2\"\n").arg(ui->txtFilePath->text()).arg(folderName);
            txForZXTapeTPA.replace("//","/");


            handlers["wav"] = [this, txForPlay](const QString &f){ serial->writeData(txForPlay.toUtf8()); };
            handlers["sid"] = handlers["wav"];
            handlers["mod"] = handlers["wav"];
            handlers["s3m"] = handlers["wav"];
            handlers["ym"]  = handlers["wav"];
            handlers["sap"] = handlers["wav"];
            handlers["tmc"] = handlers["wav"];
            handlers["cmc"] = handlers["wav"];
            handlers["dmc"] = handlers["wav"];
            handlers["fc"]  = handlers["wav"];
            handlers["dlt"] = handlers["wav"];
            handlers["mpt"] = handlers["wav"];
            handlers["cmr"] = handlers["wav"];
            handlers["rmt"] = handlers["wav"];
            handlers["tm2"] = handlers["wav"];
            handlers["cm3"] = handlers["wav"];
            handlers["ay"]  = handlers["wav"];
            handlers["med"] = handlers["wav"];
            handlers["vgm"] = handlers["wav"];
            handlers["vgp"] = handlers["wav"];
            handlers["stp"] = handlers["wav"];
            handlers["tfx"] = handlers["wav"];

            // Viewable Pictures
            handlers["iff"] = [this, txForPics](const QString &f){ serial->writeData(txForPics.toUtf8()); };

            // Emulator stuff
            handlers["gg"] = [this, txForEmulator](const QString &f){ serial->writeData(txForEmulator.toUtf8()); };
            handlers["sms"] = handlers["gg"];

            // Runnables/commands
            handlers["app"] = [this, txForApps](const QString &f){ serial->writeData(txForApps.toUtf8()); };

            handlers["prg"] = [this, txForMica](const QString &f){ serial->writeData(txForMica.toUtf8()); };

            // spectrum tapes
            handlers["tzx"] = [this, txForZXTapeTZX](const QString &f){ serial->writeData(txForZXTapeTZX.toUtf8()); };
            handlers["tpa"] = [this, txForZXTapeTPA](const QString &f){ serial->writeData(txForZXTapeTPA.toUtf8()); };

            // C64 tapes
            handlers["tap"] = [this, txForC64Tape](const QString &f){ serial->writeData(txForC64Tape.toUtf8()); };

            // ... etc

            QString ext = QFileInfo(folderName).suffix().toLower();
            if (handlers.contains(ext))
                handlers[ext](folderName);
            else
                QMessageBox::information(this,"File Selected", folderName);
        }
        return;
    }

    serial->writeData(QStringLiteral("cd \"%1\"\r\n").arg(folderName).toUtf8());

    // eg: DIR [applets] strip out everything except anything inside the [ and ]
    QTimer::singleShot(220, this, [this]() { serial->writeData("dir\r\n"); });
}

void DirectoryBoss::updateEQ()
{

    //ui->sldEQLP
    //ui->sldEQMP
    //ui->sldEQHP
    //ui->sldEQLFQ
    //ui->sldEQHFQ
    /*
        frmMain.MSComm1.Output = "eq " & EQ_LP_SCR & " " & EQ_MP_SCR & " " & EQ_HP_SCR & " " & EQ_LFQ_SCR.Value & " " & EQ_HFQ_SCR.Value & vbCrLf
        WriteINI "SIDINTERFACE", "EQ_LP", EQ_LP_SCR, App.Path & "/" & inifilename
        WriteINI "SIDINTERFACE", "EQ_MP", EQ_MP_SCR, App.Path & "/" & inifilename
        WriteINI "SIDINTERFACE", "EQ_HP", EQ_HP_SCR, App.Path & "/" & inifilename
    */
    int EQ_LP  = ui->sldEQLP->value();
    int EQ_MP  = ui->sldEQMP->value();
    int EQ_HP  = ui->sldEQHP->value();
    int EQ_LFQ = ui->sldEQLFQ->value();
    int EQ_HFQ = ui->sldEQHFQ->value();

    QString eqCommand = QString("eq %1 %2 %3 %4 %5\n")
                            .arg(EQ_LP)
                            .arg(EQ_MP)
                            .arg(EQ_HP)
                            .arg(EQ_LFQ)
                            .arg(EQ_HFQ);

    serial->writeData(eqCommand.toUtf8());


    //QSettings settings(AppPath + "/" + inifilename, QSettings::IniFormat);

    //settings->setValue("audioconfig/channelconf", bitmask);

    settings->setValue("sidinterface/EQ_LP", EQ_LP);
    settings->setValue("sidinterface/EQ_MP", EQ_MP);
    settings->setValue("sidinterface/EQ_HP", EQ_HP);
    settings->setValue("sidinterface/EQ_LFQ", EQ_LFQ);
    settings->setValue("sidinterface/EQ_HFQ", EQ_HFQ);

    // Optional: immediately sync to disk
    settings->sync();
}

void DirectoryBoss::onSerialInput(const QString &text)
{
    // Split incoming data into lines
    QString allText = leftoverLine + text;
    QStringList lines = allText.split(QRegularExpression("[\r\n]"), Qt::KeepEmptyParts);

    // The last line might be incomplete
    leftoverLine = lines.takeLast(); // store for next chunk

    auto getNumber = [](const QString& line) -> QString {
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        for (const QString& part : parts) {
            if (part[0].isDigit()) {
                // Remove any non-digit suffix like 'k' but keep number
                QString num;
                for (const QChar& c : part) {
                    if (c.isDigit()) num += c;
                    else break;
                }
                return num;
            }
        }
        return "";
    };


    for (const QString &line : lines) {

        /* looking to decode EG. ":sbinf:subsong:m_tfx:9" // we need to
            scan for :sbinf:subsong:
            skip m_tfx:
            get the 9

            set lblSubSongMax to the value of 9 for example
        */
        // === Handle subsong info ===
        if (line.contains(":sbinf:subsong:")) {
            // Example: ":sbinf:subsong:m_tfx:9"
            // Split by ':'
            QStringList parts = line.split(':', Qt::SkipEmptyParts);

            // We expect something like ["sbinf", "subsong", "m_tfx", "9"]
            if (parts.size() >= 4) {
                QString numStr = parts.last(); // "9"
                bool ok = false;
                int value = numStr.toInt(&ok);
                if (ok) {
                    ui->lblSubSongMax->setText(QString::number(value));
                    //qDebug() << "Subsong max set to:" << value;
                }
            }
        }

        if (line.contains("OS: BEGIN NOW")){
            if (ui->chkAutoRedir->isChecked()) {
                QDateTime dt = QDateTime::currentDateTime();
                QTimer::singleShot(220, this, [this, dt]() {
                    char senddatestr[128];
                    serial->writeData("dir\r\n");
                });
            }
            return;
        }


        // ===== Start of DIR LIST =====
        if (line.contains("**DIR_LIST**")) {
            capturingDir = true;
            dirBuffer.clear();
            ui->lstDirList->clear();
            continue;
        }

        // ===== End of DIR LIST =====
        if (line.contains("DIR_OK:>")) {
            capturingDir = false;

            // Split the buffer into lines
            QStringList allLines = dirBuffer.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);

            if (!allLines.isEmpty()) {

                // Skip the first line "path: /"
                int startIdx = 0;
                if (!allLines.isEmpty() && allLines.first().startsWith("path:")) {
                    startIdx = 1;
                    QString path = allLines.first().mid(5).trimmed(); // remove "path:"

                    ui->txtFilePath->setText(path);  // just use QString directly
                }


                // Last 4 lines are summary
                int summaryStart = allLines.size() - 4;

                // Process each real entry
                for (int i = startIdx; i < summaryStart; ++i) {
                    QString clean = allLines[i].trimmed();
                    if (clean.startsWith("DIR ")) {
                        // It's a directory
                        QString name = clean.mid(clean.indexOf('[')); // get "[name]"
                        ui->lstDirList->addItem("[DIR] " + name.mid(1, name.length() - 2));
                    } else {
                        // It's a file with optional size at start
                        QRegularExpression re("^\\s*(\\d+[kK]?)\\s+(.+)$");
                        QRegularExpressionMatch match = re.match(clean);
                        if( ui->txtAcceptFiles->text().length() == 0){
                            // no filters
                            QString size = match.captured(1);
                            QString filename = match.captured(2);
                            // Fixed column for size: 62 chars
                            int totalWidth = console_width_space;
                            //QString line = filename.leftJustified(totalWidth - size.length(), ' ') + size;
                            int nameWidth = totalWidth - size.length();

                            // If filename longer than space → no justification, just a gap
                            QString line;
                            if (filename.length() > nameWidth) {
                                line = filename + " " + size;
                            } else {
                                line = filename.leftJustified(nameWidth, ' ') + size;
                            }


                            ui->lstDirList->addItem(line);

                        } else if (match.hasMatch()) {
                            QString size = match.captured(1);
                            QString filename = match.captured(2);

                            // Extract the file extension in lowercase
                            QString ext = QFileInfo(filename).suffix().toLower();

                            // Get acceptable extensions from the textbox, split by comma, and trim spaces
                            QStringList allowedExts = ui->txtAcceptFiles->text().split(',', Qt::SkipEmptyParts);
                            for (QString &s : allowedExts) s = s.trimmed().toLower();

                            // Only add if the extension is in the allowed list
                            if (allowedExts.contains(ext)) {
                                // Fixed column for size: 62 chars
                                int totalWidth = console_width_space;
                                //QString line = filename.leftJustified(totalWidth - size.length(), ' ') + size;
                                int nameWidth = totalWidth - size.length();

                                // If filename longer than space → no justification, just a gap
                                QString line;
                                if (filename.length() > nameWidth) {
                                    line = filename + " " + size;
                                } else {
                                    line = filename.leftJustified(nameWidth, ' ') + size;
                                }


                                ui->lstDirList->addItem(line);
                            }
                        }

                    }
                }

                // Put last 4 lines into text boxes
                if (summaryStart >= 0) {
                    //ui->txtFileCount->setText(allLines[summaryStart]);   // e.g. "-----------------------------------"
                    ui->txtFiles->setText(getNumber(allLines[summaryStart + 1]));   // e.g. "51 file(s)"
                    ui->txtDirs->setText(getNumber(allLines[summaryStart + 2]));    // e.g. "22 Dir(s)"
                    //ui->txtSize->setText(getNumber(allLines[summaryStart + 3]));    // e.g. "Size 39111k"
                    QString raw = getNumber(allLines[summaryStart + 3]); // "39111k" → "39111"
                    bool ok;
                    double sizeKB = raw.toDouble(&ok);

                    if (ok) {
                        // Format with commas + 2 decimals
                        QString formatted = QLocale().toString(sizeKB, 'f', 0); // "39,111.00"
                        ui->txtSize->setText(formatted + " kb");
                    }
                }
            }

            return;
        }

        // ===== Capture lines in between =====
        if (capturingDir) {
            QString clean = line.trimmed();

            // Store all lines in buffer for later processing
            if (!clean.isEmpty())
                dirBuffer += clean + "\n";
        }
    }
}




void DirectoryBoss::closeEvent(QCloseEvent *event){
    printf("Closed DirectoryBoss()\r\n");
    saveWindowState();
    QMainWindow::closeEvent(event);
}


void DirectoryBoss::saveWindowState()
{
    settings->setValue("directoryboss/x", this->pos().x());
    settings->setValue("directoryboss/y", this->pos().y());
}

void DirectoryBoss::restoreWindowState(){

    int x = settings->value("directoryboss/x").toInt();
    int y = settings->value("directoryboss/y").toInt();

    if(x==0 && y==0){
        QScreen *primary = QGuiApplication::primaryScreen();
        QRect screenGeom = primary->availableGeometry();  // avoids taskbar
        QPoint center = screenGeom.center() - QPoint(this->width()/2, this->height()/2);
        this->move(center);
    } else

        this->move(x,y);

    printf("Restoring Settings here\rn");
}


DirectoryBoss::~DirectoryBoss()
{
    delete ui;
}
