// hid_passer_linux_fixed.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <termios.h>
#include <unistd.h>

#define PACKET_SIZE 12
#define CMD_HISTORY_SIZE 10
#define CMD_MAX_LEN 256
#define CMD_INPUT_LEN 74
#define MAX_LINEBUFFER 2000
#define MAX_LINES 25
#define MAX_LINE_LEN 84

// === GLOBALS ===
int isCommandMode = 0;
char cmdHistory[CMD_HISTORY_SIZE][CMD_MAX_LEN];
int cmdHistoryIndex = 0;
int cmdHistoryCount = 0;
int cmdHistoryViewIndex = -1;

char inputbuffer[MAX_LINEBUFFER][MAX_LINE_LEN];
int totalLines = 0;
int scrollOffset = 0;
int wrapWidth = 82;

char commandBuffer[CMD_MAX_LEN];
int commandPos = 0;

int serial_fd = -1;
int kbd_fd = -1;
uint8_t keyReport[8] = {0};
volatile bool exit_requested = false;

// SIGINT handler
void signal_handler(int sig) {
    exit_requested = true;
}

// === ANSI HELPER ===
void set_console_title(const char *title) {
    printf("\033]0;%s\007", title);
    fflush(stdout);
}

void clear_screen() {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void move_cursor(int x, int y) {
    printf("\033[%d;%dH", y + 1, x + 1);
    fflush(stdout);
}

void hide_cursor() { printf("\033[?25l"); fflush(stdout); }
void show_cursor() { printf("\033[?25h"); fflush(stdout); }

void set_text_color(int fg, int bg) {
    if (bg >= 0)
        printf("\033[%d;%dm", fg + 30, bg + 40);  // 30-37 fg, 40-47 bg
    else
        printf("\033[%dm", fg + 30);
    fflush(stdout);
}

void reset_color() {
    printf("\033[0m");
    fflush(stdout);
}

// === CRC8 ===
uint8_t crc8(uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x07) : (crc << 1);
    }
    return crc;
}

// === HISTORY ===
void CMDHistory_Add(const char *cmd) {
    if (strlen(cmd) == 0) return;
    if (cmdHistoryCount > 0 &&
        strcmp(cmdHistory[(cmdHistoryIndex - 1 + CMD_HISTORY_SIZE) % CMD_HISTORY_SIZE], cmd) == 0)
        return;
    strncpy(cmdHistory[cmdHistoryIndex], cmd, CMD_MAX_LEN - 1);
    cmdHistory[cmdHistoryIndex][CMD_MAX_LEN - 1] = '\0';
    cmdHistoryIndex = (cmdHistoryIndex + 1) % CMD_HISTORY_SIZE;
    if (cmdHistoryCount < CMD_HISTORY_SIZE) cmdHistoryCount++;
    cmdHistoryViewIndex = -1;
}

const char* CMDHistory_Prev() {
    if (cmdHistoryCount == 0) return NULL;
    if (cmdHistoryViewIndex < cmdHistoryCount - 1) cmdHistoryViewIndex++;
    int idx = (cmdHistoryIndex - 1 - cmdHistoryViewIndex + CMD_HISTORY_SIZE) % CMD_HISTORY_SIZE;
    return cmdHistory[idx];
}

const char* CMDHistory_Next() {
    if (cmdHistoryCount == 0) return NULL;
    if (cmdHistoryViewIndex > 0) {
        cmdHistoryViewIndex--;
    } else {
        cmdHistoryViewIndex = -1;
        return "";
    }
    int idx = (cmdHistoryIndex - 1 - cmdHistoryViewIndex + CMD_HISTORY_SIZE) % CMD_HISTORY_SIZE;
    return cmdHistory[idx];
}

// === SERIAL ===
int open_serial(const char *port, int baud) {
    printf("DEBUG: Opening serial %s @ %d baud...\n", port, baud);
    serial_fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (serial_fd < 0) {
        fprintf(stderr, "ERROR: Failed to open %s: %s\n", port, strerror(errno));
        return 0;
    }

    struct termios tty;
    if (tcgetattr(serial_fd, &tty) != 0) {
        fprintf(stderr, "ERROR: tcgetattr failed: %s\n", strerror(errno));
        close(serial_fd);
        return 0;
    }

    cfsetospeed(&tty, baud);
    cfsetispeed(&tty, baud);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "ERROR: tcsetattr failed: %s\n", strerror(errno));
        close(serial_fd);
        serial_fd = -1;
        return 0;
    }
    printf("DEBUG: Serial opened successfully.\n");
    return 1;
}

