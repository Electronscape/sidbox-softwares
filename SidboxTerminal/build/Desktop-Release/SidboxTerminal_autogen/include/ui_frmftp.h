/********************************************************************************
** Form generated from reading UI file 'frmftp.ui'
**
** Created by: Qt User Interface Compiler version 6.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FRMFTP_H
#define UI_FRMFTP_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FrmFTP
{
public:
    QWidget *centralwidget;
    QFrame *frame;
    QLabel *label_2;
    QLineEdit *txtStartAddress;
    QRadioButton *opt1Ram;
    QRadioButton *opt1File;
    QRadioButton *opt1FileOverwrite;
    QPushButton *cmdCloseFTP;
    QCheckBox *chkClearLogAuto;
    QPushButton *cmdClearLog;
    QLabel *label;
    QFrame *frame_2;
    QRadioButton *opt2Chunk1;
    QRadioButton *opt2Chunk2;
    QRadioButton *opt2Chunk3;
    QRadioButton *opt2Chunk4;
    QRadioButton *opt2Chunk5;
    QLabel *label_3;
    QPushButton *cmdClearRam;
    QPushButton *cmdSendFile;
    QPushButton *cmdRunInRam;
    QPushButton *cmdViewRam;
    QCheckBox *chkAutoRun;
    QPushButton *cmdBrowseFile;
    QPushButton *cmdHIDController;
    QFrame *line;
    QLineEdit *txtFileselected;
    QProgressBar *pbFileupload;
    QLabel *lblProgress;
    QFrame *line_2;
    QTextEdit *txtFTPLog;
    QPushButton *cmdAbort;
    QLabel *label_4;

    void setupUi(QMainWindow *FrmFTP)
    {
        if (FrmFTP->objectName().isEmpty())
            FrmFTP->setObjectName("FrmFTP");
        FrmFTP->resize(657, 489);
        FrmFTP->setMinimumSize(QSize(657, 489));
        FrmFTP->setMaximumSize(QSize(657, 489));
        QPalette palette;
        QBrush brush(QColor(20, 22, 24, 255));
        brush.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Base, brush);
        QBrush brush1(QColor(88, 88, 88, 255));
        brush1.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Window, brush1);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Base, brush);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Window, brush1);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Base, brush1);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Window, brush1);
        FrmFTP->setPalette(palette);
        QIcon icon(QIcon::fromTheme(QIcon::ThemeIcon::FolderDragAccept));
        FrmFTP->setWindowIcon(icon);
        centralwidget = new QWidget(FrmFTP);
        centralwidget->setObjectName("centralwidget");
        frame = new QFrame(centralwidget);
        frame->setObjectName("frame");
        frame->setGeometry(QRect(10, 10, 636, 76));
        frame->setStyleSheet(QString::fromUtf8("border: 1px solid #224075;\n"
""));
        frame->setFrameShape(QFrame::Shape::StyledPanel);
        frame->setFrameShadow(QFrame::Shadow::Raised);
        label_2 = new QLabel(frame);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(20, 18, 101, 18));
        label_2->setStyleSheet(QString::fromUtf8("border:none"));
        txtStartAddress = new QLineEdit(frame);
        txtStartAddress->setObjectName("txtStartAddress");
        txtStartAddress->setGeometry(QRect(40, 40, 80, 26));
        QFont font;
        font.setFamilies({QString::fromUtf8("Consolas")});
        txtStartAddress->setFont(font);
        opt1Ram = new QRadioButton(frame);
        opt1Ram->setObjectName("opt1Ram");
        opt1Ram->setGeometry(QRect(130, 10, 71, 23));
        opt1Ram->setStyleSheet(QString::fromUtf8("border:none"));
        opt1Ram->setCheckable(true);
        opt1Ram->setChecked(true);
        opt1File = new QRadioButton(frame);
        opt1File->setObjectName("opt1File");
        opt1File->setGeometry(QRect(210, 10, 71, 23));
        opt1File->setStyleSheet(QString::fromUtf8("border:none"));
        opt1FileOverwrite = new QRadioButton(frame);
        opt1FileOverwrite->setObjectName("opt1FileOverwrite");
        opt1FileOverwrite->setGeometry(QRect(290, 10, 121, 23));
        opt1FileOverwrite->setStyleSheet(QString::fromUtf8("border:none"));
        cmdCloseFTP = new QPushButton(centralwidget);
        cmdCloseFTP->setObjectName("cmdCloseFTP");
        cmdCloseFTP->setGeometry(QRect(560, 450, 86, 26));
        QPalette palette1;
        QBrush brush2(QColor(117, 0, 0, 255));
        brush2.setStyle(Qt::BrushStyle::SolidPattern);
        palette1.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush2);
        QBrush brush3(QColor(215, 235, 255, 255));
        brush3.setStyle(Qt::BrushStyle::SolidPattern);
        palette1.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Light, brush3);
        QBrush brush4(QColor(236, 56, 56, 255));
        brush4.setStyle(Qt::BrushStyle::SolidPattern);
        palette1.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Midlight, brush4);
        QBrush brush5(QColor(255, 0, 4, 255));
        brush5.setStyle(Qt::BrushStyle::SolidPattern);
        palette1.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Highlight, brush5);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        palette1.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Accent, brush5);
#endif
        palette1.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush2);
        palette1.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Light, brush3);
        palette1.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Midlight, brush4);
        palette1.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Highlight, brush5);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        palette1.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Accent, brush5);
#endif
        palette1.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush2);
        palette1.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Light, brush3);
        palette1.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Midlight, brush4);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        palette1.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Accent, brush5);
#endif
        cmdCloseFTP->setPalette(palette1);
        chkClearLogAuto = new QCheckBox(centralwidget);
        chkClearLogAuto->setObjectName("chkClearLogAuto");
        chkClearLogAuto->setGeometry(QRect(10, 450, 171, 23));
        cmdClearLog = new QPushButton(centralwidget);
        cmdClearLog->setObjectName("cmdClearLog");
        cmdClearLog->setGeometry(QRect(260, 450, 101, 26));
        label = new QLabel(centralwidget);
        label->setObjectName("label");
        label->setGeometry(QRect(21, 2, 105, 18));
        label->setAutoFillBackground(true);
        frame_2 = new QFrame(centralwidget);
        frame_2->setObjectName("frame_2");
        frame_2->setGeometry(QRect(10, 94, 636, 41));
        frame_2->setStyleSheet(QString::fromUtf8("border: 1px solid #224075;\n"
""));
        frame_2->setFrameShape(QFrame::Shape::StyledPanel);
        frame_2->setFrameShadow(QFrame::Shadow::Raised);
        opt2Chunk1 = new QRadioButton(frame_2);
        opt2Chunk1->setObjectName("opt2Chunk1");
        opt2Chunk1->setGeometry(QRect(10, 10, 100, 23));
        opt2Chunk1->setStyleSheet(QString::fromUtf8("border:none"));
        opt2Chunk1->setCheckable(true);
        opt2Chunk1->setChecked(true);
        opt2Chunk2 = new QRadioButton(frame_2);
        opt2Chunk2->setObjectName("opt2Chunk2");
        opt2Chunk2->setGeometry(QRect(100, 10, 100, 23));
        opt2Chunk2->setStyleSheet(QString::fromUtf8("border:none"));
        opt2Chunk3 = new QRadioButton(frame_2);
        opt2Chunk3->setObjectName("opt2Chunk3");
        opt2Chunk3->setGeometry(QRect(190, 10, 100, 23));
        opt2Chunk3->setStyleSheet(QString::fromUtf8("border:none"));
        opt2Chunk4 = new QRadioButton(frame_2);
        opt2Chunk4->setObjectName("opt2Chunk4");
        opt2Chunk4->setGeometry(QRect(270, 10, 100, 23));
        opt2Chunk4->setStyleSheet(QString::fromUtf8("border:none"));
        opt2Chunk5 = new QRadioButton(frame_2);
        opt2Chunk5->setObjectName("opt2Chunk5");
        opt2Chunk5->setGeometry(QRect(350, 10, 100, 23));
        opt2Chunk5->setStyleSheet(QString::fromUtf8("border:none"));
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(20, 86, 74, 18));
        label_3->setAutoFillBackground(true);
        cmdClearRam = new QPushButton(centralwidget);
        cmdClearRam->setObjectName("cmdClearRam");
        cmdClearRam->setGeometry(QRect(140, 50, 80, 26));
        cmdSendFile = new QPushButton(centralwidget);
        cmdSendFile->setObjectName("cmdSendFile");
        cmdSendFile->setGeometry(QRect(224, 50, 71, 26));
        cmdRunInRam = new QPushButton(centralwidget);
        cmdRunInRam->setObjectName("cmdRunInRam");
        cmdRunInRam->setGeometry(QRect(510, 50, 50, 26));
        cmdViewRam = new QPushButton(centralwidget);
        cmdViewRam->setObjectName("cmdViewRam");
        cmdViewRam->setGeometry(QRect(570, 50, 60, 26));
        chkAutoRun = new QCheckBox(centralwidget);
        chkAutoRun->setObjectName("chkAutoRun");
        chkAutoRun->setGeometry(QRect(420, 52, 84, 23));
        cmdBrowseFile = new QPushButton(centralwidget);
        cmdBrowseFile->setObjectName("cmdBrowseFile");
        cmdBrowseFile->setGeometry(QRect(560, 150, 86, 26));
        cmdHIDController = new QPushButton(centralwidget);
        cmdHIDController->setObjectName("cmdHIDController");
        cmdHIDController->setGeometry(QRect(440, 450, 111, 26));
        line = new QFrame(centralwidget);
        line->setObjectName("line");
        line->setGeometry(QRect(-30, 140, 750, 3));
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);
        txtFileselected = new QLineEdit(centralwidget);
        txtFileselected->setObjectName("txtFileselected");
        txtFileselected->setGeometry(QRect(10, 150, 540, 26));
        txtFileselected->setFont(font);
        txtFileselected->setStyleSheet(QString::fromUtf8("border: 1px solid #224075;\n"
""));
        pbFileupload = new QProgressBar(centralwidget);
        pbFileupload->setObjectName("pbFileupload");
        pbFileupload->setGeometry(QRect(10, 181, 480, 24));
        pbFileupload->setStyleSheet(QString::fromUtf8("QProgressBar {\n"
"    border: 1px solid #3c6dc3;    /* blue outer border */\n"
"    border-radius: 4px;\n"
"    text-align: center;\n"
"    background-color: #2b2b2b;    /* dark empty bar */\n"
"    color: #ffffff;                /* text color */\n"
"}\n"
"\n"
"QProgressBar::chunk {\n"
"    background-color: #3c6dc3;    /* blue fill */\n"
"    border: 1px solid #2b2b2b;    /* match empty bar for nice border effect */\n"
"    border-radius: 3px;            /* slightly smaller than bar radius */\n"
"    margin: 0px;\n"
"}\n"
""));
        pbFileupload->setValue(25);
        pbFileupload->setOrientation(Qt::Orientation::Horizontal);
        pbFileupload->setInvertedAppearance(false);
        lblProgress = new QLabel(centralwidget);
        lblProgress->setObjectName("lblProgress");
        lblProgress->setGeometry(QRect(502, 183, 140, 18));
        line_2 = new QFrame(centralwidget);
        line_2->setObjectName("line_2");
        line_2->setGeometry(QRect(-30, 210, 750, 3));
        line_2->setFrameShape(QFrame::Shape::HLine);
        line_2->setFrameShadow(QFrame::Shadow::Sunken);
        txtFTPLog = new QTextEdit(centralwidget);
        txtFTPLog->setObjectName("txtFTPLog");
        txtFTPLog->setGeometry(QRect(10, 220, 637, 221));
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(txtFTPLog->sizePolicy().hasHeightForWidth());
        txtFTPLog->setSizePolicy(sizePolicy);
        txtFTPLog->setMaximumSize(QSize(16777215, 16777215));
        QPalette palette2;
        QBrush brush6(QColor(0, 255, 255, 255));
        brush6.setStyle(Qt::BrushStyle::SolidPattern);
        palette2.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::WindowText, brush6);
        palette2.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Text, brush6);
        QBrush brush7(QColor(26, 31, 63, 255));
        brush7.setStyle(Qt::BrushStyle::SolidPattern);
        palette2.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Base, brush7);
        palette2.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::WindowText, brush6);
        palette2.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Text, brush6);
        palette2.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Base, brush7);
        txtFTPLog->setPalette(palette2);
        QFont font1;
        font1.setFamilies({QString::fromUtf8("Consolas")});
        font1.setPointSize(10);
        txtFTPLog->setFont(font1);
        txtFTPLog->setFrameShape(QFrame::Shape::WinPanel);
        txtFTPLog->setReadOnly(true);
        cmdAbort = new QPushButton(centralwidget);
        cmdAbort->setObjectName("cmdAbort");
        cmdAbort->setGeometry(QRect(300, 50, 71, 26));
        label_4 = new QLabel(centralwidget);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(30, 54, 58, 18));
        FrmFTP->setCentralWidget(centralwidget);

        retranslateUi(FrmFTP);

        QMetaObject::connectSlotsByName(FrmFTP);
    } // setupUi

    void retranslateUi(QMainWindow *FrmFTP)
    {
        FrmFTP->setWindowTitle(QCoreApplication::translate("FrmFTP", "MainWindow", nullptr));
        label_2->setText(QCoreApplication::translate("FrmFTP", "ram offset:", nullptr));
        txtStartAddress->setText(QCoreApplication::translate("FrmFTP", "0000", nullptr));
        opt1Ram->setText(QCoreApplication::translate("FrmFTP", "To RAM", nullptr));
        opt1File->setText(QCoreApplication::translate("FrmFTP", "To FILE", nullptr));
        opt1FileOverwrite->setText(QCoreApplication::translate("FrmFTP", "File Overwrite", nullptr));
        cmdCloseFTP->setText(QCoreApplication::translate("FrmFTP", "[ close ]", nullptr));
        chkClearLogAuto->setText(QCoreApplication::translate("FrmFTP", "Clear on new send.", nullptr));
        cmdClearLog->setText(QCoreApplication::translate("FrmFTP", "clear log", nullptr));
        label->setText(QCoreApplication::translate("FrmFTP", " Target Location:", nullptr));
        opt2Chunk1->setText(QCoreApplication::translate("FrmFTP", "256 bytes", nullptr));
        opt2Chunk2->setText(QCoreApplication::translate("FrmFTP", "512 bytes", nullptr));
        opt2Chunk3->setText(QCoreApplication::translate("FrmFTP", "1k bytes", nullptr));
        opt2Chunk4->setText(QCoreApplication::translate("FrmFTP", "2k bytes", nullptr));
        opt2Chunk5->setText(QCoreApplication::translate("FrmFTP", "4k bytes", nullptr));
        label_3->setText(QCoreApplication::translate("FrmFTP", " Chunk size ", nullptr));
        cmdClearRam->setText(QCoreApplication::translate("FrmFTP", "Clear ram >", nullptr));
        cmdSendFile->setText(QCoreApplication::translate("FrmFTP", "Send >", nullptr));
        cmdRunInRam->setText(QCoreApplication::translate("FrmFTP", "Run !", nullptr));
        cmdViewRam->setText(QCoreApplication::translate("FrmFTP", "view", nullptr));
        chkAutoRun->setText(QCoreApplication::translate("FrmFTP", "auto run", nullptr));
        cmdBrowseFile->setText(QCoreApplication::translate("FrmFTP", "Browse...", nullptr));
        cmdHIDController->setText(QCoreApplication::translate("FrmFTP", "HID Ctrl...", nullptr));
        pbFileupload->setFormat(QCoreApplication::translate("FrmFTP", "%p%", nullptr));
        lblProgress->setText(QCoreApplication::translate("FrmFTP", "00:00 (0.0 KB/s)", nullptr));
        txtFTPLog->setHtml(QCoreApplication::translate("FrmFTP", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"hr { height: 1px; border-width: 0; }\n"
"li.unchecked::marker { content: \"\\2610\"; }\n"
"li.checked::marker { content: \"\\2612\"; }\n"
"</style></head><body style=\" font-family:'Consolas'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-family:'Liberation Mono';\"><br /></p></body></html>", nullptr));
        cmdAbort->setText(QCoreApplication::translate("FrmFTP", "abort!", nullptr));
        label_4->setText(QCoreApplication::translate("FrmFTP", "0x", nullptr));
    } // retranslateUi

};

namespace Ui {
    class FrmFTP: public Ui_FrmFTP {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FRMFTP_H
