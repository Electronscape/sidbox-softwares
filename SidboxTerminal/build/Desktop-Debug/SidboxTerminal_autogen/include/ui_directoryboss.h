/********************************************************************************
** Form generated from reading UI file 'directoryboss.ui'
**
** Created by: Qt User Interface Compiler version 6.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIRECTORYBOSS_H
#define UI_DIRECTORYBOSS_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>
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
    QPushButton *cmdStopAud;
    QScrollBar *scrSubSong;
    QLabel *label_4;
    QLabel *lblSubSong;
    QPushButton *cmdFiltersetNone;
    QPushButton *cmdOpenFTP;

    void setupUi(QMainWindow *DirectoryBoss)
    {
        if (DirectoryBoss->objectName().isEmpty())
            DirectoryBoss->setObjectName("DirectoryBoss");
        DirectoryBoss->resize(560, 575);
        QIcon icon(QIcon::fromTheme(QIcon::ThemeIcon::FolderNew));
        DirectoryBoss->setWindowIcon(icon);
        centralwidget = new QWidget(DirectoryBoss);
        centralwidget->setObjectName("centralwidget");
        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(10, 10, 541, 421));
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
        lstDirList->setGeometry(QRect(10, 60, 521, 301));
        QPalette palette;
        QBrush brush(QColor(0, 0, 0, 255));
        brush.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Dark, brush);
        QBrush brush1(QColor(0, 255, 0, 255));
        brush1.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Text, brush1);
        QBrush brush2(QColor(26, 31, 63, 255));
        brush2.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Base, brush2);
        QBrush brush3(QColor(0, 85, 127, 255));
        brush3.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Highlight, brush3);
        QBrush brush4(QColor(0, 255, 255, 255));
        brush4.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::HighlightedText, brush4);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Dark, brush);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Text, brush1);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Base, brush2);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Highlight, brush3);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::HighlightedText, brush4);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::WindowText, brush);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Dark, brush);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Text, brush);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::ButtonText, brush);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::HighlightedText, brush4);
        lstDirList->setPalette(palette);
        QFont font;
        font.setFamilies({QString::fromUtf8("Monospace")});
        lstDirList->setFont(font);
        lstDirList->setFrameShape(QFrame::Shape::WinPanel);
        cmdGetDir = new QPushButton(groupBox);
        cmdGetDir->setObjectName("cmdGetDir");
        cmdGetDir->setGeometry(QRect(10, 30, 70, 26));
        txtFilePath = new QLineEdit(groupBox);
        txtFilePath->setObjectName("txtFilePath");
        txtFilePath->setGeometry(QRect(84, 30, 361, 26));
        txtFilePath->setFont(font);
        txtFilePath->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        txtFiles = new QLineEdit(groupBox);
        txtFiles->setObjectName("txtFiles");
        txtFiles->setGeometry(QRect(80, 370, 60, 20));
        txtFiles->setFont(font);
        txtFiles->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        txtFiles->setReadOnly(true);
        txtDirs = new QLineEdit(groupBox);
        txtDirs->setObjectName("txtDirs");
        txtDirs->setGeometry(QRect(240, 370, 70, 20));
        txtDirs->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        txtDirs->setReadOnly(true);
        txtSize = new QLineEdit(groupBox);
        txtSize->setObjectName("txtSize");
        txtSize->setGeometry(QRect(420, 370, 113, 20));
        txtSize->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        txtSize->setReadOnly(true);
        label = new QLabel(groupBox);
        label->setObjectName("label");
        label->setGeometry(QRect(10, 370, 70, 18));
        label_2 = new QLabel(groupBox);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(170, 370, 70, 18));
        label_3 = new QLabel(groupBox);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(350, 370, 70, 18));
        cmdChangeDir = new QPushButton(groupBox);
        cmdChangeDir->setObjectName("cmdChangeDir");
        cmdChangeDir->setGeometry(QRect(450, 30, 40, 26));
        chkAutoRedir = new QCheckBox(groupBox);
        chkAutoRedir->setObjectName("chkAutoRedir");
        chkAutoRedir->setGeometry(QRect(10, 390, 171, 23));
        groupBox_2 = new QGroupBox(centralwidget);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setGeometry(QRect(10, 440, 541, 131));
        txtAcceptFiles = new QLineEdit(groupBox_2);
        txtAcceptFiles->setObjectName("txtAcceptFiles");
        txtAcceptFiles->setGeometry(QRect(10, 30, 521, 26));
        QPalette palette1;
        QBrush brush5(QColor(255, 170, 255, 255));
        brush5.setStyle(Qt::BrushStyle::SolidPattern);
        palette1.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Shadow, brush5);
        palette1.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Shadow, brush5);
        palette1.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Shadow, brush5);
        txtAcceptFiles->setPalette(palette1);
        txtAcceptFiles->setAcceptDrops(false);
        txtAcceptFiles->setAutoFillBackground(false);
        txtAcceptFiles->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        cmdFiltersetDefaults = new QPushButton(groupBox_2);
        cmdFiltersetDefaults->setObjectName("cmdFiltersetDefaults");
        cmdFiltersetDefaults->setGeometry(QRect(10, 60, 86, 26));
        cmdFiltersetChipTunes = new QPushButton(groupBox_2);
        cmdFiltersetChipTunes->setObjectName("cmdFiltersetChipTunes");
        cmdFiltersetChipTunes->setGeometry(QRect(100, 60, 86, 26));
        cmdFiltersetAppsAndDatas = new QPushButton(groupBox_2);
        cmdFiltersetAppsAndDatas->setObjectName("cmdFiltersetAppsAndDatas");
        cmdFiltersetAppsAndDatas->setGeometry(QRect(190, 60, 86, 26));
        cmdStopAud = new QPushButton(groupBox_2);
        cmdStopAud->setObjectName("cmdStopAud");
        cmdStopAud->setGeometry(QRect(440, 90, 86, 26));
        scrSubSong = new QScrollBar(groupBox_2);
        scrSubSong->setObjectName("scrSubSong");
        scrSubSong->setGeometry(QRect(80, 100, 160, 16));
        scrSubSong->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        scrSubSong->setOrientation(Qt::Orientation::Horizontal);
        scrSubSong->setInvertedAppearance(false);
        scrSubSong->setInvertedControls(true);
        label_4 = new QLabel(groupBox_2);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(10, 100, 71, 18));
        lblSubSong = new QLabel(groupBox_2);
        lblSubSong->setObjectName("lblSubSong");
        lblSubSong->setGeometry(QRect(250, 100, 58, 18));
        cmdFiltersetNone = new QPushButton(groupBox_2);
        cmdFiltersetNone->setObjectName("cmdFiltersetNone");
        cmdFiltersetNone->setGeometry(QRect(280, 60, 86, 26));
        cmdOpenFTP = new QPushButton(groupBox_2);
        cmdOpenFTP->setObjectName("cmdOpenFTP");
        cmdOpenFTP->setGeometry(QRect(480, 60, 50, 26));
        cmdOpenFTP->setMinimumSize(QSize(50, 0));
        cmdOpenFTP->setMaximumSize(QSize(50, 16777215));
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
        cmdStopAud->setText(QCoreApplication::translate("DirectoryBoss", "Stop Audio", nullptr));
        label_4->setText(QCoreApplication::translate("DirectoryBoss", "Sub-Song:", nullptr));
        lblSubSong->setText(QCoreApplication::translate("DirectoryBoss", "0", nullptr));
        cmdFiltersetNone->setText(QCoreApplication::translate("DirectoryBoss", "*.* (all)", nullptr));
        cmdOpenFTP->setText(QCoreApplication::translate("DirectoryBoss", "FTP...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DirectoryBoss: public Ui_DirectoryBoss {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIRECTORYBOSS_H