char* find_keyboard_device() {
    FILE *fp = fopen("/proc/bus/input/devices", "r");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot open /proc/bus/input/devices: %s\n", strerror(errno));
        return NULL;
    }

    char line[256];
    char event_name[32] = {0};
    bool found_kbd = false;
    bool found_handler = false;

    while (fgets(line, sizeof(line), fp)) {
        // Look for keyboard
        if (strstr(line, "keyboard") && strstr(line, "Handlers")) {
            found_kbd = true;
        }
        // Look for eventX in Handlers
        if (strstr(line, "Handlers=") && strstr(line, "event")) {
            char *ev = strstr(line, "event");
            if (ev) {
                ev += 5;  // skip "event"
                char *space = strchr(ev, ' ');
                if (space) *space = '\0';
                strncpy(event_name, ev, sizeof(event_name) - 1);
                found_handler = true;
            }
        }
        // If we have both, we're done
        if (found_kbd && found_handler && event_name[0]) {
            break;
        }
        // Reset on new device
        if (line[0] == '\n' || strstr(line, "Name=")) {
            found_kbd = found_handler = false;
            event_name[0] = '\0';
        }
    }
    fclose(fp);

    if (event_name[0]) {
        char *path = malloc(64);
        if (path) {
            snprintf(path, 64, "/dev/input/event%s", event_name);
            printf("DEBUG: Found keyboard: %s\n", path);

            return path;
        }
    }

    // Fallback: scan event0 to event15
    for (int i = 0; i < 16; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        if (access(path, R_OK) == 0) {
            struct input_id id;
            int fd = open(path, O_RDONLY);
            if (fd >= 0) {
                if (ioctl(fd, EVIOCGID, &id) >= 0) {
                    close(fd);
                    char *result = strdup(path);
                    printf("DEBUG: Fallback keyboard: %s\n", result);
                    return result;
                }
                close(fd);
            }
        }
    }

    fprintf(stderr, "ERROR: No keyboard found! Try: sudo evtest\n");
    return NULL;
}

// === PACKET SEND ===
void send_packet(uint8_t *report) {
    uint8_t packet[PACKET_SIZE] = {0xFA, 0xFB};
    memcpy(&packet[2], report, 8);
    packet[10] = 0xFE;
    packet[11] = crc8(packet, 11);

    // Print packet (same as original)
    hide_cursor();
    set_text_color(37, 0); // white
    move_cursor(2, 6);
    printf("Sending packet: ");
    set_text_color(32, 0); // green
    printf("[");
    for (int i = 0; i < 2; i++) printf("%02X ", packet[i]);
    set_text_color(37, 0); // white
    for (int i = 2; i < 10; i++) {
        printf("%02X", packet[i]);
        if (i < 9) printf(" ");
    }
    set_text_color(32, 0);
    printf(" %02X %02X]", packet[10], packet[11]);
    reset_color();
    printf("\n");

    ssize_t written = write(serial_fd, packet, PACKET_SIZE);
    if (written != PACKET_SIZE) {
        fprintf(stderr, "ERROR: Write failed: %zd / %d bytes\n", written, PACKET_SIZE);
    }
    fsync(serial_fd);
}

