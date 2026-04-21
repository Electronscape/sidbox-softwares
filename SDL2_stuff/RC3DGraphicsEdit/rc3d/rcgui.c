#include "rcgui.h"
#include "rc3dedit.h"

#include <SDL2/SDL.h>

#include <stddef.h>
#include <string.h>

#include "../gfx.h"

static const char *rcguiGetTooltipText(const RCGUI_Button *btn)
{
    if (!btn) {
        return NULL;
    }

    if (btn->tooltip && btn->tooltip[0]) {
        return btn->tooltip;
    }

    if (btn->text && btn->text[0]) {
        return btn->text;
    }

    return NULL;
}

static void rcguiMeasureTooltipText(const char *tooltip, int *outMaxChars, int *outLineCount)
{
    int maxChars = 0;
    int lineCount = 1;
    int curChars = 0;

    if (!tooltip) {
        if (outMaxChars) *outMaxChars = 0;
        if (outLineCount) *outLineCount = 0;
        return;
    }

    for (const char *p = tooltip; ; p++) {
        if (*p == '\n' || *p == '\0') {
            if (curChars > maxChars) {
                maxChars = curChars;
            }

            if (*p == '\0') {
                break;
            }

            lineCount++;
            curChars = 0;
            continue;
        }

        curChars++;
    }

    if (outMaxChars) *outMaxChars = maxChars;
    if (outLineCount) *outLineCount = lineCount;
}

static void rcguiDrawTooltip(const RCGUI_Context *ui)
{
    const RCGUI_Button *btn;
    const char *tooltip;
    int x, y, w, h;
    int maxChars;
    int lineCount;
    int lineY;
    const char *lineStart;

    if (!ui || ui->tooltipId == 0) {
        return;
    }

    btn = rcguiGetButtonConst(ui, ui->tooltipId);
    tooltip = rcguiGetTooltipText(btn);

    if (!btn || !btn->visible || !tooltip) {
        return;
    }

    rcguiMeasureTooltipText(tooltip, &maxChars, &lineCount);
    if (maxChars <= 0 || lineCount <= 0) {
        return;
    }

    w = maxChars * 8 + 12;
    h = lineCount * 16 + 12;
    x = ui->mouseX + 14;
    y = ui->mouseY + 22;

    if (x + w > SCREEN_W - 12) {
        x = SCREEN_W - w - 12;
    }

    if (y + h > SCREEN_H - 12) {
        y = ui->mouseY - h - 20;
    }

    if (x < 4) x = 4;
    if (y < 4) y = 4;

    drawRect(x-3, y-3, w+6, h+6, 16);
    drawRect(x, y, w, h, ED_COLOUR_TOOLTIP_BG);
    drawRectL(x, y, w, h, ED_COLOUR_TOOLTIP_BORDER);

    lineStart = tooltip;
    lineY = y + 6;

    for (const char *p = tooltip; ; p++) {
        if (*p == '\n' || *p == '\0') {
            char lineBuf[256];
            ptrdiff_t lineLen = p - lineStart;

            if (lineLen < 0) {
                lineLen = 0;
            }

            if ((size_t)lineLen >= sizeof(lineBuf)) {
                lineLen = (ptrdiff_t)(sizeof(lineBuf) - 1);
            }

            memcpy(lineBuf, lineStart, (size_t)lineLen);
            lineBuf[lineLen] = '\0';
            drawText(x + 6, lineY, lineBuf, ED_COLOUR_TOOLTIP_TEXT);
            lineY += 16;

            if (*p == '\0') {
                break;
            }

            lineStart = p + 1;
        }
    }
}

static void rcguiDrawOneButton(const RCGUI_Context *ui, const RCGUI_Button *btn)
{
    uint8_t bg = ui->btnBg;
    uint8_t border = ui->btnBorder;
    uint8_t textCol = ui->btnText;
    int textx = 0;
    int texty = 0;

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

    if (btn->type == RCGUI_CONTROL_TOGGLEBOX) {
        const int boxSize = 12;
        const int boxX = btn->x + 4;
        const int boxY = btn->y + ((btn->h - boxSize) / 2);

        drawRect(boxX, boxY, boxSize, boxSize, 0);
        drawLine(boxX, boxY, boxX + boxSize - 1, boxY, border);
        drawLine(boxX, boxY + boxSize - 1, boxX + boxSize - 1, boxY + boxSize - 1, border);
        drawLine(boxX, boxY, boxX, boxY + boxSize - 1, border);
        drawLine(boxX + boxSize - 1, boxY, boxX + boxSize - 1, boxY + boxSize - 1, border);

        if (btn->checked) {
            drawRect(boxX + 3, boxY + 3, boxSize - 6, boxSize - 6, textCol);
        }

        if (btn->text && btn->text[0]) {
            textx = boxX + boxSize + 6;
            texty = btn->y + ((btn->h / 2) - 8);
            drawText(textx, texty, btn->text, btn->hot ? ui->btnHoverText : textCol);
        }
    } else {
        if (btn->text && btn->text[0]) {
            textx = (btn->w / 2) - ((int)(strlen(btn->text) * 8) / 2);
            texty = (btn->h / 2) - 8;
            drawText(btn->x + textx, btn->y + texty, btn->text, btn->hot ? ui->btnHoverText : textCol);
        }
    }
}

