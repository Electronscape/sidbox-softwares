// serialhandler.cpp
#include "serialhandler.h"

#include "directoryboss.h"

#include <QColor>
#include <QCoreApplication>
#include <QSettings>
#include <QFont>
#include <QTextCursor>
#include <QTextFormat>

SerialHandler::SerialHandler(QTextEdit* outputBox, QObject* parent)
    : QObject(parent), textBox(outputBox)
{
    serial = new QSerialPort(this);
    loadAnsiPalette();
    resetAnsiFormat();

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

    loadAnsiPalette();
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

static const char *ansiPaletteKeys[16] = {
    "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white",
    "bright_black", "bright_red", "bright_green", "bright_yellow",
    "bright_blue", "bright_magenta", "bright_cyan", "bright_white"
};

static const QColor ansiDefaultPalette[16] = {
    QColor(0, 0, 0), QColor(170, 0, 0), QColor(0, 170, 0), QColor(170, 85, 0),
    QColor(0, 0, 170), QColor(170, 0, 170), QColor(0, 170, 170), QColor(210, 210, 210),
    QColor(85, 85, 85), QColor(255, 85, 85), QColor(85, 255, 85), QColor(255, 255, 85),
    QColor(85, 85, 255), QColor(255, 85, 255), QColor(85, 255, 255), QColor(255, 255, 255)
};

void SerialHandler::loadAnsiPalette()
{
    for (int i = 0; i < 16; i++)
        ansiPalette[i] = ansiDefaultPalette[i];

    QSettings settings(QCoreApplication::applicationDirPath() + "/settings.ini", QSettings::IniFormat);
    settings.beginGroup("terminal_palette");

    for (int i = 0; i < 16; i++) {
        const QString key = QString::fromLatin1(ansiPaletteKeys[i]);
        if (!settings.contains(key)) {
            settings.setValue(key, ansiPalette[i].name(QColor::HexRgb));
            continue;
        }

        const QColor color(settings.value(key).toString());
        if (color.isValid())
            ansiPalette[i] = color;
    }

    settings.endGroup();
    settings.sync();
}

QColor SerialHandler::ansiColor(int code, bool bright) const
{
    if (code < 0 || code > 7)
        return QColor();

    return ansiPalette[code + (bright ? 8 : 0)];
}

static QColor ansi256Color(int index, const std::array<QColor, 16> &palette)
{
    if (index < 0 || index > 255)
        return QColor();

    if (index < 16)
        return palette[index];

    if (index < 232) {
        int v = index - 16;
        int r = v / 36;
        int g = (v / 6) % 6;
        int b = v % 6;
        auto level = [](int c) { return c == 0 ? 0 : 55 + (c * 40); };
        return QColor(level(r), level(g), level(b));
    }

    int gray = 8 + ((index - 232) * 10);
    return QColor(gray, gray, gray);
}

void SerialHandler::resetAnsiFormat()
{
    ansiFormat = QTextCharFormat();
}

void SerialHandler::applyAnsiSgr(const QByteArray &params)
{
    QList<int> codes;
    if (params.isEmpty()) {
        codes.append(0);
    } else {
        const QList<QByteArray> parts = params.split(';');
        for (const QByteArray &part : parts) {
            bool ok = false;
            int value = part.isEmpty() ? 0 : part.toInt(&ok);
            codes.append(ok || part.isEmpty() ? value : 0);
        }
    }

    for (int i = 0; i < codes.size(); i++) {
        const int code = codes[i];

        if (code == 0) {
            resetAnsiFormat();
        } else if (code == 1) {
            ansiFormat.setFontWeight(QFont::Bold);
        } else if (code == 22) {
            ansiFormat.setFontWeight(QFont::Normal);
        } else if (code == 4) {
            ansiFormat.setFontUnderline(true);
        } else if (code == 24) {
            ansiFormat.setFontUnderline(false);
        } else if (code == 39) {
            ansiFormat.clearProperty(QTextFormat::ForegroundBrush);
        } else if (code == 49) {
            ansiFormat.clearProperty(QTextFormat::BackgroundBrush);
        } else if (code >= 30 && code <= 37) {
            ansiFormat.setForeground(ansiColor(code - 30, false));
        } else if (code >= 90 && code <= 97) {
            ansiFormat.setForeground(ansiColor(code - 90, true));
        } else if (code >= 40 && code <= 47) {
            ansiFormat.setBackground(ansiColor(code - 40, false));
        } else if (code >= 100 && code <= 107) {
            ansiFormat.setBackground(ansiColor(code - 100, true));
        } else if ((code == 38 || code == 48) && i + 2 < codes.size()) {
            const bool foreground = (code == 38);
            const int mode = codes[++i];

            if (mode == 5 && i + 1 < codes.size()) {
                QColor color = ansi256Color(codes[++i], ansiPalette);
                if (color.isValid()) {
                    if (foreground) ansiFormat.setForeground(color);
                    else            ansiFormat.setBackground(color);
                }
            } else if (mode == 2 && i + 3 < codes.size()) {
                int r = codes[++i];
                int g = codes[++i];
                int b = codes[++i];
                QColor color(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255));
                if (foreground) ansiFormat.setForeground(color);
                else            ansiFormat.setBackground(color);
            }
        }
    }
}

void SerialHandler::appendAnsiData(const QByteArray &data, QString *plainText)
{
    QByteArray bytes = ansiCarry + data;
    ansiCarry.clear();

    QTextCursor cursor = textBox->textCursor();
    cursor.movePosition(QTextCursor::End);
    textBox->setTextCursor(cursor);

    int segmentStart = 0;
    int i = 0;

    auto appendSegment = [&](int end) {
        if (end <= segmentStart)
            return;

        QByteArray segment = bytes.mid(segmentStart, end - segmentStart);
        QString text = QString::fromUtf8(segment.constData(), segment.size());
        cursor.insertText(text, ansiFormat);
        if (plainText)
            plainText->append(text);
    };

    while (i < bytes.size()) {
        if (static_cast<unsigned char>(bytes[i]) != 0x1b) {
            i++;
            continue;
        }

        appendSegment(i);

        if (i + 1 >= bytes.size()) {
            ansiCarry = bytes.mid(i);
            segmentStart = bytes.size();
            break;
        }

        if (bytes[i + 1] == '[') {
            int j = i + 2;
            while (j < bytes.size()) {
                unsigned char c = static_cast<unsigned char>(bytes[j]);
                if (c >= 0x40 && c <= 0x7e)
                    break;
                j++;
            }

            if (j >= bytes.size()) {
                ansiCarry = bytes.mid(i);
                segmentStart = bytes.size();
                break;
            }

            if (bytes[j] == 'm')
                applyAnsiSgr(bytes.mid(i + 2, j - (i + 2)));

            i = j + 1;
            segmentStart = i;
            continue;
        }

        i += 2;
        segmentStart = i;
    }

    appendSegment(bytes.size());
    textBox->setTextCursor(cursor);
}

void SerialHandler::readData()
{
    QByteArray data = serial->readAll();
    if (data.isEmpty()) return;

    QString plainText;
    appendAnsiData(data, &plainText);

    if (!plainText.isEmpty())
        emit rawSerial(plainText);      // send clean text to parser windows

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
