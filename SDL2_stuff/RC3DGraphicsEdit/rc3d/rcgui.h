#ifndef RCGUI_H
#define RCGUI_H

#include <stdint.h>

#ifndef RCGUI_MAX_BUTTONS
#define RCGUI_MAX_BUTTONS 256
#endif

typedef enum
{
    RCGUI_CONTROL_BUTTON = 0,
    RCGUI_CONTROL_TOGGLEBOX
} RCGUI_ControlType;

typedef struct
{
    int id;
    int x;
    int y;
    int w;
    int h;
    const char *text;

    uint8_t type;
    uint8_t visible;
    uint8_t disabled;

    uint8_t hot;
    uint8_t active;

    uint8_t checked;
} RCGUI_Button;

typedef struct
{
    int mouseX;
    int mouseY;
    int leftDown;
    int leftPressed;
    int leftReleased;

    int hotId;
    int activeId;
    int hitId;

    int buttonCount;
    RCGUI_Button buttons[RCGUI_MAX_BUTTONS];

    uint8_t btnBg;
    uint8_t btnBorder;
    uint8_t btnText;
    uint8_t btnHover;
    uint8_t btnHoverText;
    uint8_t btnActive;
    uint8_t btnDisabled;
    uint8_t btnTextDisabled;
    uint8_t btnBorderDisabled;
} RCGUI_Context;

void rcguiInit(RCGUI_Context *ui);

void rcguiSetButtonColours(RCGUI_Context *ui,
                           uint8_t btnBg,
                           uint8_t btnBorder,
                           uint8_t btnText,
                           uint8_t btnHover,
                           uint8_t btnActive,
                           uint8_t btnDisabled,
                           uint8_t btnTextDisabled,
                           uint8_t btnBorderDisabled);

int rcguiCreateButton(RCGUI_Context *ui,
                      int id,
                      int x,
                      int y,
                      int w,
                      int h,
                      const char *text);

int rcguiCreateToggleBox(RCGUI_Context *ui,
                         int id,
                         int x,
                         int y,
                         int w,
                         int h,
                         const char *text,
                         int checked);

RCGUI_Button *rcguiGetButton(RCGUI_Context *ui, int id);
const RCGUI_Button *rcguiGetButtonConst(const RCGUI_Context *ui, int id);

void rcguiSetButtonText(RCGUI_Context *ui, int id, const char *text);
void rcguiSetButtonRect(RCGUI_Context *ui, int id, int x, int y, int w, int h);
void rcguiSetButtonVisible(RCGUI_Context *ui, int id, int visible);
void rcguiSetButtonDisabled(RCGUI_Context *ui, int id, int disabled);

void rcguiSetToggleChecked(RCGUI_Context *ui, int id, int checked);
int rcguiGetToggleChecked(const RCGUI_Context *ui, int id);
void rcguiToggleChecked(RCGUI_Context *ui, int id);

void rcguiUpdate(RCGUI_Context *ui,
                 int mouseX,
                 int mouseY,
                 int leftDown,
                 int leftPressed,
                 int leftReleased);

void rcguiDraw(const RCGUI_Context *ui);

int rcguiGetButtonHit(RCGUI_Context *ui);
int rcguiGetHotButton(const RCGUI_Context *ui);
int rcguiGetActiveButton(const RCGUI_Context *ui);

int rcguiPointInRect(int px, int py, int x, int y, int w, int h);

#endif