// === HID MAPPING (Fixed for Linux codes) ===
uint8_t vkey_to_hid(uint16_t code) {
    // Map Linux KEY_* to HID usage IDs
    switch (code) {
        case KEY_A ... KEY_Z: return 0x04 + (code - KEY_A);
        case KEY_1: return 0x1E; case KEY_2: return 0x1F; case KEY_3: return 0x20;
        case KEY_4: return 0x21; case KEY_5: return 0x22; case KEY_6: return 0x23;
        case KEY_7: return 0x24; case KEY_8: return 0x25; case KEY_9: return 0x26;
        case KEY_0: return 0x27;
        case KEY_ENTER: return 0x28; case KEY_ESC: return 0x29; case KEY_BACKSPACE: return 0x2A;
        case KEY_TAB: return 0x2B; case KEY_SPACE: return 0x2C; case KEY_MINUS: return 0x2D;
        case KEY_EQUAL: return 0x2E; case KEY_LEFTBRACE: return 0x2F; case KEY_RIGHTBRACE: return 0x30;
        //case KEY_BACKSLASH: return 0x31; case KEY_SEMICOLON: return 0x33; case KEY_APOSTROPHE: return 0x34;
        //case KEY_GRAVE: return 0x35; case KEY_COMMA: return 0x36; case KEY_DOT: return 0x37;
        case KEY_SLASH: return 0x38;
        case KEY_LEFT: return 0x50; case KEY_RIGHT: return 0x4F; case KEY_UP: return 0x52; case KEY_DOWN: return 0x51;
        case KEY_DELETE: return 0x4C; case KEY_INSERT: return 0x49; case KEY_HOME: return 0x4A; case KEY_END: return 0x4D;
        case KEY_PAGEUP: return 0x4B; case KEY_PAGEDOWN: return 0x4E;
        case KEY_F1: return 0x3A; case KEY_F2: return 0x3B; case KEY_F3: return 0x3C; case KEY_F4: return 0x3D;
        case KEY_F5: return 0x3E; case KEY_F6: return 0x3F; case KEY_F7: return 0x40; case KEY_F8: return 0x41;
        case KEY_F9: return 0x42; case KEY_F10: return 0x43; case KEY_F11: return 0x44; case KEY_F12: return 0x45;
        default: return 0;
    }
}

void add_key(uint8_t code) {
    for (int i = 2; i < 8; i++) if (keyReport[i] == code) return;
    for (int i = 2; i < 8; i++) if (keyReport[i] == 0) { keyReport[i] = code; return; }
}

void remove_key(uint8_t code) {
    for (int i = 2; i < 8; i++) if (keyReport[i] == code) { keyReport[i] = 0; return; }
}

// === DISPLAY (Simplified ScreenFace + DrawConsoleFR) ===
void DrawConsole() {
    hide_cursor();
    int scrollMax = (totalLines > MAX_LINES) ? (totalLines - MAX_LINES) : 0;
    int start = scrollMax - scrollOffset;
    if (start < 0) start = 0;

    // Scroll position
    move_cursor(70, 7);
    set_text_color(35, 0); // magenta
    printf("%4d/%4d", start + 1, scrollMax + 1);
    reset_color();

    // Lines
    for (int i = 0; i < MAX_LINES; i++) {
        int idx = start + i;
        move_cursor(0, 9 + i);
        if (idx < totalLines) {
            printf(" %-*.*s", MAX_LINE_LEN - 2, MAX_LINE_LEN - 2, inputbuffer[idx]);
        } else {
            printf(" %-*s", MAX_LINE_LEN - 2, "");
        }
    }

    // Header/Footer (your ScreenFace)
    move_cursor(0, 1);
    set_text_color(31, 0); // red
    printf(" Port opened: Listening to keyboard now!\r\n\r\n");
    printf(" ----------------------------------------------------------------------------------\r\n");
    printf(" [h1 h2][--- HID DATA PACK ---][h3] [crc]\r\n\r\n");
    printf("\r\n");
    printf(" Close this window when done\n");
    printf(" ----------------------------------------------------------------------------------\r\n");
    move_cursor(0, 34);
    printf(" ----------------------------------------------------------------------------------\r\n");
    printf("\r\n");
    printf(" ----------------------------------------------------------------------------------\r\n");
    printf(" F10: toggle HID/CMD mode, F12: clear scroll buffer\r\n");
    printf(" insert - paste from clipboard. right click, mark select text, right click to copy\r\n");
    printf(" arrow up/down - scrollup and down, Page Up/Down - page scroll up/down\r\n");
    printf(" home/end - commands history\r\n");
    printf(" exit or endcli to exit this\r\n");

    // Mode
    move_cursor(0, 2);
    set_text_color(32, 0); // green
    printf(" Mode: %s \n", isCommandMode ? "Command Prompt" : "HID Passthrough");

    // CMD line
    move_cursor(0, 35);
    set_text_color(32, 40); // green on blue
    printf(" CMD> ");
    set_text_color(37, 40); // white on blue
    printf("%-73s", commandBuffer);
    reset_color();
    move_cursor(6 + commandPos, 35);
    show_cursor();
    fflush(stdout);
}

void ClearScrollBuffer() {
    totalLines = 0;
    scrollOffset = 0;
    commandPos = 0;
    memset(inputbuffer, 0, sizeof(inputbuffer));
    memset(commandBuffer, 0, sizeof(commandBuffer));
    DrawConsole();
}

