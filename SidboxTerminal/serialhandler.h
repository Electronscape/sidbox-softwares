#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H

// serialhandler.h
#pragma once

#include <QObject>
#include <QtSerialPort/QtSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTextEdit>


class SerialHandler : public QObject
{
    Q_OBJECT

public:
    explicit SerialHandler(QTextEdit* outputBox, QObject* parent = nullptr);

    void listPorts();             // List available ports
    bool openPort(const QString& portName, int baud = QSerialPort::Baud115200);
    void closePort();
    void writeData(const QString& string);
    QSerialPort* getSerial() const { return serial; }

    void readData();

private slots:


private:
    QSerialPort* serial;
    QTextEdit* textBox;


signals:
    void rawSerial(const QString &text);
    void requestOpenSerialPort();

};


#endif // SERIALHANDLER_H
