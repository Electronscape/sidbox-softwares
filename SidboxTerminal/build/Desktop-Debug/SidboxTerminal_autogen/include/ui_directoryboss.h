/********************************************************************************
** Form generated from reading UI file 'directoryboss.ui'
**
** Created by: Qt User Interface Compiler version 6.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIRECTORYBOSS_H
#define UI_DIRECTORYBOSS_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSlider>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DirectoryBoss
{
public:
    QWidget *centralwidget;
    QGroupBox *groupBox;
    QPushButton *cmdParentDir;
    QListWidget *lstDirList;
    QPushButton *cmdGetDir;
    QLineEdit *txtFilePath;
    QLineEdit *txtFiles;
    QLineEdit *txtDirs;
    QLineEdit *txtSize;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QPushButton *cmdChangeDir;
    QCheckBox *chkAutoRedir;
    QGroupBox *groupBox_2;
    QLineEdit *txtAcceptFiles;
    QPushButton *cmdFiltersetDefaults;
    QPushButton *cmdFiltersetChipTunes;
    QPushButton *cmdFiltersetAppsAndDatas;
    QPushButton *cmdFiltersetNone;
    QPushButton *cmdOpenFTP;
    QPushButton *cmdCloseMe;
    QFrame *line;
    QGroupBox *groupBox_3;
    QLabel *label_4;
    QScrollBar *scrSubSong;
    QLabel *lblSubSong;
    QPushButton *cmdStopAud;
    QFrame *line_2;
    QGroupBox *groupBox_4;
    QLabel *label_9;
    QLabel *label_10;
    QLabel *label_11;
    QLabel *label_12;
    QLabel *label_13;
    QLabel *lblEQMP;
    QLabel *lblEQLP;
    QLabel *lblEQHP;
    QLabel *lblEQHFQ;
    QLabel *lblEQLFQ;
    QSlider *sldEQLP;
    QSlider *sldEQMP;
    QSlider *sldEQHP;
    QSlider *sldEQLFQ;
    QSlider *sldEQHFQ;
    QGroupBox *groupBox_5;
    QLabel *label_5;
    QCheckBox *chkAudConf21;
    QCheckBox *chkAudConf18;
    QCheckBox *chkAudConf19;
    QCheckBox *chkAudConf17;
    QCheckBox *chkAudConf20;
    QCheckBox *chkAudConf16;
    QLabel *label_6;
    QLabel *label_7;
    QLabel *label_8;
    QCheckBox *chkAudConf0;
    QCheckBox *chkAudConf8;
    QCheckBox *chkAudConf1;
    QCheckBox *chkAudConf9;
    QCheckBox *chkAudConf2;
    QCheckBox *chkAudConf10;
    QCheckBox *chkAudConf3;
    QCheckBox *chkAudConf11;
    QCheckBox *chkAudConf4;
    QCheckBox *chkAudConf12;
    QCheckBox *chkAudConf5;
    QCheckBox *chkAudConf13;
    QCheckBox *chkAudConf6;
    QCheckBox *chkAudConf14;
    QCheckBox *chkAudConf7;
    QCheckBox *chkAudConf15;
    QLabel *label_14;
    QFrame *line_6;
    QLabel *lblSubSong_2;
    QLabel *lblSubSongMax;
    QGroupBox *groupBox_6;
    QGroupBox *groupBox_7;
    QLineEdit *txtKFMNoteFrequency;
    QLabel *label_15;
    QPushButton *cmdKFMToneOn;
    QPushButton *cmdKFMToneOff;
    QLineEdit *txtADSR_Rel;
    QLineEdit *txtADSR_Dec;
    QLineEdit *txtKFMChannel;
    QLineEdit *txtADSR_Att;
    QLineEdit *txtADSR_Sus;
    QCheckBox *chkToneGateTypeNSE;
    QCheckBox *chkToneGateTypePWM;
    QCheckBox *chkToneGateTypeSAW;
    QCheckBox *chkToneGateTypeTRI;
    QLabel *label_16;
    QLabel *label_17;
    QLabel *label_18;
    QLabel *label_19;
    QLabel *label_20;
    QPushButton *cmdKFMToneRetrig;
    QPushButton *cmdSendAdsr;
    QFrame *line_3;
    QFrame *line_4;
    QFrame *line_5;
    QPushButton *cmdChanMore;
    QPushButton *cmdChanLess;
    QPushButton *cmdKFMToneOn_2;
    QGroupBox *groupBox_8;
    QPlainTextEdit *txtKFMProgram;
    QPlainTextEdit *txtKFMBytecode;
    QPushButton *cmdCompileKFM;
    QPushButton *cmdSendKFMByteCode;
    QPushButton *cmdSaveKFM;
    QPushButton *cmdLoadKFM;
    QPushButton *cmdClearKFM;
    QCheckBox *chkCompileToFile;
    QCheckBox *chkLiveCompile;

    void setupUi(QMainWindow *DirectoryBoss)
    {
        if (DirectoryBoss->objectName().isEmpty())
            DirectoryBoss->setObjectName("DirectoryBoss");
        DirectoryBoss->resize(1106, 579);
        DirectoryBoss->setMinimumSize(QSize(1106, 579));
        DirectoryBoss->setMaximumSize(QSize(1106, 579));
        QPalette palette;
        QBrush brush(QColor(88, 88, 88, 255));
        brush.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Window, brush);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Window, brush);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Base, brush);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Window, brush);
        DirectoryBoss->setPalette(palette);
        DirectoryBoss->setAcceptDrops(false);
        QIcon icon(QIcon::fromTheme(QIcon::ThemeIcon::FolderNew));
        DirectoryBoss->setWindowIcon(icon);
        DirectoryBoss->setTabShape(QTabWidget::TabShape::Triangular);
        centralwidget = new QWidget(DirectoryBoss);
        centralwidget->setObjectName("centralwidget");
        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(10, 10, 541, 451));
        groupBox->setFlat(false);
        groupBox->setCheckable(false);
        cmdParentDir = new QPushButton(groupBox);
        cmdParentDir->setObjectName("cmdParentDir");
        cmdParentDir->setGeometry(QRect(490, 30, 40, 26));
        lstDirList = new QListWidget(groupBox);
        new QListWidgetItem(lstDirList);
        new QListWidgetItem(lstDirList);
        new QListWidgetItem(lstDirList);
        new QListWidgetItem(lstDirList);
        new QListWidgetItem(lstDirList);
        new QListWidgetItem(lstDirList);
        new QListWidgetItem(lstDirList);
        new QListWidgetItem(lstDirList);
        new QListWidgetItem(lstDirList);
        new QListWidgetItem(lstDirList);
        new QListWidgetItem(lstDirList);
        new QListWidgetItem(lstDirList);
        new QListWidgetItem(lstDirList);
        lstDirList->setObjectName("lstDirList");
        lstDirList->setGeometry(QRect(10, 60, 522, 331));
        QPalette palette1;
        QBrush brush1(QColor(141, 141, 141, 255));
        brush1.setStyle(Qt::BrushStyle::SolidPattern);
        palette1.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Light, brush1);
        QBrush brush2(QColor(0, 0, 0, 255));
        brush2.setStyle(Qt::BrushStyle::SolidPattern);
        palette1.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Dark, brush2);
        QBrush brush3(QColor(255, 166, 23, 255));
        brush3.setStyle(Qt::BrushStyle::SolidPattern);
        palette1.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Text, brush3);
        palette1.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Base, brush2);
        QBrush brush4(QColor(0, 85, 127, 255));
        brush4.setStyle(Qt::BrushStyle::SolidPattern);
        palette1.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Highlight, brush4);
        QBrush brush5(QColor(0, 255, 255, 255));
        brush5.setStyle(Qt::BrushStyle::SolidPattern);
        palette1.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::HighlightedText, brush5);
        palette1.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Light, brush1);
        palette1.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Dark, brush2);
        palette1.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Text, brush3);
        palette1.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Base, brush2);
        palette1.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Highlight, brush4);
        palette1.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::HighlightedText, brush5);
        palette1.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::WindowText, brush2);
        palette1.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Light, brush1);
        palette1.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Dark, brush2);
        palette1.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Text, brush2);
        palette1.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::ButtonText, brush2);
        palette1.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::HighlightedText, brush5);
        lstDirList->setPalette(palette1);
        QFont font;
        font.setFamilies({QString::fromUtf8("Consolas")});
        font.setPointSize(10);
        lstDirList->setFont(font);
        lstDirList->setFrameShape(QFrame::Shape::WinPanel);
        cmdGetDir = new QPushButton(groupBox);
        cmdGetDir->setObjectName("cmdGetDir");
        cmdGetDir->setGeometry(QRect(10, 30, 70, 26));
        txtFilePath = new QLineEdit(groupBox);
        txtFilePath->setObjectName("txtFilePath");
        txtFilePath->setGeometry(QRect(84, 30, 361, 26));
        QFont font1;
        font1.setFamilies({QString::fromUtf8("Monospace")});
        txtFilePath->setFont(font1);
        txtFilePath->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        txtFiles = new QLineEdit(groupBox);
        txtFiles->setObjectName("txtFiles");
        txtFiles->setGeometry(QRect(80, 400, 60, 20));
        txtFiles->setFont(font1);
        txtFiles->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        txtFiles->setReadOnly(true);
        txtDirs = new QLineEdit(groupBox);
        txtDirs->setObjectName("txtDirs");
        txtDirs->setGeometry(QRect(240, 400, 70, 20));
        txtDirs->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        txtDirs->setReadOnly(true);
        txtSize = new QLineEdit(groupBox);
        txtSize->setObjectName("txtSize");
        txtSize->setGeometry(QRect(420, 400, 113, 20));
        txtSize->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        txtSize->setReadOnly(true);
        label = new QLabel(groupBox);
        label->setObjectName("label");
        label->setGeometry(QRect(10, 400, 70, 18));
        label_2 = new QLabel(groupBox);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(170, 400, 70, 18));
        label_3 = new QLabel(groupBox);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(350, 400, 70, 18));
        cmdChangeDir = new QPushButton(groupBox);
        cmdChangeDir->setObjectName("cmdChangeDir");
        cmdChangeDir->setGeometry(QRect(450, 30, 40, 26));
        chkAutoRedir = new QCheckBox(groupBox);
        chkAutoRedir->setObjectName("chkAutoRedir");
        chkAutoRedir->setGeometry(QRect(10, 420, 171, 23));
        groupBox_2 = new QGroupBox(centralwidget);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setGeometry(QRect(10, 470, 541, 101));
        txtAcceptFiles = new QLineEdit(groupBox_2);
        txtAcceptFiles->setObjectName("txtAcceptFiles");
        txtAcceptFiles->setGeometry(QRect(10, 30, 521, 26));
        QPalette palette2;
        QBrush brush6(QColor(255, 170, 255, 255));
        brush6.setStyle(Qt::BrushStyle::SolidPattern);
        palette2.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Shadow, brush6);
        palette2.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Shadow, brush6);
        palette2.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Shadow, brush6);
        txtAcceptFiles->setPalette(palette2);
        txtAcceptFiles->setAcceptDrops(false);
        txtAcceptFiles->setAutoFillBackground(false);
        txtAcceptFiles->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        cmdFiltersetDefaults = new QPushButton(groupBox_2);
        cmdFiltersetDefaults->setObjectName("cmdFiltersetDefaults");
        cmdFiltersetDefaults->setGeometry(QRect(10, 60, 70, 26));
        QPalette palette3;
        QBrush brush7(QColor(0, 85, 0, 255));
        brush7.setStyle(Qt::BrushStyle::SolidPattern);
        palette3.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush7);
        QBrush brush8(QColor(215, 235, 255, 255));
        brush8.setStyle(Qt::BrushStyle::SolidPattern);
        palette3.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Light, brush8);
        QBrush brush9(QColor(85, 170, 127, 255));
        brush9.setStyle(Qt::BrushStyle::SolidPattern);
        palette3.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Midlight, brush9);
        palette3.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Highlight, brush9);
        QBrush brush10(QColor(0, 255, 0, 255));
        brush10.setStyle(Qt::BrushStyle::SolidPattern);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        palette3.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Accent, brush10);
#endif
        palette3.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush7);
        palette3.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Light, brush8);
        palette3.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Midlight, brush9);
        palette3.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Highlight, brush9);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        palette3.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Accent, brush10);
#endif
        palette3.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush7);
        palette3.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Light, brush8);
        palette3.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Midlight, brush9);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        palette3.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Accent, brush10);
#endif
        cmdFiltersetDefaults->setPalette(palette3);
        cmdFiltersetChipTunes = new QPushButton(groupBox_2);
        cmdFiltersetChipTunes->setObjectName("cmdFiltersetChipTunes");
        cmdFiltersetChipTunes->setGeometry(QRect(82, 60, 80, 26));
        cmdFiltersetAppsAndDatas = new QPushButton(groupBox_2);
        cmdFiltersetAppsAndDatas->setObjectName("cmdFiltersetAppsAndDatas");
        cmdFiltersetAppsAndDatas->setGeometry(QRect(164, 60, 50, 26));
        cmdFiltersetNone = new QPushButton(groupBox_2);
        cmdFiltersetNone->setObjectName("cmdFiltersetNone");
        cmdFiltersetNone->setGeometry(QRect(216, 60, 70, 26));
        cmdOpenFTP = new QPushButton(groupBox_2);
        cmdOpenFTP->setObjectName("cmdOpenFTP");
        cmdOpenFTP->setGeometry(QRect(400, 60, 50, 26));
        cmdOpenFTP->setMinimumSize(QSize(50, 0));
        cmdOpenFTP->setMaximumSize(QSize(50, 16777215));
        QPalette palette4;
        palette4.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush4);
        palette4.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Light, brush8);
        QBrush brush11(QColor(0, 170, 255, 255));
        brush11.setStyle(Qt::BrushStyle::SolidPattern);
        palette4.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Midlight, brush11);
        palette4.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Highlight, brush5);
        QBrush brush12(QColor(85, 255, 255, 255));
        brush12.setStyle(Qt::BrushStyle::SolidPattern);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        palette4.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Accent, brush12);
#endif
        palette4.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush4);
        palette4.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Light, brush8);
        palette4.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Midlight, brush11);
        palette4.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Highlight, brush5);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        palette4.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Accent, brush12);
#endif
        palette4.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush4);
        palette4.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Light, brush8);
        palette4.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Midlight, brush11);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        palette4.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Accent, brush12);
#endif
        cmdOpenFTP->setPalette(palette4);
        cmdCloseMe = new QPushButton(groupBox_2);
        cmdCloseMe->setObjectName("cmdCloseMe");
        cmdCloseMe->setGeometry(QRect(460, 60, 71, 26));
        QPalette palette5;
        QBrush brush13(QColor(117, 0, 0, 255));
        brush13.setStyle(Qt::BrushStyle::SolidPattern);
        palette5.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush13);
        palette5.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Light, brush8);
        QBrush brush14(QColor(236, 56, 56, 255));
        brush14.setStyle(Qt::BrushStyle::SolidPattern);
        palette5.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Midlight, brush14);
        QBrush brush15(QColor(255, 0, 4, 255));
        brush15.setStyle(Qt::BrushStyle::SolidPattern);
        palette5.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Highlight, brush15);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        palette5.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Accent, brush15);
#endif
        palette5.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush13);
        palette5.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Light, brush8);
        palette5.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Midlight, brush14);
        palette5.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Highlight, brush15);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        palette5.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Accent, brush15);
#endif
        palette5.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush13);
        palette5.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Light, brush8);
        palette5.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Midlight, brush14);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        palette5.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Accent, brush15);
#endif
        cmdCloseMe->setPalette(palette5);
        line = new QFrame(centralwidget);
        line->setObjectName("line");
        line->setGeometry(QRect(550, -100, 20, 761));
        line->setFrameShape(QFrame::Shape::VLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);
        groupBox_3 = new QGroupBox(centralwidget);
        groupBox_3->setObjectName("groupBox_3");
        groupBox_3->setGeometry(QRect(570, 10, 521, 221));
        groupBox_3->setFlat(false);
        groupBox_3->setCheckable(false);
        label_4 = new QLabel(groupBox_3);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(12, 33, 71, 18));
        scrSubSong = new QScrollBar(groupBox_3);
        scrSubSong->setObjectName("scrSubSong");
        scrSubSong->setGeometry(QRect(80, 36, 120, 16));
        scrSubSong->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        scrSubSong->setMaximum(31);
        scrSubSong->setOrientation(Qt::Orientation::Horizontal);
        scrSubSong->setInvertedAppearance(false);
        scrSubSong->setInvertedControls(true);
        lblSubSong = new QLabel(groupBox_3);
        lblSubSong->setObjectName("lblSubSong");
        lblSubSong->setGeometry(QRect(210, 33, 20, 20));
        cmdStopAud = new QPushButton(groupBox_3);
        cmdStopAud->setObjectName("cmdStopAud");
        cmdStopAud->setGeometry(QRect(270, 30, 86, 26));
        line_2 = new QFrame(groupBox_3);
        line_2->setObjectName("line_2");
        line_2->setGeometry(QRect(10, 54, 501, 16));
        line_2->setFrameShape(QFrame::Shape::HLine);
        line_2->setFrameShadow(QFrame::Shadow::Sunken);
        groupBox_4 = new QGroupBox(groupBox_3);
        groupBox_4->setObjectName("groupBox_4");
        groupBox_4->setGeometry(QRect(270, 70, 240, 141));
        label_9 = new QLabel(groupBox_4);
        label_9->setObjectName("label_9");
        label_9->setGeometry(QRect(20, 30, 31, 16));
        label_10 = new QLabel(groupBox_4);
        label_10->setObjectName("label_10");
        label_10->setGeometry(QRect(20, 50, 31, 16));
        label_11 = new QLabel(groupBox_4);
        label_11->setObjectName("label_11");
        label_11->setGeometry(QRect(20, 70, 31, 16));
        label_12 = new QLabel(groupBox_4);
        label_12->setObjectName("label_12");
        label_12->setGeometry(QRect(20, 90, 41, 16));
        label_13 = new QLabel(groupBox_4);
        label_13->setObjectName("label_13");
        label_13->setGeometry(QRect(20, 110, 41, 16));
        lblEQMP = new QLabel(groupBox_4);
        lblEQMP->setObjectName("lblEQMP");
        lblEQMP->setGeometry(QRect(180, 50, 40, 16));
        lblEQLP = new QLabel(groupBox_4);
        lblEQLP->setObjectName("lblEQLP");
        lblEQLP->setGeometry(QRect(180, 30, 40, 16));
        lblEQHP = new QLabel(groupBox_4);
        lblEQHP->setObjectName("lblEQHP");
        lblEQHP->setGeometry(QRect(180, 70, 40, 16));
        lblEQHFQ = new QLabel(groupBox_4);
        lblEQHFQ->setObjectName("lblEQHFQ");
        lblEQHFQ->setGeometry(QRect(180, 110, 51, 16));
        lblEQLFQ = new QLabel(groupBox_4);
        lblEQLFQ->setObjectName("lblEQLFQ");
        lblEQLFQ->setGeometry(QRect(180, 90, 51, 16));
        sldEQLP = new QSlider(groupBox_4);
        sldEQLP->setObjectName("sldEQLP");
        sldEQLP->setGeometry(QRect(60, 30, 110, 16));
        sldEQLP->setMaximum(200);
        sldEQLP->setValue(100);
        sldEQLP->setOrientation(Qt::Orientation::Horizontal);
        sldEQMP = new QSlider(groupBox_4);
        sldEQMP->setObjectName("sldEQMP");
        sldEQMP->setGeometry(QRect(60, 50, 110, 16));
        sldEQMP->setMaximum(200);
        sldEQMP->setValue(100);
        sldEQMP->setOrientation(Qt::Orientation::Horizontal);
        sldEQHP = new QSlider(groupBox_4);
        sldEQHP->setObjectName("sldEQHP");
        sldEQHP->setGeometry(QRect(60, 70, 110, 16));
        sldEQHP->setMaximum(200);
        sldEQHP->setValue(100);
        sldEQHP->setOrientation(Qt::Orientation::Horizontal);
        sldEQLFQ = new QSlider(groupBox_4);
        sldEQLFQ->setObjectName("sldEQLFQ");
        sldEQLFQ->setGeometry(QRect(60, 90, 110, 16));
        sldEQLFQ->setMinimum(0);
        sldEQLFQ->setMaximum(880);
        sldEQLFQ->setValue(440);
        sldEQLFQ->setOrientation(Qt::Orientation::Horizontal);
        sldEQHFQ = new QSlider(groupBox_4);
        sldEQHFQ->setObjectName("sldEQHFQ");
        sldEQHFQ->setGeometry(QRect(60, 110, 110, 16));
        sldEQHFQ->setMaximum(7000);
        sldEQHFQ->setValue(3500);
        sldEQHFQ->setOrientation(Qt::Orientation::Horizontal);
        groupBox_5 = new QGroupBox(groupBox_3);
        groupBox_5->setObjectName("groupBox_5");
        groupBox_5->setGeometry(QRect(10, 70, 251, 141));
        label_5 = new QLabel(groupBox_5);
        label_5->setObjectName("label_5");
        label_5->setGeometry(QRect(20, 30, 231, 16));
        chkAudConf21 = new QCheckBox(groupBox_5);
        chkAudConf21->setObjectName("chkAudConf21");
        chkAudConf21->setGeometry(QRect(20, 46, 20, 21));
        chkAudConf18 = new QCheckBox(groupBox_5);
        chkAudConf18->setObjectName("chkAudConf18");
        chkAudConf18->setGeometry(QRect(60, 46, 20, 21));
        chkAudConf19 = new QCheckBox(groupBox_5);
        chkAudConf19->setObjectName("chkAudConf19");
        chkAudConf19->setGeometry(QRect(100, 46, 20, 21));
        chkAudConf17 = new QCheckBox(groupBox_5);
        chkAudConf17->setObjectName("chkAudConf17");
        chkAudConf17->setGeometry(QRect(140, 46, 20, 21));
        chkAudConf20 = new QCheckBox(groupBox_5);
        chkAudConf20->setObjectName("chkAudConf20");
        chkAudConf20->setGeometry(QRect(180, 46, 20, 21));
        chkAudConf16 = new QCheckBox(groupBox_5);
        chkAudConf16->setObjectName("chkAudConf16");
        chkAudConf16->setGeometry(QRect(220, 46, 20, 21));
        label_6 = new QLabel(groupBox_5);
        label_6->setObjectName("label_6");
        label_6->setGeometry(QRect(20, 76, 121, 16));
        label_7 = new QLabel(groupBox_5);
        label_7->setObjectName("label_7");
        label_7->setGeometry(QRect(20, 93, 30, 16));
        label_8 = new QLabel(groupBox_5);
        label_8->setObjectName("label_8");
        label_8->setGeometry(QRect(20, 111, 30, 16));
        chkAudConf0 = new QCheckBox(groupBox_5);
        chkAudConf0->setObjectName("chkAudConf0");
        chkAudConf0->setGeometry(QRect(50, 92, 20, 21));
        chkAudConf8 = new QCheckBox(groupBox_5);
        chkAudConf8->setObjectName("chkAudConf8");
        chkAudConf8->setGeometry(QRect(50, 110, 20, 21));
        chkAudConf1 = new QCheckBox(groupBox_5);
        chkAudConf1->setObjectName("chkAudConf1");
        chkAudConf1->setGeometry(QRect(70, 92, 20, 21));
        chkAudConf9 = new QCheckBox(groupBox_5);
        chkAudConf9->setObjectName("chkAudConf9");
        chkAudConf9->setGeometry(QRect(70, 110, 20, 21));
        chkAudConf2 = new QCheckBox(groupBox_5);
        chkAudConf2->setObjectName("chkAudConf2");
        chkAudConf2->setGeometry(QRect(90, 92, 20, 21));
        chkAudConf10 = new QCheckBox(groupBox_5);
        chkAudConf10->setObjectName("chkAudConf10");
        chkAudConf10->setGeometry(QRect(90, 110, 20, 21));
        chkAudConf3 = new QCheckBox(groupBox_5);
        chkAudConf3->setObjectName("chkAudConf3");
        chkAudConf3->setGeometry(QRect(110, 92, 20, 21));
        chkAudConf11 = new QCheckBox(groupBox_5);
        chkAudConf11->setObjectName("chkAudConf11");
        chkAudConf11->setGeometry(QRect(110, 110, 20, 21));
        chkAudConf4 = new QCheckBox(groupBox_5);
        chkAudConf4->setObjectName("chkAudConf4");
        chkAudConf4->setGeometry(QRect(140, 92, 20, 21));
        chkAudConf12 = new QCheckBox(groupBox_5);
        chkAudConf12->setObjectName("chkAudConf12");
        chkAudConf12->setGeometry(QRect(140, 110, 20, 21));
        chkAudConf5 = new QCheckBox(groupBox_5);
        chkAudConf5->setObjectName("chkAudConf5");
        chkAudConf5->setGeometry(QRect(160, 92, 20, 21));
        chkAudConf13 = new QCheckBox(groupBox_5);
        chkAudConf13->setObjectName("chkAudConf13");
        chkAudConf13->setGeometry(QRect(160, 110, 20, 21));
        chkAudConf6 = new QCheckBox(groupBox_5);
        chkAudConf6->setObjectName("chkAudConf6");
        chkAudConf6->setGeometry(QRect(180, 92, 20, 21));
        chkAudConf14 = new QCheckBox(groupBox_5);
        chkAudConf14->setObjectName("chkAudConf14");
        chkAudConf14->setGeometry(QRect(180, 110, 20, 21));
        chkAudConf7 = new QCheckBox(groupBox_5);
        chkAudConf7->setObjectName("chkAudConf7");
        chkAudConf7->setGeometry(QRect(200, 92, 20, 21));
        chkAudConf15 = new QCheckBox(groupBox_5);
        chkAudConf15->setObjectName("chkAudConf15");
        chkAudConf15->setGeometry(QRect(200, 110, 20, 21));
        label_14 = new QLabel(groupBox_5);
        label_14->setObjectName("label_14");
        label_14->setGeometry(QRect(144, 76, 81, 16));
        line_6 = new QFrame(groupBox_5);
        line_6->setObjectName("line_6");
        line_6->setGeometry(QRect(10, 63, 230, 16));
        line_6->setFrameShape(QFrame::Shape::HLine);
        line_6->setFrameShadow(QFrame::Shadow::Sunken);
        lblSubSong_2 = new QLabel(groupBox_3);
        lblSubSong_2->setObjectName("lblSubSong_2");
        lblSubSong_2->setGeometry(QRect(226, 33, 10, 20));
        lblSubSongMax = new QLabel(groupBox_3);
        lblSubSongMax->setObjectName("lblSubSongMax");
        lblSubSongMax->setGeometry(QRect(232, 33, 20, 20));
        groupBox_6 = new QGroupBox(centralwidget);
        groupBox_6->setObjectName("groupBox_6");
        groupBox_6->setGeometry(QRect(570, 240, 521, 331));
        groupBox_6->setFlat(false);
        groupBox_6->setCheckable(false);
        groupBox_7 = new QGroupBox(groupBox_6);
        groupBox_7->setObjectName("groupBox_7");
        groupBox_7->setGeometry(QRect(250, 30, 261, 186));
        txtKFMNoteFrequency = new QLineEdit(groupBox_7);
        txtKFMNoteFrequency->setObjectName("txtKFMNoteFrequency");
        txtKFMNoteFrequency->setGeometry(QRect(50, 128, 70, 20));
        txtKFMNoteFrequency->setFont(font1);
        txtKFMNoteFrequency->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        txtKFMNoteFrequency->setReadOnly(false);
        label_15 = new QLabel(groupBox_7);
        label_15->setObjectName("label_15");
        label_15->setGeometry(QRect(10, 129, 40, 16));
        cmdKFMToneOn = new QPushButton(groupBox_7);
        cmdKFMToneOn->setObjectName("cmdKFMToneOn");
        cmdKFMToneOn->setGeometry(QRect(130, 125, 37, 26));
        cmdKFMToneOff = new QPushButton(groupBox_7);
        cmdKFMToneOff->setObjectName("cmdKFMToneOff");
        cmdKFMToneOff->setGeometry(QRect(211, 125, 39, 26));
        txtADSR_Rel = new QLineEdit(groupBox_7);
        txtADSR_Rel->setObjectName("txtADSR_Rel");
        txtADSR_Rel->setGeometry(QRect(141, 49, 30, 20));
        txtADSR_Rel->setFont(font1);
        txtADSR_Rel->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        txtADSR_Rel->setReadOnly(false);
        txtADSR_Dec = new QLineEdit(groupBox_7);
        txtADSR_Dec->setObjectName("txtADSR_Dec");
        txtADSR_Dec->setGeometry(QRect(141, 27, 30, 20));
        txtADSR_Dec->setFont(font1);
        txtADSR_Dec->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        txtADSR_Dec->setReadOnly(false);
        txtKFMChannel = new QLineEdit(groupBox_7);
        txtKFMChannel->setObjectName("txtKFMChannel");
        txtKFMChannel->setGeometry(QRect(73, 87, 30, 20));
        QSizePolicy sizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(txtKFMChannel->sizePolicy().hasHeightForWidth());
        txtKFMChannel->setSizePolicy(sizePolicy);
        txtKFMChannel->setFont(font1);
        txtKFMChannel->setStyleSheet(QString::fromUtf8("border: 1px solid #3cAAc3;\n"
""));
        txtKFMChannel->setReadOnly(false);
        txtADSR_Att = new QLineEdit(groupBox_7);
        txtADSR_Att->setObjectName("txtADSR_Att");
        txtADSR_Att->setGeometry(QRect(50, 27, 30, 20));
        txtADSR_Att->setFont(font1);
        txtADSR_Att->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        txtADSR_Att->setReadOnly(false);
        txtADSR_Sus = new QLineEdit(groupBox_7);
        txtADSR_Sus->setObjectName("txtADSR_Sus");
        txtADSR_Sus->setGeometry(QRect(50, 49, 30, 20));
        txtADSR_Sus->setFont(font1);
        txtADSR_Sus->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        txtADSR_Sus->setReadOnly(false);
        chkToneGateTypeNSE = new QCheckBox(groupBox_7);
        chkToneGateTypeNSE->setObjectName("chkToneGateTypeNSE");
        chkToneGateTypeNSE->setGeometry(QRect(190, 90, 61, 21));
        chkToneGateTypePWM = new QCheckBox(groupBox_7);
        chkToneGateTypePWM->setObjectName("chkToneGateTypePWM");
        chkToneGateTypePWM->setGeometry(QRect(190, 30, 61, 21));
        chkToneGateTypeSAW = new QCheckBox(groupBox_7);
        chkToneGateTypeSAW->setObjectName("chkToneGateTypeSAW");
        chkToneGateTypeSAW->setGeometry(QRect(190, 50, 61, 21));
        chkToneGateTypeTRI = new QCheckBox(groupBox_7);
        chkToneGateTypeTRI->setObjectName("chkToneGateTypeTRI");
        chkToneGateTypeTRI->setGeometry(QRect(190, 70, 61, 21));
        label_16 = new QLabel(groupBox_7);
        label_16->setObjectName("label_16");
        label_16->setGeometry(QRect(10, 88, 40, 16));
        label_17 = new QLabel(groupBox_7);
        label_17->setObjectName("label_17");
        label_17->setGeometry(QRect(10, 28, 40, 16));
        label_18 = new QLabel(groupBox_7);
        label_18->setObjectName("label_18");
        label_18->setGeometry(QRect(101, 28, 40, 16));
        label_19 = new QLabel(groupBox_7);
        label_19->setObjectName("label_19");
        label_19->setGeometry(QRect(10, 50, 40, 16));
        label_20 = new QLabel(groupBox_7);
        label_20->setObjectName("label_20");
        label_20->setGeometry(QRect(101, 50, 40, 16));
        cmdKFMToneRetrig = new QPushButton(groupBox_7);
        cmdKFMToneRetrig->setObjectName("cmdKFMToneRetrig");
        cmdKFMToneRetrig->setGeometry(QRect(169, 125, 40, 26));
        cmdKFMToneRetrig->setBaseSize(QSize(0, 0));
        cmdSendAdsr = new QPushButton(groupBox_7);
        cmdSendAdsr->setObjectName("cmdSendAdsr");
        cmdSendAdsr->setGeometry(QRect(129, 84, 47, 26));
        line_3 = new QFrame(groupBox_7);
        line_3->setObjectName("line_3");
        line_3->setGeometry(QRect(10, 70, 170, 16));
        line_3->setFrameShape(QFrame::Shape::HLine);
        line_3->setFrameShadow(QFrame::Shadow::Sunken);
        line_4 = new QFrame(groupBox_7);
        line_4->setObjectName("line_4");
        line_4->setGeometry(QRect(180, 30, 3, 80));
        line_4->setFrameShape(QFrame::Shape::VLine);
        line_4->setFrameShadow(QFrame::Shadow::Sunken);
        line_5 = new QFrame(groupBox_7);
        line_5->setObjectName("line_5");
        line_5->setGeometry(QRect(10, 110, 240, 16));
        line_5->setFrameShape(QFrame::Shape::HLine);
        line_5->setFrameShadow(QFrame::Shadow::Sunken);
        cmdChanMore = new QPushButton(groupBox_7);
        cmdChanMore->setObjectName("cmdChanMore");
        cmdChanMore->setGeometry(QRect(107, 84, 20, 26));
        cmdChanLess = new QPushButton(groupBox_7);
        cmdChanLess->setObjectName("cmdChanLess");
        cmdChanLess->setGeometry(QRect(50, 84, 20, 26));
        cmdKFMToneOn_2 = new QPushButton(groupBox_7);
        cmdKFMToneOn_2->setObjectName("cmdKFMToneOn_2");
        cmdKFMToneOn_2->setGeometry(QRect(130, 153, 120, 26));
        groupBox_8 = new QGroupBox(groupBox_6);
        groupBox_8->setObjectName("groupBox_8");
        groupBox_8->setGeometry(QRect(10, 30, 231, 290));
        txtKFMProgram = new QPlainTextEdit(groupBox_8);
        txtKFMProgram->setObjectName("txtKFMProgram");
        txtKFMProgram->setGeometry(QRect(10, 31, 131, 250));
        QPalette palette6;
        palette6.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Light, brush1);
        palette6.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Dark, brush2);
        QBrush brush16(QColor(255, 255, 0, 255));
        brush16.setStyle(Qt::BrushStyle::SolidPattern);
        palette6.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Text, brush16);
        QBrush brush17(QColor(80, 80, 80, 255));
        brush17.setStyle(Qt::BrushStyle::SolidPattern);
        palette6.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Base, brush17);
        palette6.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Light, brush1);
        palette6.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Dark, brush2);
        palette6.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Text, brush16);
        palette6.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Base, brush17);
        palette6.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::WindowText, brush2);
        palette6.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Light, brush1);
        palette6.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Dark, brush2);
        palette6.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Text, brush2);
        palette6.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::ButtonText, brush2);
        txtKFMProgram->setPalette(palette6);
        QFont font2;
        font2.setFamilies({QString::fromUtf8("Consolas")});
        font2.setPointSize(9);
        txtKFMProgram->setFont(font2);
        txtKFMProgram->setAcceptDrops(false);
        txtKFMProgram->setFrameShape(QFrame::Shape::Panel);
        txtKFMProgram->setFrameShadow(QFrame::Shadow::Sunken);
        txtKFMProgram->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
        txtKFMProgram->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
        txtKFMProgram->setPlainText(QString::fromUtf8("ATT 8\n"
"DCY 20\n"
"SUS 80\n"
"REL 2\n"
"GTE 3\n"
"STV XV1, 8\n"
"STV XV0, 10\n"
"ADD XV0, XV1\n"
"PWM XV0\n"
"NOP 2\n"
"CMP XV0 8\n"
"JIL 5\n"
"CMP XV0 62\n"
"JIL -6\n"
"STV XVI, -2\n"
"JMP -8\n"
"STV XVI1, 2\n"
"JMP -11\n"
"END"));
        txtKFMBytecode = new QPlainTextEdit(groupBox_8);
        txtKFMBytecode->setObjectName("txtKFMBytecode");
        txtKFMBytecode->setGeometry(QRect(150, 31, 71, 250));
        QPalette palette7;
        palette7.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Light, brush1);
        palette7.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Dark, brush2);
        palette7.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Text, brush16);
        palette7.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Base, brush17);
        palette7.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Light, brush1);
        palette7.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Dark, brush2);
        palette7.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Text, brush16);
        palette7.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Base, brush17);
        palette7.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::WindowText, brush2);
        palette7.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Light, brush1);
        palette7.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Dark, brush2);
        palette7.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Text, brush2);
        palette7.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::ButtonText, brush2);
        txtKFMBytecode->setPalette(palette7);
        txtKFMBytecode->setFont(font2);
        txtKFMBytecode->setFrameShape(QFrame::Shape::Panel);
        txtKFMBytecode->setFrameShadow(QFrame::Shadow::Sunken);
        txtKFMBytecode->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
        txtKFMBytecode->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
        txtKFMBytecode->setPlainText(QString::fromUtf8(""));
        cmdCompileKFM = new QPushButton(groupBox_6);
        cmdCompileKFM->setObjectName("cmdCompileKFM");
        cmdCompileKFM->setGeometry(QRect(351, 290, 97, 30));
        cmdSendKFMByteCode = new QPushButton(groupBox_6);
        cmdSendKFMByteCode->setObjectName("cmdSendKFMByteCode");
        cmdSendKFMByteCode->setGeometry(QRect(450, 290, 60, 30));
        cmdSaveKFM = new QPushButton(groupBox_6);
        cmdSaveKFM->setObjectName("cmdSaveKFM");
        cmdSaveKFM->setGeometry(QRect(250, 290, 50, 30));
        cmdSaveKFM->setStyleSheet(QString::fromUtf8(""));
        cmdLoadKFM = new QPushButton(groupBox_6);
        cmdLoadKFM->setObjectName("cmdLoadKFM");
        cmdLoadKFM->setGeometry(QRect(302, 290, 47, 30));
        cmdClearKFM = new QPushButton(groupBox_6);
        cmdClearKFM->setObjectName("cmdClearKFM");
        cmdClearKFM->setGeometry(QRect(250, 258, 50, 30));
        chkCompileToFile = new QCheckBox(groupBox_6);
        chkCompileToFile->setObjectName("chkCompileToFile");
        chkCompileToFile->setGeometry(QRect(410, 263, 110, 21));
        chkLiveCompile = new QCheckBox(groupBox_6);
        chkLiveCompile->setObjectName("chkLiveCompile");
        chkLiveCompile->setGeometry(QRect(310, 263, 110, 21));
        DirectoryBoss->setCentralWidget(centralwidget);

        retranslateUi(DirectoryBoss);

        QMetaObject::connectSlotsByName(DirectoryBoss);
    } // setupUi

    void retranslateUi(QMainWindow *DirectoryBoss)
    {
        DirectoryBoss->setWindowTitle(QCoreApplication::translate("DirectoryBoss", "SIDBOX DirectoryBoss", nullptr));
        groupBox->setTitle(QCoreApplication::translate("DirectoryBoss", "File Listbox", nullptr));
        cmdParentDir->setText(QCoreApplication::translate("DirectoryBoss", "../", nullptr));

        const bool __sortingEnabled = lstDirList->isSortingEnabled();
        lstDirList->setSortingEnabled(false);
        QListWidgetItem *___qlistwidgetitem = lstDirList->item(0);
        ___qlistwidgetitem->setText(QCoreApplication::translate("DirectoryBoss", "applet.app", nullptr));
        QListWidgetItem *___qlistwidgetitem1 = lstDirList->item(1);
        ___qlistwidgetitem1->setText(QCoreApplication::translate("DirectoryBoss", "song.ay", nullptr));
        QListWidgetItem *___qlistwidgetitem2 = lstDirList->item(2);
        ___qlistwidgetitem2->setText(QCoreApplication::translate("DirectoryBoss", "song.med", nullptr));
        QListWidgetItem *___qlistwidgetitem3 = lstDirList->item(3);
        ___qlistwidgetitem3->setText(QCoreApplication::translate("DirectoryBoss", "song.mod", nullptr));
        QListWidgetItem *___qlistwidgetitem4 = lstDirList->item(4);
        ___qlistwidgetitem4->setText(QCoreApplication::translate("DirectoryBoss", "song.s3m", nullptr));
        QListWidgetItem *___qlistwidgetitem5 = lstDirList->item(5);
        ___qlistwidgetitem5->setText(QCoreApplication::translate("DirectoryBoss", "song.sap", nullptr));
        QListWidgetItem *___qlistwidgetitem6 = lstDirList->item(6);
        ___qlistwidgetitem6->setText(QCoreApplication::translate("DirectoryBoss", "song.sid", nullptr));
        QListWidgetItem *___qlistwidgetitem7 = lstDirList->item(7);
        ___qlistwidgetitem7->setText(QCoreApplication::translate("DirectoryBoss", "song.stp", nullptr));
        QListWidgetItem *___qlistwidgetitem8 = lstDirList->item(8);
        ___qlistwidgetitem8->setText(QCoreApplication::translate("DirectoryBoss", "song.tfx", nullptr));
        QListWidgetItem *___qlistwidgetitem9 = lstDirList->item(9);
        ___qlistwidgetitem9->setText(QCoreApplication::translate("DirectoryBoss", "song.vgm", nullptr));
        QListWidgetItem *___qlistwidgetitem10 = lstDirList->item(10);
        ___qlistwidgetitem10->setText(QCoreApplication::translate("DirectoryBoss", "song.vgp", nullptr));
        QListWidgetItem *___qlistwidgetitem11 = lstDirList->item(11);
        ___qlistwidgetitem11->setText(QCoreApplication::translate("DirectoryBoss", "song.wav", nullptr));
        QListWidgetItem *___qlistwidgetitem12 = lstDirList->item(12);
        ___qlistwidgetitem12->setText(QCoreApplication::translate("DirectoryBoss", "song.ym", nullptr));
        lstDirList->setSortingEnabled(__sortingEnabled);

        cmdGetDir->setText(QCoreApplication::translate("DirectoryBoss", "get dir...", nullptr));
        label->setText(QCoreApplication::translate("DirectoryBoss", "Total Files:", nullptr));
        label_2->setText(QCoreApplication::translate("DirectoryBoss", "Total Dirs:", nullptr));
        label_3->setText(QCoreApplication::translate("DirectoryBoss", "Total Size:", nullptr));
        cmdChangeDir->setText(QCoreApplication::translate("DirectoryBoss", "CD", nullptr));
        chkAutoRedir->setText(QCoreApplication::translate("DirectoryBoss", "Auto directory on reset", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("DirectoryBoss", "Filetypes and Filters", nullptr));
#if QT_CONFIG(tooltip)
        txtAcceptFiles->setToolTip(QString());
#endif // QT_CONFIG(tooltip)
        txtAcceptFiles->setText(QCoreApplication::translate("DirectoryBoss", "sid,wav,mod,s3m,ym,prg,tpz,tap,pls,tzx,scr,sap,tmc,cmc,dmc,fc,dlt,mpt,cmr,rmt,tm2,cm3,ay,rgb,stp,tfx,tpa,vgm,vgp,iff,gg,sms,med,app", nullptr));
        cmdFiltersetDefaults->setText(QCoreApplication::translate("DirectoryBoss", "Default", nullptr));
        cmdFiltersetChipTunes->setText(QCoreApplication::translate("DirectoryBoss", "chip tunes", nullptr));
        cmdFiltersetAppsAndDatas->setText(QCoreApplication::translate("DirectoryBoss", "apps", nullptr));
        cmdFiltersetNone->setText(QCoreApplication::translate("DirectoryBoss", "*.* (all)", nullptr));
        cmdOpenFTP->setText(QCoreApplication::translate("DirectoryBoss", "FTP...", nullptr));
        cmdCloseMe->setText(QCoreApplication::translate("DirectoryBoss", "[ close ]", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("DirectoryBoss", "Audio Config / Play Systems", nullptr));
        label_4->setText(QCoreApplication::translate("DirectoryBoss", "Sub-Song:", nullptr));
        lblSubSong->setText(QCoreApplication::translate("DirectoryBoss", "0", nullptr));
        cmdStopAud->setText(QCoreApplication::translate("DirectoryBoss", "Stop Audio", nullptr));
        groupBox_4->setTitle(QCoreApplication::translate("DirectoryBoss", "EQ Settings", nullptr));
        label_9->setText(QCoreApplication::translate("DirectoryBoss", "LP:", nullptr));
        label_10->setText(QCoreApplication::translate("DirectoryBoss", "MP:", nullptr));
        label_11->setText(QCoreApplication::translate("DirectoryBoss", "HP:", nullptr));
        label_12->setText(QCoreApplication::translate("DirectoryBoss", "LFQ:", nullptr));
        label_13->setText(QCoreApplication::translate("DirectoryBoss", "HFQ:", nullptr));
        lblEQMP->setText(QCoreApplication::translate("DirectoryBoss", "100%", nullptr));
        lblEQLP->setText(QCoreApplication::translate("DirectoryBoss", "100%", nullptr));
        lblEQHP->setText(QCoreApplication::translate("DirectoryBoss", "100%", nullptr));
        lblEQHFQ->setText(QCoreApplication::translate("DirectoryBoss", "3.5khz", nullptr));
        lblEQLFQ->setText(QCoreApplication::translate("DirectoryBoss", "440hz", nullptr));
        groupBox_5->setTitle(QCoreApplication::translate("DirectoryBoss", "Channel and Sound configs", nullptr));
        label_5->setText(QCoreApplication::translate("DirectoryBoss", "KFM   AGC   STRS    EQ     FDR     AF", nullptr));
        chkAudConf21->setText(QString());
        chkAudConf18->setText(QString());
        chkAudConf19->setText(QString());
        chkAudConf17->setText(QString());
        chkAudConf20->setText(QString());
        chkAudConf16->setText(QString());
        label_6->setText(QCoreApplication::translate("DirectoryBoss", "CH:   1   2   3   4", nullptr));
        label_7->setText(QCoreApplication::translate("DirectoryBoss", "En:", nullptr));
        label_8->setText(QCoreApplication::translate("DirectoryBoss", "LR:", nullptr));
        chkAudConf0->setText(QString());
        chkAudConf8->setText(QString());
        chkAudConf1->setText(QString());
        chkAudConf9->setText(QString());
        chkAudConf2->setText(QString());
        chkAudConf10->setText(QString());
        chkAudConf3->setText(QString());
        chkAudConf11->setText(QString());
        chkAudConf4->setText(QString());
        chkAudConf12->setText(QString());
        chkAudConf5->setText(QString());
        chkAudConf13->setText(QString());
        chkAudConf6->setText(QString());
        chkAudConf14->setText(QString());
        chkAudConf7->setText(QString());
        chkAudConf15->setText(QString());
        label_14->setText(QCoreApplication::translate("DirectoryBoss", "5   6   7   8", nullptr));
        lblSubSong_2->setText(QCoreApplication::translate("DirectoryBoss", "/", nullptr));
        lblSubSongMax->setText(QCoreApplication::translate("DirectoryBoss", "0", nullptr));
        groupBox_6->setTitle(QCoreApplication::translate("DirectoryBoss", "KFM Programming and testing", nullptr));
        groupBox_7->setTitle(QCoreApplication::translate("DirectoryBoss", "Wave bits", nullptr));
        txtKFMNoteFrequency->setText(QCoreApplication::translate("DirectoryBoss", "440", nullptr));
        label_15->setText(QCoreApplication::translate("DirectoryBoss", "Freq:", nullptr));
        cmdKFMToneOn->setText(QCoreApplication::translate("DirectoryBoss", "on", nullptr));
        cmdKFMToneOff->setText(QCoreApplication::translate("DirectoryBoss", "off", nullptr));
        txtADSR_Rel->setText(QCoreApplication::translate("DirectoryBoss", "21", nullptr));
        txtADSR_Dec->setText(QCoreApplication::translate("DirectoryBoss", "21", nullptr));
        txtKFMChannel->setText(QCoreApplication::translate("DirectoryBoss", "1", nullptr));
        txtADSR_Att->setText(QCoreApplication::translate("DirectoryBoss", "21", nullptr));
        txtADSR_Sus->setText(QCoreApplication::translate("DirectoryBoss", "80", nullptr));
        chkToneGateTypeNSE->setText(QCoreApplication::translate("DirectoryBoss", "nse", nullptr));
        chkToneGateTypePWM->setText(QCoreApplication::translate("DirectoryBoss", "pwm", nullptr));
        chkToneGateTypeSAW->setText(QCoreApplication::translate("DirectoryBoss", "saw", nullptr));
        chkToneGateTypeTRI->setText(QCoreApplication::translate("DirectoryBoss", "tri", nullptr));
        label_16->setText(QCoreApplication::translate("DirectoryBoss", "Chan:", nullptr));
        label_17->setText(QCoreApplication::translate("DirectoryBoss", "Attk:", nullptr));
        label_18->setText(QCoreApplication::translate("DirectoryBoss", "Dec:", nullptr));
        label_19->setText(QCoreApplication::translate("DirectoryBoss", "Sus:", nullptr));
        label_20->setText(QCoreApplication::translate("DirectoryBoss", "Rel:", nullptr));
        cmdKFMToneRetrig->setText(QCoreApplication::translate("DirectoryBoss", "t.on", nullptr));
        cmdSendAdsr->setText(QCoreApplication::translate("DirectoryBoss", "send", nullptr));
        cmdChanMore->setText(QCoreApplication::translate("DirectoryBoss", ">", nullptr));
        cmdChanLess->setText(QCoreApplication::translate("DirectoryBoss", "<", nullptr));
        cmdKFMToneOn_2->setText(QCoreApplication::translate("DirectoryBoss", "stop all KFM notes", nullptr));
        groupBox_8->setTitle(QCoreApplication::translate("DirectoryBoss", "KFM Assembly", nullptr));
#if QT_CONFIG(whatsthis)
        txtKFMProgram->setWhatsThis(QCoreApplication::translate("DirectoryBoss", "KFM Asm code", nullptr));
#endif // QT_CONFIG(whatsthis)
        cmdCompileKFM->setText(QCoreApplication::translate("DirectoryBoss", "compile >>>", nullptr));
        cmdSendKFMByteCode->setText(QCoreApplication::translate("DirectoryBoss", "send", nullptr));
        cmdSaveKFM->setText(QCoreApplication::translate("DirectoryBoss", "Save", nullptr));
        cmdLoadKFM->setText(QCoreApplication::translate("DirectoryBoss", "Load", nullptr));
        cmdClearKFM->setText(QCoreApplication::translate("DirectoryBoss", "clear", nullptr));
        chkCompileToFile->setText(QCoreApplication::translate("DirectoryBoss", "compile to file", nullptr));
        chkLiveCompile->setText(QCoreApplication::translate("DirectoryBoss", "live compile", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DirectoryBoss: public Ui_DirectoryBoss {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIRECTORYBOSS_H
