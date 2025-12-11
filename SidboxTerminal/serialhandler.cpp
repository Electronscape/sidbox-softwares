// serialhandler.cpp
#include "serialhandler.h"

#include "directoryboss.h"

SerialHandler::SerialHandler(QTextEdit* outputBox, QObject* parent)
    : QObject(parent), textBox(outputBox)
{
    serial = new QSerialPort(this);

    // Connect readyRead signal to our slot
    connect(serial, &QSerialPort::readyRead, this, &SerialHandler::readData);
}

void SerialHandler::listPorts()
{
    const auto infos = QSerialPortInfo::availablePorts();
    textBox->append("Available Ports:");
    for (const QSerialPortInfo &info : infos) {
        textBox->append(info.portName() + " - " + info.description());
    }
}

bool SerialHandler::openPort(const QString& portName, int baud)
{
    if (serial->isOpen())
        serial->close();    // close it, then re-open it

    serial->setPortName(portName);
    serial->setBaudRate(baud);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!serial->open(QIODevice::ReadWrite)) {
        textBox->append("Failed to open port: " + serial->errorString());

        return false;
    }

    textBox->append("Port opened: " + portName + "\r\nReady.\r\n");
    return true;
}



void SerialHandler::closePort()
{
    if (serial->isOpen()) {
        serial->close();
        textBox->append("Port closed.");
    }
}

void SerialHandler::readData()
{
    QByteArray data = serial->readAll();
    if (data.isEmpty()) return;

    QString text = QString::fromUtf8(data);

    emit rawSerial(text);      // <---- send to all windows


    // Force cursor to end
    QTextCursor cursor = textBox->textCursor();
    cursor.movePosition(QTextCursor::End);
    textBox->setTextCursor(cursor);

    // Append the new data
    textBox->insertPlainText(text);

    // Ensure it's visible (scrolls to bottom)
    textBox->ensureCursorVisible();
}

void SerialHandler::writeData(const QString& string)
{
    if (!serial || !serial->isOpen()) {
        qWarning() << "Serial port not open – attempting to open serial port";
        emit requestOpenSerialPort();
    }

    // Convert the QString to UTF-8 bytes (most serial devices expect plain text)
    QByteArray data = string.toUtf8();

    // Optional: append a line-ending if your device expects it
    // data.append("\r\n");   // for CRLF
    // data.append('\n');     // for LF only

    qint64 written = serial->write(data);
    if (written == -1) {
        qWarning() << "Failed to write to serial port:" << serial->errorString();
    } else if (written < data.size()) {
        qWarning() << "Only wrote" << written << "of" << data.size() << "bytes";
    } else {
        qDebug() << "Sent:" << string;
    }

    // Force the data out immediately (useful for interactive commands)
    serial->flush();
}