void rcguiInit(RCGUI_Context *ui)
{
    memset(ui, 0, sizeof(*ui));

    ui->btnBg = ED_COLOUR_BTN_BG;
    ui->btnBorder = ED_COLOUR_BTN_FRAME;
    ui->btnText = ED_COLOUR_BTN_TEXT;
    ui->btnHover = ED_COLOUR_BTN_BG_ACTIVE;
    ui->btnHoverText = ED_COLOUR_BTN_TEXT_HOVER;
    ui->btnActive = ED_COLOUR_BTN_ACTIVE;
    ui->btnDisabled = ED_COLOUR_BTN_BG_DISABLED;
    ui->btnTextDisabled = ED_COLOUR_BTN_TXT_DISABLED;
    ui->btnBorderDisabled = ED_COLOUR_BTN_FRAME_DISABLED;
    ui->tooltipDelayMs = 350u;
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
    btn->tooltip = NULL;
    btn->type = RCGUI_CONTROL_BUTTON;
    btn->visible = 1;
    btn->disabled = 0;
    btn->hot = 0;
    btn->active = 0;
    btn->checked = 0;

    return 1;
}

int rcguiCreateToggleBox(RCGUI_Context *ui,
                         int id,
                         int x,
                         int y,
                         int w,
                         int h,
                         const char *text,
                         int checked)
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
    btn->tooltip = NULL;
    btn->type = RCGUI_CONTROL_TOGGLEBOX;
    btn->visible = 1;
    btn->disabled = 0;
    btn->hot = 0;
    btn->active = 0;
    btn->checked = checked ? 1u : 0u;

    return 1;
}

void rcguiSetButtonText(RCGUI_Context *ui, int id, const char *text)
{
    RCGUI_Button *btn = rcguiGetButton(ui, id);
    if (!btn) return;
    btn->text = text;
}

void rcguiSetButtonTooltip(RCGUI_Context *ui, int id, const char *tooltip)
{
    RCGUI_Button *btn = rcguiGetButton(ui, id);
    if (!btn) return;
    btn->tooltip = tooltip;
}

void rcguiSetTooltipDelay(RCGUI_Context *ui, uint32_t delayMs)
{
    if (!ui) return;
    ui->tooltipDelayMs = delayMs;
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

void rcguiSetToggleChecked(RCGUI_Context *ui, int id, int checked)
{
    RCGUI_Button *btn = rcguiGetButton(ui, id);
    if (!btn) return;
    if (btn->type != RCGUI_CONTROL_TOGGLEBOX) return;
    btn->checked = checked ? 1u : 0u;
}

int rcguiGetToggleChecked(const RCGUI_Context *ui, int id)
{
    const RCGUI_Button *btn = rcguiGetButtonConst(ui, id);
    if (!btn) return 0;
    if (btn->type != RCGUI_CONTROL_TOGGLEBOX) return 0;
    return btn->checked ? 1 : 0;
}

void rcguiToggleChecked(RCGUI_Context *ui, int id)
{
    RCGUI_Button *btn = rcguiGetButton(ui, id);
    if (!btn) return;
    if (btn->type != RCGUI_CONTROL_TOGGLEBOX) return;
    btn->checked = btn->checked ? 0u : 1u;
}

void rcguiUpdate(RCGUI_Context *ui,
                 int mouseX,
                 int mouseY,
                 int leftDown,
                 int leftPressed,
                 int leftReleased)
{
    int i;
    int currentTooltipId = 0;
    int prevTooltipId;
    uint32_t nowMs;

    ui->mouseX = mouseX;
    ui->mouseY = mouseY;
    ui->leftDown = leftDown;
    ui->leftPressed = leftPressed;
    ui->leftReleased = leftReleased;

    ui->hotId = 0;
    ui->hitId = 0;
    prevTooltipId = ui->tooltipId;
    ui->tooltipId = 0;

    for (i = 0; i < ui->buttonCount; i++) {
        RCGUI_Button *btn = &ui->buttons[i];
        int hot;

        btn->hot = 0;
        btn->active = 0;

        if (!btn->visible) {
            continue;
        }

        hot = rcguiPointInRect(mouseX, mouseY, btn->x, btn->y, btn->w, btn->h);

        if (hot && rcguiGetTooltipText(btn)) {
            currentTooltipId = btn->id;
        }

        if (btn->disabled) {
            continue;
        }

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
                if (btn->type == RCGUI_CONTROL_TOGGLEBOX) {
                    btn->checked = btn->checked ? 0u : 1u;
                }
                ui->hitId = btn->id;
            }
        }
    }

    if (!leftDown) {
        ui->activeId = 0;
    }

    nowMs = SDL_GetTicks();

    if (currentTooltipId != ui->tooltipHoverId) {
        ui->tooltipHoverId = currentTooltipId;
        ui->tooltipHoverStartMs = nowMs;
    }

    if (currentTooltipId != 0 &&
        (nowMs - ui->tooltipHoverStartMs) >= ui->tooltipDelayMs) {
        ui->tooltipId = currentTooltipId;
    } else if (currentTooltipId != 0) {
        /* Keep the editor ticking until the hover delay expires. */
        rc3dGuiDirty();
    }

    if (ui->tooltipId != prevTooltipId) {
        rc3dGuiDirty();
    }
}

void rcguiDraw(const RCGUI_Context *ui)
{
    int i;

    for (i = 0; i < ui->buttonCount; i++) {
        rcguiDrawOneButton(ui, &ui->buttons[i]);
    }

    rcguiDrawTooltip(ui);
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
