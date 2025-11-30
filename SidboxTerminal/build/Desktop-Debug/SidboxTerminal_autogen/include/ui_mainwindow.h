/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionAbout;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QTextEdit *textBox;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *cmdOpenPort;
    QPushButton *cmdClosePort;
    QWidget *widget;
    QPushButton *cmdSendCustomCMDDir;
    QPushButton *cmdSetTimeDate;
    QPushButton *cmdOpenFTP;
    QPushButton *cmdOpenDirectory;
    QPushButton *cmdViewRam;
    QLineEdit *txtViewFrom;
    QPushButton *cmdClearLog;
    QHBoxLayout *horizontalLayout;
    QLineEdit *txtCommandLine;
    QPushButton *cmdClear;
    QPushButton *cmdSend;
    QMenuBar *menubar;
    QMenu *menuTerminal;
    QMenu *menuHelp;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->setWindowModality(Qt::WindowModality::NonModal);
        MainWindow->resize(977, 537);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        MainWindow->setMinimumSize(QSize(780, 433));
        MainWindow->setMaximumSize(QSize(16777215, 16777215));
        MainWindow->setBaseSize(QSize(780, 800));
        MainWindow->setFocusPolicy(Qt::FocusPolicy::ClickFocus);
        MainWindow->setAcceptDrops(false);
        QIcon icon;
        icon.addFile(QString::fromUtf8("../system-icons/guildwars2.ico"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        MainWindow->setWindowIcon(icon);
        MainWindow->setTabShape(QTabWidget::TabShape::Rounded);
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName("actionAbout");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        centralwidget->setMaximumSize(QSize(16777215, 16777215));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(-1, 12, -1, -1);
        textBox = new QTextEdit(centralwidget);
        textBox->setObjectName("textBox");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(textBox->sizePolicy().hasHeightForWidth());
        textBox->setSizePolicy(sizePolicy1);
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

        verticalLayout->addWidget(textBox);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(2);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalLayout_2->setContentsMargins(0, 0, -1, -1);
        cmdOpenPort = new QPushButton(centralwidget);
        cmdOpenPort->setObjectName("cmdOpenPort");
        cmdOpenPort->setMinimumSize(QSize(0, 22));
        cmdOpenPort->setMaximumSize(QSize(80, 16777215));

        horizontalLayout_2->addWidget(cmdOpenPort);

        cmdClosePort = new QPushButton(centralwidget);
        cmdClosePort->setObjectName("cmdClosePort");
        cmdClosePort->setMinimumSize(QSize(0, 22));
        cmdClosePort->setMaximumSize(QSize(80, 16777215));

        horizontalLayout_2->addWidget(cmdClosePort);

        widget = new QWidget(centralwidget);
        widget->setObjectName("widget");

        horizontalLayout_2->addWidget(widget);

        cmdSendCustomCMDDir = new QPushButton(centralwidget);
        cmdSendCustomCMDDir->setObjectName("cmdSendCustomCMDDir");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(cmdSendCustomCMDDir->sizePolicy().hasHeightForWidth());
        cmdSendCustomCMDDir->setSizePolicy(sizePolicy2);
        cmdSendCustomCMDDir->setMinimumSize(QSize(64, 22));
        cmdSendCustomCMDDir->setMaximumSize(QSize(64, 16777215));
        cmdSendCustomCMDDir->setBaseSize(QSize(0, 0));
        cmdSendCustomCMDDir->setFlat(false);

        horizontalLayout_2->addWidget(cmdSendCustomCMDDir);

        cmdSetTimeDate = new QPushButton(centralwidget);
        cmdSetTimeDate->setObjectName("cmdSetTimeDate");
        cmdSetTimeDate->setMinimumSize(QSize(90, 22));
        cmdSetTimeDate->setMaximumSize(QSize(90, 16777215));
        cmdSetTimeDate->setFlat(false);

        horizontalLayout_2->addWidget(cmdSetTimeDate);

        cmdOpenFTP = new QPushButton(centralwidget);
        cmdOpenFTP->setObjectName("cmdOpenFTP");
        cmdOpenFTP->setMinimumSize(QSize(50, 0));
        cmdOpenFTP->setMaximumSize(QSize(50, 16777215));

        horizontalLayout_2->addWidget(cmdOpenFTP);

        cmdOpenDirectory = new QPushButton(centralwidget);
        cmdOpenDirectory->setObjectName("cmdOpenDirectory");
        cmdOpenDirectory->setMinimumSize(QSize(110, 0));
        cmdOpenDirectory->setMaximumSize(QSize(110, 16777215));

        horizontalLayout_2->addWidget(cmdOpenDirectory);

        cmdViewRam = new QPushButton(centralwidget);
        cmdViewRam->setObjectName("cmdViewRam");
        cmdViewRam->setMinimumSize(QSize(100, 22));
        cmdViewRam->setMaximumSize(QSize(100, 16777215));
        cmdViewRam->setFlat(false);

        horizontalLayout_2->addWidget(cmdViewRam);

        txtViewFrom = new QLineEdit(centralwidget);
        txtViewFrom->setObjectName("txtViewFrom");
        txtViewFrom->setMinimumSize(QSize(100, 0));
        txtViewFrom->setMaximumSize(QSize(100, 16777215));
        QFont font1;
        font1.setFamilies({QString::fromUtf8("Noto Sans Mono")});
        txtViewFrom->setFont(font1);

        horizontalLayout_2->addWidget(txtViewFrom);

        cmdClearLog = new QPushButton(centralwidget);
        cmdClearLog->setObjectName("cmdClearLog");
        cmdClearLog->setMinimumSize(QSize(60, 22));
        cmdClearLog->setMaximumSize(QSize(60, 16384));
        cmdClearLog->setFlat(false);

        horizontalLayout_2->addWidget(cmdClearLog);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(3);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setSizeConstraint(QLayout::SizeConstraint::SetMaximumSize);
        horizontalLayout->setContentsMargins(0, -1, -1, -1);
        txtCommandLine = new QLineEdit(centralwidget);
        txtCommandLine->setObjectName("txtCommandLine");
        QSizePolicy sizePolicy3(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(txtCommandLine->sizePolicy().hasHeightForWidth());
        txtCommandLine->setSizePolicy(sizePolicy3);
        txtCommandLine->setMaxLength(2048);

        horizontalLayout->addWidget(txtCommandLine);

        cmdClear = new QPushButton(centralwidget);
        cmdClear->setObjectName("cmdClear");
        QSizePolicy sizePolicy4(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(cmdClear->sizePolicy().hasHeightForWidth());
        cmdClear->setSizePolicy(sizePolicy4);
        cmdClear->setMinimumSize(QSize(80, 0));
        cmdClear->setMaximumSize(QSize(80, 16777215));

        horizontalLayout->addWidget(cmdClear);

        cmdSend = new QPushButton(centralwidget);
        cmdSend->setObjectName("cmdSend");
        sizePolicy2.setHeightForWidth(cmdSend->sizePolicy().hasHeightForWidth());
        cmdSend->setSizePolicy(sizePolicy2);
        cmdSend->setMinimumSize(QSize(80, 0));
        cmdSend->setMaximumSize(QSize(80, 16777215));
        cmdSend->setAutoDefault(false);
        cmdSend->setFlat(false);

        horizontalLayout->addWidget(cmdSend);


        verticalLayout->addLayout(horizontalLayout);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 977, 23));
        menuTerminal = new QMenu(menubar);
        menuTerminal->setObjectName("menuTerminal");
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName("menuHelp");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuTerminal->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuHelp->addSeparator();
        menuHelp->addAction(actionAbout);

        retranslateUi(MainWindow);

        cmdSend->setDefault(true);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Sidbox Terminal", nullptr));
        actionAbout->setText(QCoreApplication::translate("MainWindow", "About", nullptr));
        textBox->setHtml(QCoreApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"hr { height: 1px; border-width: 0; }\n"
"li.unchecked::marker { content: \"\\2610\"; }\n"
"li.checked::marker { content: \"\\2612\"; }\n"
"</style></head><body style=\" font-family:'Liberation Mono'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><br /></p></body></html>", nullptr));
        cmdOpenPort->setText(QCoreApplication::translate("MainWindow", "Open Port", nullptr));
        cmdClosePort->setText(QCoreApplication::translate("MainWindow", "Close Port", nullptr));
        cmdSendCustomCMDDir->setText(QCoreApplication::translate("MainWindow", "[ dir.. ]", nullptr));
        cmdSetTimeDate->setText(QCoreApplication::translate("MainWindow", "[ set time ]", nullptr));
        cmdOpenFTP->setText(QCoreApplication::translate("MainWindow", "FTP...", nullptr));
        cmdOpenDirectory->setText(QCoreApplication::translate("MainWindow", "SD-Directory...", nullptr));
        cmdViewRam->setText(QCoreApplication::translate("MainWindow", "[ view ram ]", nullptr));
        txtViewFrom->setText(QCoreApplication::translate("MainWindow", "0x4000", nullptr));
        cmdClearLog->setText(QCoreApplication::translate("MainWindow", "clear", nullptr));
        txtCommandLine->setPlaceholderText(QString());
        cmdClear->setText(QCoreApplication::translate("MainWindow", "clear", nullptr));
        cmdSend->setText(QCoreApplication::translate("MainWindow", "send", nullptr));
        menuTerminal->setTitle(QCoreApplication::translate("MainWindow", "Terminal", nullptr));
        menuHelp->setTitle(QCoreApplication::translate("MainWindow", "Help", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