void EnterLine(const char *data, int len) {
    if (len == 0) return;
    char temp[10][MAX_LINE_LEN];
    int lines = 0, pos = 0;
    while (pos < len && lines < 10) {
        int take = (len - pos > wrapWidth) ? wrapWidth : (len - pos);
        strncpy(temp[lines], &data[pos], take);
        temp[lines][take] = '\0';
        pos += take;
        lines++;
    }

    // Shift buffer if full
    if (totalLines + lines >= MAX_LINEBUFFER) {
        int shift = totalLines + lines - MAX_LINEBUFFER;
        memmove(inputbuffer, inputbuffer + shift, (MAX_LINEBUFFER - shift) * MAX_LINE_LEN);
        totalLines -= shift;
    }

    for (int i = 0; i < lines && totalLines < MAX_LINEBUFFER; i++) {
        strncpy(inputbuffer[totalLines], temp[i], MAX_LINE_LEN - 1);
        inputbuffer[totalLines][MAX_LINE_LEN - 1] = '\0';
        totalLines++;
    }
    scrollOffset = 0;
    DrawConsole();
}

// Scroll functions
void ScrollUp() {
    if (scrollOffset + MAX_LINES < totalLines) {
        scrollOffset++;
        DrawConsole();
    }
}

void ScrollDown() {
    if (scrollOffset > 0) {
        scrollOffset--;
        DrawConsole();
    }
}
// Global for restore on exit
struct termios original_termios;

void setup_raw_terminal() {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &original_termios) != 0) {
        perror("tcgetattr (save original)");
        return;
    }

    raw = original_termios;

    // Disable echo, canonical mode, signals, etc.
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag |= (CS8);
    raw.c_oflag &= ~(OPOST);

    // Non-blocking read: return immediately
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        perror("tcsetattr (raw mode)");
    }
}

void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}


