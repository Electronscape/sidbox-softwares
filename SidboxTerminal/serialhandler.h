#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H

// serialhandler.h
#pragma once

#include <QObject>
#include <QtSerialPort/QtSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QByteArray>
#include <QColor>
#include <QTextEdit>
#include <QTextCharFormat>
#include <array>


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
    QTextCharFormat ansiFormat;
    QByteArray ansiCarry;
    std::array<QColor, 16> ansiPalette;

    void loadAnsiPalette();
    QColor ansiColor(int code, bool bright) const;
    void resetAnsiFormat();
    void appendAnsiData(const QByteArray &data, QString *plainText);
    void applyAnsiSgr(const QByteArray &params);


signals:
    void rawSerial(const QString &text);
    void requestOpenSerialPort();

};


#endif // SERIALHANDLER_H
