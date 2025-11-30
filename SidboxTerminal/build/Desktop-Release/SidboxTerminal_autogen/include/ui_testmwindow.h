/********************************************************************************
** Form generated from reading UI file 'testmwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TESTMWINDOW_H
#define UI_TESTMWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TestMWindow
{
public:
    QWidget *centralwidget;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *TestMWindow)
    {
        if (TestMWindow->objectName().isEmpty())
            TestMWindow->setObjectName("TestMWindow");
        TestMWindow->resize(800, 600);
        centralwidget = new QWidget(TestMWindow);
        centralwidget->setObjectName("centralwidget");
        TestMWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(TestMWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 23));
        TestMWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(TestMWindow);
        statusbar->setObjectName("statusbar");
        TestMWindow->setStatusBar(statusbar);

        retranslateUi(TestMWindow);

        QMetaObject::connectSlotsByName(TestMWindow);
    } // setupUi

    void retranslateUi(QMainWindow *TestMWindow)
    {
        TestMWindow->setWindowTitle(QCoreApplication::translate("TestMWindow", "MainWindow", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TestMWindow: public Ui_TestMWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TESTMWINDOW_H