int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);

    const int allowed_baud[] = {110, 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 56000, 57600, 115200, 128000, 256000};
    int baud = 256000;
    char port[64] = "/dev/ttyUSB0";  // Default


    setup_raw_terminal();
    atexit(restore_terminal);  // Auto-restore on exit

    if (argc >= 2) {
        strncpy(port, argv[1], sizeof(port) - 1);
        port[sizeof(port) - 1] = '\0';
        if (argc >= 3) baud = atoi(argv[2]);
    } else {
        // Try config file
        FILE *conf = fopen("passer2.ini", "r");
        if (conf) {
            fgets(port, sizeof(port), conf);
            port[strcspn(port, "\r\n")] = 0;
            char speed[32];
            fgets(speed, sizeof(speed), conf);
            baud = atoi(speed + 1);  // Skip @
            fclose(conf);
        }
    }

    // Validate baud
    bool valid = false;
    for (int i = 0; i < sizeof(allowed_baud)/sizeof(allowed_baud[0]); i++) {
        if (baud == allowed_baud[i]) { valid = true; break; }
    }
    if (!valid) {
        fprintf(stderr, "Invalid baud %d. Using 256000.\n", baud);
        baud = 256000;
    }

    set_console_title("HID Passthrough V1.0 (Linux Fixed)");
    clear_screen();
    hide_cursor();
    set_text_color(31, 0); // red
    printf("Starting HID Passthrough...\n");
    reset_color();

    // Open serial
    if (!open_serial(port, baud)) {
        fprintf(stderr, "Press Enter to exit...\n");
        getchar();
        return 1;
    }

    // Auto-find keyboard


        // === HARD-CODED KEYBOARD (CORSAIR K55) ===
    char *kbd_path = "/dev/input/event2";
    printf("DEBUG: Using keyboard: %s (CORSAIR K55)\n", kbd_path);

    kbd_fd = open(kbd_path, O_RDONLY | O_NONBLOCK);
    if (kbd_fd < 0) {
        fprintf(stderr, "FATAL: Cannot open %s: %s\n", kbd_path, strerror(errno));
        fprintf(stderr, "Did you log out/in after 'usermod -aG input $USER'?\n");
        close(serial_fd);
        return 1;
    }
    printf("DEBUG: Keyboard opened (fd=%d)\n", kbd_fd);

    for(;;);/*


        char *kbd_path = find_keyboard_device();
    if (!kbd_path) {
        close(serial_fd);
        return 1;
    }
    kbd_fd = open(kbd_path, O_RDONLY | O_NONBLOCK);
    free(kbd_path);
    if (kbd_fd < 0) {
        fprintf(stderr, "ERROR: Failed to open keyboard %s: %s\n", kbd_path, strerror(errno));
        fprintf(stderr, "Try: sudo usermod -a -G input $USER (then log out/in)\n");
        close(serial_fd);
        return 1;
    }
    printf("DEBUG: Keyboard opened.\n");

*/
    // Initial draw
    DrawConsole();

    struct pollfd fds[2] = {
        { .fd = kbd_fd, .events = POLLIN },
        { .fd = serial_fd, .events = POLLIN }
    };

    struct input_event ev;
    uint8_t mod = 0;
    int keyheldtmr = 0;
    int scrollDir = 0;
    char linebuf[256] = {0};
    int linepos = 0;

    printf("DEBUG: Entering main loop. Press Ctrl+C to exit.\n");


    while (!exit_requested) {
        int ret = poll(fds, 2, 500);  // 500ms timeout
        if (ret < 0 && errno != EINTR) break;

        // Serial input (echo to console)
        if (fds[1].revents & POLLIN) {
            char buf[128];
            ssize_t n = read(serial_fd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = 0;
                for (ssize_t i = 0; i < n; i++) {
                    if (buf[i] == '\r') continue;
                    if (buf[i] == '\n') {
                        linebuf[linepos] = 0;
                        EnterLine(linebuf, linepos);
                        linepos = 0;
                    } else if (linepos < sizeof(linebuf) - 1) {
                        linebuf[linepos++] = buf[i];
                    }
                }
            }
        }

        // Keyboard input
        if (fds[0].revents & POLLIN) {
            ssize_t n = read(kbd_fd, &ev, sizeof(ev));
            if (n != sizeof(ev)) continue;

            if (ev.type != EV_KEY) continue;
            bool down = (ev.value == 1);
            uint16_t code = ev.code;

            // F10 toggle
            if (code == KEY_F10 && !down) {
                isCommandMode = !isCommandMode;
                DrawConsole();
                continue;
            }

            // F12 clear
            if (code == KEY_F12 && !down) {
                ClearScrollBuffer();
                continue;
            }

            if (!isCommandMode) {
                // HID passthrough
                if (down) {
                    switch (code) {
                        case KEY_LEFTSHIFT: mod |= 0x02; break;
                        case KEY_RIGHTSHIFT: mod |= 0x20; break;
                        case KEY_LEFTCTRL: mod |= 0x01; break;
                        case KEY_RIGHTCTRL: mod |= 0x10; break;
                        case KEY_LEFTALT: mod |= 0x04; break;
                        case KEY_RIGHTALT: mod |= 0x40; break;
                        default: {
                            uint8_t hid = vkey_to_hid(code);
                            if (hid) add_key(hid);
                        }
                    }
                } else {
                    switch (code) {
                        case KEY_LEFTSHIFT: mod &= ~0x02; break;
                        case KEY_RIGHTSHIFT: mod &= ~0x20; break;
                        case KEY_LEFTCTRL: mod &= ~0x01; break;
                        case KEY_RIGHTCTRL: mod &= ~0x10; break;
                        case KEY_LEFTALT: mod &= ~0x04; break;
                        case KEY_RIGHTALT: mod &= ~0x40; break;
                        default: {
                            uint8_t hid = vkey_to_hid(code);
                            if (hid) remove_key(hid);
                        }
                    }
                }
                keyReport[0] = mod;
                if (keyReport[0] || keyReport[1] || keyReport[2]) send_packet(keyReport);  // Send if changed
            } else {
                // Command mode (simplified ASCII handling)
                if (down) {
                    if (code == KEY_UP) { keyheldtmr = 5; scrollDir = 1; }
                    else if (code == KEY_DOWN) { keyheldtmr = 5; scrollDir = -1; }
                    else if (code == KEY_PAGEUP) { keyheldtmr = 5; scrollDir = 2; ScrollUp(); for (int p=0; p<17; p++) ScrollUp(); }
                    else if (code == KEY_PAGEDOWN) { keyheldtmr = 5; scrollDir = -2; ScrollDown(); for (int p=0; p<17; p++) ScrollDown(); }
                    else if (code == KEY_HOME) {
                        const char *prev = CMDHistory_Prev();
                        if (prev) {
                            strncpy(commandBuffer, prev, sizeof(commandBuffer) - 1);
                            commandPos = strlen(commandBuffer);
                        }
                        DrawConsole();
                    }
                    else if (code == KEY_END) {
                        const char *next = CMDHistory_Next();
                        if (next) {
                            strncpy(commandBuffer, next, sizeof(commandBuffer) - 1);
                            commandPos = strlen(commandBuffer);
                        }
                        DrawConsole();
                    }
                    else if (code == KEY_LEFT && commandPos > 0) { commandPos--; DrawConsole(); }
                    else if (code == KEY_RIGHT && commandPos < strlen(commandBuffer)) { commandPos++; DrawConsole(); }
                    else if (code == KEY_BACKSPACE && commandPos > 0) {
                        memmove(commandBuffer + commandPos - 1, commandBuffer + commandPos, strlen(commandBuffer) - commandPos + 1);
                        commandPos--;
                        DrawConsole();
                    }
                    else if (code == KEY_ENTER) {
                        if (strlen(commandBuffer) > 0) {
                            EnterLine(commandBuffer, strlen(commandBuffer));
                            CMDHistory_Add(commandBuffer);
                            if (strcasecmp(commandBuffer, "exit") == 0 || strcasecmp(commandBuffer, "endcli") == 0) {
                                exit_requested = true;
                            } else {
                                write(serial_fd, commandBuffer, strlen(commandBuffer));
                                write(serial_fd, "\n", 1);
                            }
                            memset(commandBuffer, 0, sizeof(commandBuffer));
                            commandPos = 0;
                            DrawConsole();
                        }
                    }
                    else if (code == KEY_INSERT) {
                        // Paste from clipboard (xclip)
                        FILE *clip = popen("xclip -o -selection clipboard 2>/dev/null", "r");
                        if (clip) {
                            char paste[CMD_INPUT_LEN];
                            if (fgets(paste, sizeof(paste), clip)) {
                                paste[strcspn(paste, "\r\n")] = 0;
                                int addlen = strlen(paste);
                                if (commandPos + addlen < CMD_INPUT_LEN) {
                                    memmove(commandBuffer + commandPos + addlen, commandBuffer + commandPos, strlen(commandBuffer) - commandPos + 1);
                                    memcpy(commandBuffer + commandPos, paste, addlen);
                                    commandPos += addlen;
                                }
                            }
                            pclose(clip);
                            DrawConsole();
                        }
                    }
                    else if (code >= KEY_A && code <= KEY_Z) {
                        char c = 'a' + (code - KEY_A);
                        if (strlen(commandBuffer) < CMD_INPUT_LEN - 1) {
                            memmove(commandBuffer + commandPos + 1, commandBuffer + commandPos, strlen(commandBuffer) - commandPos + 1);
                            commandBuffer[commandPos++] = c;
                            DrawConsole();
                        }
                    }
                    else if (code >= KEY_1 && code <= KEY_0) {
                        char c = '1' + (code - KEY_1);
                        if (code == KEY_0) c = '0';
                        if (strlen(commandBuffer) < CMD_INPUT_LEN - 1) {
                            memmove(commandBuffer + commandPos + 1, commandBuffer + commandPos, strlen(commandBuffer) - commandPos + 1);
                            commandBuffer[commandPos++] = c;
                            DrawConsole();
                        }
                    }
                    else if (code == KEY_SPACE && strlen(commandBuffer) < CMD_INPUT_LEN - 1) {
                        memmove(commandBuffer + commandPos + 1, commandBuffer + commandPos, strlen(commandBuffer) - commandPos + 1);
                        commandBuffer[commandPos++] = ' ';
                        DrawConsole();
                    }
                    // Add more keys as needed (e.g. symbols)
                }
            }

            // Auto-repeat scroll
            if (scrollDir != 0 && keyheldtmr > 0) {
                if (keyheldtmr == 1 || keyheldtmr == 5) {
                    if (scrollDir == 1) ScrollUp();
                    if (scrollDir == -1) ScrollDown();
                    // Page scrolls handled above
                }
                keyheldtmr = (keyheldtmr > 1) ? keyheldtmr - 1 : 1;
            } else {
                keyheldtmr = 0;
            }
        }
    }

    if (kbd_fd >= 0) close(kbd_fd);
    if (serial_fd >= 0) close(serial_fd);
    reset_color();
    clear_screen();
    printf("EXITING... Thanks for using HID Passthrough!\n");
    return 0;
}
