#include "keyboardhidemu.h"
#include <QDir>
#include <QProcess>
#include <QDebug>
#include <QStringList>
#include <algorithm>

KeyboardHidEmu::KeyboardHidEmu(QObject *parent)
    : QObject(parent), fd(-1), notifier(nullptr), modifiers(0)
{
    pressedKeys.fill(0);
    initKeymap();
}

KeyboardHidEmu::~KeyboardHidEmu()
{
    if (notifier) notifier->deleteLater();
    if (fd >= 0) ::close(fd);
}

void KeyboardHidEmu::initKeymap()
{
    // Letters
    evdevToHid = {
        {KEY_A,0x04},{KEY_B,0x05},{KEY_C,0x06},{KEY_D,0x07},{KEY_E,0x08},
        {KEY_F,0x09},{KEY_G,0x0A},{KEY_H,0x0B},{KEY_I,0x0C},{KEY_J,0x0D},
        {KEY_K,0x0E},{KEY_L,0x0F},{KEY_M,0x10},{KEY_N,0x11},{KEY_O,0x12},
        {KEY_P,0x13},{KEY_Q,0x14},{KEY_R,0x15},{KEY_S,0x16},{KEY_T,0x17},
        {KEY_U,0x18},{KEY_V,0x19},{KEY_W,0x1A},{KEY_X,0x1B},{KEY_Y,0x1C},
        {KEY_Z,0x1D},
        // Numbers
        {KEY_1,0x1E},{KEY_2,0x1F},{KEY_3,0x20},{KEY_4,0x21},{KEY_5,0x22},
        {KEY_6,0x23},{KEY_7,0x24},{KEY_8,0x25},{KEY_9,0x26},{KEY_0,0x27},
        {KEY_ENTER,0x28},{KEY_ESC,0x29},{KEY_BACKSPACE,0x2A},{KEY_TAB,0x2B},
        {KEY_SPACE,0x2C},
        // Add more keys as needed
    };
}

// Auto-detect keyboard using udev property
QString KeyboardHidEmu::findKeyboardEventDevice()
{
    QDir devInput("/dev/input");
    QStringList entries = devInput.entryList(QStringList() << "event*", QDir::System);

    for (const QString &dev : entries) {
        QString path = "/dev/input/" + dev;

        QProcess p;
        p.start("udevadm", {"info","--query=property","--name",path});
        p.waitForFinished();
        QString out = p.readAllStandardOutput();

        if (out.contains("ID_INPUT_KEYBOARD=1"))
            return path;
    }
    return QString();
}


bool KeyboardHidEmu::start()
{
    QString dev = findKeyboardEventDevice();
    if (dev.isEmpty()) {
        qDebug() << "No keyboard found!";
        return false;
    }

    fd = ::open(dev.toStdString().c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        return false;
    }

    notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, [this](int){
        struct input_event ev;
        while (::read(fd, &ev, sizeof(ev)) == sizeof(ev))
            handleEvent(ev);
    });

    qDebug() << "Keyboard HID emulator started on" << dev;
    return true;
}

void KeyboardHidEmu::handleEvent(const struct input_event &ev)
{
    if (ev.type != EV_KEY) return;

    // Modifiers
    switch(ev.code) {
    case KEY_LEFTCTRL:  modifiers = ev.value ? modifiers|0x01 : modifiers&~0x01; break;
    case KEY_LEFTSHIFT: modifiers = ev.value ? modifiers|0x02 : modifiers&~0x02; break;
    case KEY_LEFTALT:   modifiers = ev.value ? modifiers|0x04 : modifiers&~0x04; break;
    case KEY_LEFTMETA:  modifiers = ev.value ? modifiers|0x08 : modifiers&~0x08; break;
    case KEY_RIGHTCTRL: modifiers = ev.value ? modifiers|0x10 : modifiers&~0x10; break;
    case KEY_RIGHTSHIFT:modifiers = ev.value ? modifiers|0x20 : modifiers&~0x20; break;
    case KEY_RIGHTALT:  modifiers = ev.value ? modifiers|0x40 : modifiers&~0x40; break;
    case KEY_RIGHTMETA: modifiers = ev.value ? modifiers|0x80 : modifiers&~0x80; break;
    }

    uint8_t hidCode = 0;
    auto it = evdevToHid.find(ev.code);
    if (it != evdevToHid.end())
        hidCode = it->second;
    if (hidCode == 0) return;

    // Convert pressedKeys array to a vector for easier manipulation
    std::vector<uint8_t> keys;
    for (auto k : pressedKeys)
        if (k != 0) keys.push_back(k);

    if (ev.value) { // Key press
        if (std::find(keys.begin(), keys.end(), hidCode) == keys.end()) {
            if (keys.size() < 6) keys.push_back(hidCode);
        }
    } else { // Key release
        keys.erase(std::remove(keys.begin(), keys.end(), hidCode), keys.end());
    }

    // Copy back to pressedKeys array
    pressedKeys.fill(0);
    for (size_t i = 0; i < keys.size(); i++)
        pressedKeys[i] = keys[i];

    emitReport();
}


void KeyboardHidEmu::emitReport()
{
    std::array<uint8_t,8> report;
    report[0] = modifiers;
    report[1] = 0; // reserved
    for(int i=0;i<6;i++)
        report[2+i] = pressedKeys[i];

    emit hidReportReady(report);
}
