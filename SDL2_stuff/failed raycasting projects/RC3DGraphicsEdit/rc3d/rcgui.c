#include "rcgui.h"

#include <string.h>

#include "../gfx.h"

static void rcguiDrawOneButton(const RCGUI_Context *ui, const RCGUI_Button *btn)
{
    uint8_t bg = ui->btnBg;
    uint8_t border = ui->btnBorder;
    uint8_t textCol = ui->btnText;

    if (!btn->visible) {
        return;
    }

    if (btn->disabled) {
        bg = ui->btnDisabled;
        border = ui->btnBorderDisabled;
        textCol = ui->btnTextDisabled;
    } else if (btn->active) {
        bg = ui->btnActive;
    } else if (btn->hot) {
        bg = ui->btnHover;
    }

    drawRect(btn->x, btn->y, btn->w, btn->h, bg);
    drawLine(btn->x, btn->y, btn->x + btn->w - 1, btn->y, border);
    drawLine(btn->x, btn->y + btn->h - 1, btn->x + btn->w - 1, btn->y + btn->h - 1, border);
    drawLine(btn->x, btn->y, btn->x, btn->y + btn->h - 1, border);
    drawLine(btn->x + btn->w - 1, btn->y, btn->x + btn->w - 1, btn->y + btn->h - 1, border);

    if (btn->text && btn->text[0]) {
        drawText(btn->x + 4, btn->y + 2, btn->text, textCol);
    }
}

void rcguiInit(RCGUI_Context *ui)
{
    memset(ui, 0, sizeof(*ui));

    ui->btnBg = 13;
    ui->btnBorder = 27;
    ui->btnText = 2;
    ui->btnHover = 12;
    ui->btnActive = 19;
    ui->btnDisabled = 6;
    ui->btnTextDisabled = 5;
    ui->btnBorderDisabled = 16;
}

void rcguiSetButtonColours(RCGUI_Context *ui,
                           uint8_t btnBg,
                           uint8_t btnBorder,
                           uint8_t btnText,
                           uint8_t btnHover,
                           uint8_t btnActive,
                           uint8_t btnDisabled,
                           uint8_t btnTextDisabled,
                           uint8_t btnBorderDisabled)
{
    ui->btnBg = btnBg;
    ui->btnBorder = btnBorder;
    ui->btnText = btnText;
    ui->btnHover = btnHover;
    ui->btnActive = btnActive;
    ui->btnDisabled = btnDisabled;
    ui->btnTextDisabled = btnTextDisabled;
    ui->btnBorderDisabled = btnBorderDisabled;
}

int rcguiPointInRect(int px, int py, int x, int y, int w, int h)
{
    return (px >= x && px < (x + w) && py >= y && py < (y + h));
}

RCGUI_Button *rcguiGetButton(RCGUI_Context *ui, int id)
{
    int i;

    for (i = 0; i < ui->buttonCount; i++) {
        if (ui->buttons[i].id == id) {
            return &ui->buttons[i];
        }
    }

    return 0;
}

const RCGUI_Button *rcguiGetButtonConst(const RCGUI_Context *ui, int id)
{
    int i;

    for (i = 0; i < ui->buttonCount; i++) {
        if (ui->buttons[i].id == id) {
            return &ui->buttons[i];
        }
    }

    return 0;
}

int rcguiCreateButton(RCGUI_Context *ui,
                      int id,
                      int x,
                      int y,
                      int w,
                      int h,
                      const char *text)
{
    RCGUI_Button *btn;

    if (ui->buttonCount >= RCGUI_MAX_BUTTONS) {
        return 0;
    }

    if (rcguiGetButton(ui, id) != 0) {
        return 0;
    }

    btn = &ui->buttons[ui->buttonCount];
    ui->buttonCount++;

    btn->id = id;
    btn->x = x;
    btn->y = y;
    btn->w = w;
    btn->h = h;
    btn->text = text;
    btn->visible = 1;
    btn->disabled = 0;
    btn->hot = 0;
    btn->active = 0;

    return 1;
}

void rcguiSetButtonText(RCGUI_Context *ui, int id, const char *text)
{
    RCGUI_Button *btn = rcguiGetButton(ui, id);
    if (!btn) return;
    btn->text = text;
}

void rcguiSetButtonRect(RCGUI_Context *ui, int id, int x, int y, int w, int h)
{
    RCGUI_Button *btn = rcguiGetButton(ui, id);
    if (!btn) return;

    btn->x = x;
    btn->y = y;
    btn->w = w;
    btn->h = h;
}

void rcguiSetButtonVisible(RCGUI_Context *ui, int id, int visible)
{
    RCGUI_Button *btn = rcguiGetButton(ui, id);
    if (!btn) return;
    btn->visible = visible ? 1u : 0u;
}

void rcguiSetButtonDisabled(RCGUI_Context *ui, int id, int disabled)
{
    RCGUI_Button *btn = rcguiGetButton(ui, id);
    if (!btn) return;
    btn->disabled = disabled ? 1u : 0u;
}

void rcguiUpdate(RCGUI_Context *ui,
                 int mouseX,
                 int mouseY,
                 int leftDown,
                 int leftPressed,
                 int leftReleased)
{
    int i;

    ui->mouseX = mouseX;
    ui->mouseY = mouseY;
    ui->leftDown = leftDown;
    ui->leftPressed = leftPressed;
    ui->leftReleased = leftReleased;

    ui->hotId = 0;
    ui->hitId = 0;

    for (i = 0; i < ui->buttonCount; i++) {
        RCGUI_Button *btn = &ui->buttons[i];
        int hot;

        btn->hot = 0;
        btn->active = 0;

        if (!btn->visible) {
            continue;
        }

        if (btn->disabled) {
            continue;
        }

        hot = rcguiPointInRect(mouseX, mouseY, btn->x, btn->y, btn->w, btn->h);

        if (hot) {
            ui->hotId = btn->id;
            btn->hot = 1;
        }

        if (hot && leftPressed) {
            ui->activeId = btn->id;
        }

        if (ui->activeId == btn->id) {
            btn->active = 1;
        }

        if (leftReleased && ui->activeId == btn->id) {
            if (hot) {
                ui->hitId = btn->id;
            }
        }
    }

    if (!leftDown) {
        ui->activeId = 0;
    }
}

void rcguiDraw(const RCGUI_Context *ui)
{
    int i;

    for (i = 0; i < ui->buttonCount; i++) {
        rcguiDrawOneButton(ui, &ui->buttons[i]);
    }
}

int rcguiGetButtonHit(RCGUI_Context *ui)
{
    int id = ui->hitId;
    ui->hitId = 0;
    return id;
}

int rcguiGetHotButton(const RCGUI_Context *ui)
{
    return ui->hotId;
}

int rcguiGetActiveButton(const RCGUI_Context *ui)
{
    return ui->activeId;
}