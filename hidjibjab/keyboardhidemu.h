#ifndef KEYBOARDHIDEMU_H
#define KEYBOARDHIDEMU_H

#include <QObject>
#include <QSocketNotifier>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <array>
#include <map>

class KeyboardHidEmu : public QObject
{
    Q_OBJECT
public:
    explicit KeyboardHidEmu(QObject *parent = nullptr);
    ~KeyboardHidEmu();

    // Start reading events
    bool start();

signals:
    // Emits full 8-byte HID report
    void hidReportReady(const std::array<uint8_t,8> &report);

private:
    int fd;
    QSocketNotifier *notifier;
    uint8_t modifiers;
    std::array<uint8_t,6> pressedKeys;

    // mapping evdev KEY codes -> USB HID codes
    std::map<int,uint8_t> evdevToHid;

    void initKeymap();
    void handleEvent(const struct input_event &ev);
    void emitReport();
    QString findKeyboardEventDevice();
};

#endif // KEYBOARDHIDEMU_H
