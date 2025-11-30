/********************************************************************************
** Form generated from reading UI file 'frmftp.ui'
**
** Created by: Qt User Interface Compiler version 6.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FRMFTP_H
#define UI_FRMFTP_H

#include <QtCore/QVariant>
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
    QLineEdit *lineEdit;
    QRadioButton *radioButton;
    QRadioButton *radioButton_2;
    QRadioButton *radioButton_3;
    QPushButton *cmdCloseFTP;
    QCheckBox *checkBox;
    QPushButton *cmdCloseFTP_2;
    QLabel *label;
    QFrame *frame_2;
    QRadioButton *radioButton_4;
    QRadioButton *radioButton_5;
    QRadioButton *radioButton_6;
    QRadioButton *radioButton_7;
    QRadioButton *radioButton_8;
    QLabel *label_3;
    QPushButton *cmdCloseFTP_3;
    QPushButton *cmdCloseFTP_4;
    QPushButton *cmdCloseFTP_5;
    QPushButton *cmdCloseFTP_6;
    QCheckBox *checkBox_2;
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QPushButton *pushButton_3;
    QFrame *line;
    QLineEdit *lineEdit_2;
    QProgressBar *progressBar;
    QLabel *label_4;
    QFrame *line_2;
    QTextEdit *textBox;

    void setupUi(QMainWindow *FrmFTP)
    {
        if (FrmFTP->objectName().isEmpty())
            FrmFTP->setObjectName("FrmFTP");
        FrmFTP->resize(657, 374);
        centralwidget = new QWidget(FrmFTP);
        centralwidget->setObjectName("centralwidget");
        frame = new QFrame(centralwidget);
        frame->setObjectName("frame");
        frame->setGeometry(QRect(10, 10, 511, 76));
        frame->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        frame->setFrameShape(QFrame::Shape::StyledPanel);
        frame->setFrameShadow(QFrame::Shadow::Raised);
        label_2 = new QLabel(frame);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(20, 20, 101, 18));
        label_2->setStyleSheet(QString::fromUtf8("border:none"));
        lineEdit = new QLineEdit(frame);
        lineEdit->setObjectName("lineEdit");
        lineEdit->setGeometry(QRect(10, 40, 111, 26));
        radioButton = new QRadioButton(frame);
        radioButton->setObjectName("radioButton");
        radioButton->setGeometry(QRect(130, 10, 71, 23));
        radioButton->setStyleSheet(QString::fromUtf8("border:none"));
        radioButton->setCheckable(true);
        radioButton->setChecked(true);
        radioButton_2 = new QRadioButton(frame);
        radioButton_2->setObjectName("radioButton_2");
        radioButton_2->setGeometry(QRect(210, 10, 71, 23));
        radioButton_2->setStyleSheet(QString::fromUtf8("border:none"));
        radioButton_3 = new QRadioButton(frame);
        radioButton_3->setObjectName("radioButton_3");
        radioButton_3->setGeometry(QRect(290, 10, 121, 23));
        radioButton_3->setStyleSheet(QString::fromUtf8("border:none"));
        cmdCloseFTP = new QPushButton(centralwidget);
        cmdCloseFTP->setObjectName("cmdCloseFTP");
        cmdCloseFTP->setGeometry(QRect(430, 100, 86, 26));
        checkBox = new QCheckBox(centralwidget);
        checkBox->setObjectName("checkBox");
        checkBox->setGeometry(QRect(10, 340, 171, 23));
        cmdCloseFTP_2 = new QPushButton(centralwidget);
        cmdCloseFTP_2->setObjectName("cmdCloseFTP_2");
        cmdCloseFTP_2->setGeometry(QRect(260, 340, 101, 26));
        label = new QLabel(centralwidget);
        label->setObjectName("label");
        label->setGeometry(QRect(21, 2, 105, 18));
        label->setAutoFillBackground(true);
        frame_2 = new QFrame(centralwidget);
        frame_2->setObjectName("frame_2");
        frame_2->setGeometry(QRect(540, 10, 101, 121));
        frame_2->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        frame_2->setFrameShape(QFrame::Shape::StyledPanel);
        frame_2->setFrameShadow(QFrame::Shadow::Raised);
        radioButton_4 = new QRadioButton(frame_2);
        radioButton_4->setObjectName("radioButton_4");
        radioButton_4->setGeometry(QRect(10, 10, 100, 23));
        radioButton_4->setStyleSheet(QString::fromUtf8("border:none"));
        radioButton_4->setCheckable(true);
        radioButton_4->setChecked(true);
        radioButton_5 = new QRadioButton(frame_2);
        radioButton_5->setObjectName("radioButton_5");
        radioButton_5->setGeometry(QRect(10, 30, 100, 23));
        radioButton_5->setStyleSheet(QString::fromUtf8("border:none"));
        radioButton_6 = new QRadioButton(frame_2);
        radioButton_6->setObjectName("radioButton_6");
        radioButton_6->setGeometry(QRect(10, 50, 100, 23));
        radioButton_6->setStyleSheet(QString::fromUtf8("border:none"));
        radioButton_7 = new QRadioButton(frame_2);
        radioButton_7->setObjectName("radioButton_7");
        radioButton_7->setGeometry(QRect(10, 70, 100, 23));
        radioButton_7->setStyleSheet(QString::fromUtf8("border:none"));
        radioButton_8 = new QRadioButton(frame_2);
        radioButton_8->setObjectName("radioButton_8");
        radioButton_8->setGeometry(QRect(10, 90, 100, 23));
        radioButton_8->setStyleSheet(QString::fromUtf8("border:none"));
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(550, 2, 71, 18));
        label_3->setAutoFillBackground(true);
        cmdCloseFTP_3 = new QPushButton(centralwidget);
        cmdCloseFTP_3->setObjectName("cmdCloseFTP_3");
        cmdCloseFTP_3->setGeometry(QRect(140, 50, 80, 26));
        cmdCloseFTP_4 = new QPushButton(centralwidget);
        cmdCloseFTP_4->setObjectName("cmdCloseFTP_4");
        cmdCloseFTP_4->setGeometry(QRect(220, 50, 71, 26));
        cmdCloseFTP_5 = new QPushButton(centralwidget);
        cmdCloseFTP_5->setObjectName("cmdCloseFTP_5");
        cmdCloseFTP_5->setGeometry(QRect(390, 50, 50, 26));
        cmdCloseFTP_6 = new QPushButton(centralwidget);
        cmdCloseFTP_6->setObjectName("cmdCloseFTP_6");
        cmdCloseFTP_6->setGeometry(QRect(450, 50, 60, 26));
        checkBox_2 = new QCheckBox(centralwidget);
        checkBox_2->setObjectName("checkBox_2");
        checkBox_2->setGeometry(QRect(310, 52, 84, 23));
        pushButton = new QPushButton(centralwidget);
        pushButton->setObjectName("pushButton");
        pushButton->setGeometry(QRect(20, 100, 86, 26));
        pushButton_2 = new QPushButton(centralwidget);
        pushButton_2->setObjectName("pushButton_2");
        pushButton_2->setGeometry(QRect(220, 100, 71, 26));
        pushButton_3 = new QPushButton(centralwidget);
        pushButton_3->setObjectName("pushButton_3");
        pushButton_3->setGeometry(QRect(310, 100, 111, 26));
        line = new QFrame(centralwidget);
        line->setObjectName("line");
        line->setGeometry(QRect(-30, 140, 750, 3));
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);
        lineEdit_2 = new QLineEdit(centralwidget);
        lineEdit_2->setObjectName("lineEdit_2");
        lineEdit_2->setGeometry(QRect(10, 150, 637, 26));
        lineEdit_2->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
""));
        progressBar = new QProgressBar(centralwidget);
        progressBar->setObjectName("progressBar");
        progressBar->setGeometry(QRect(10, 180, 481, 23));
        progressBar->setStyleSheet(QString::fromUtf8("border: 1px solid #3c6dc3;\n"
"text-align: center;\n"
""));
        progressBar->setValue(24);
        label_4 = new QLabel(centralwidget);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(502, 183, 140, 18));
        line_2 = new QFrame(centralwidget);
        line_2->setObjectName("line_2");
        line_2->setGeometry(QRect(-30, 210, 750, 3));
        line_2->setFrameShape(QFrame::Shape::HLine);
        line_2->setFrameShadow(QFrame::Shadow::Sunken);
        textBox = new QTextEdit(centralwidget);
        textBox->setObjectName("textBox");
        textBox->setGeometry(QRect(10, 220, 637, 111));
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(textBox->sizePolicy().hasHeightForWidth());
        textBox->setSizePolicy(sizePolicy);
        textBox->setMaximumSize(QSize(16777215, 16777215));
        QPalette palette;
        QBrush brush(QColor(0, 255, 255, 255));
        brush.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::WindowText, brush);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Text, brush);
        QBrush brush1(QColor(26, 31, 63, 255));
        brush1.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Base, brush1);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::WindowText, brush);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Text, brush);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Base, brush1);
        textBox->setPalette(palette);
        QFont font;
        font.setFamilies({QString::fromUtf8("Liberation Mono")});
        font.setPointSize(10);
        textBox->setFont(font);
        textBox->setFrameShape(QFrame::Shape::WinPanel);
        textBox->setReadOnly(true);
        FrmFTP->setCentralWidget(centralwidget);

        retranslateUi(FrmFTP);

        QMetaObject::connectSlotsByName(FrmFTP);
    } // setupUi

    void retranslateUi(QMainWindow *FrmFTP)
    {
        FrmFTP->setWindowTitle(QCoreApplication::translate("FrmFTP", "MainWindow", nullptr));
        label_2->setText(QCoreApplication::translate("FrmFTP", "ram offset:", nullptr));
        lineEdit->setText(QCoreApplication::translate("FrmFTP", "$0000", nullptr));
        radioButton->setText(QCoreApplication::translate("FrmFTP", "To RAM", nullptr));
        radioButton_2->setText(QCoreApplication::translate("FrmFTP", "To FILE", nullptr));
        radioButton_3->setText(QCoreApplication::translate("FrmFTP", "File Overwrite", nullptr));
        cmdCloseFTP->setText(QCoreApplication::translate("FrmFTP", "close", nullptr));
        checkBox->setText(QCoreApplication::translate("FrmFTP", "Clear log on send event", nullptr));
        cmdCloseFTP_2->setText(QCoreApplication::translate("FrmFTP", "clear log", nullptr));
        label->setText(QCoreApplication::translate("FrmFTP", " Target Location:", nullptr));
        radioButton_4->setText(QCoreApplication::translate("FrmFTP", "256 bytes", nullptr));
        radioButton_5->setText(QCoreApplication::translate("FrmFTP", "512 bytes", nullptr));
        radioButton_6->setText(QCoreApplication::translate("FrmFTP", "1k bytes", nullptr));
        radioButton_7->setText(QCoreApplication::translate("FrmFTP", "2k bytes", nullptr));
        radioButton_8->setText(QCoreApplication::translate("FrmFTP", "4k bytes", nullptr));
        label_3->setText(QCoreApplication::translate("FrmFTP", "Chunk size", nullptr));
        cmdCloseFTP_3->setText(QCoreApplication::translate("FrmFTP", "Clear ram >", nullptr));
        cmdCloseFTP_4->setText(QCoreApplication::translate("FrmFTP", "Send >", nullptr));
        cmdCloseFTP_5->setText(QCoreApplication::translate("FrmFTP", "Run !", nullptr));
        cmdCloseFTP_6->setText(QCoreApplication::translate("FrmFTP", "view", nullptr));
        checkBox_2->setText(QCoreApplication::translate("FrmFTP", "auto run", nullptr));
        pushButton->setText(QCoreApplication::translate("FrmFTP", "Browse...", nullptr));
        pushButton_2->setText(QCoreApplication::translate("FrmFTP", "abort!", nullptr));
        pushButton_3->setText(QCoreApplication::translate("FrmFTP", "HID:interfacer", nullptr));
        label_4->setText(QCoreApplication::translate("FrmFTP", "00:00 (0.0 KB/s)", nullptr));
        textBox->setHtml(QCoreApplication::translate("FrmFTP", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"hr { height: 1px; border-width: 0; }\n"
"li.unchecked::marker { content: \"\\2610\"; }\n"
"li.checked::marker { content: \"\\2612\"; }\n"
"</style></head><body style=\" font-family:'Liberation Mono'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><br /></p></body></html>", nullptr));
    } // retranslateUi

};

namespace Ui {
    class FrmFTP: public Ui_FrmFTP {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FRMFTP_H
