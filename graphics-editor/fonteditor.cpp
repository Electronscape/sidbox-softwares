#include "fonteditor.h"

#include <QObject>
#include <QMouseEvent>
#include <QSettings>
#include <QFileDialog>
#include <QString>
#include <QMessageBox>

#include <stdio.h>

QGraphicsView *gfxSelectorView;
QGraphicsView *gfxEditorView;
QObject       *host;

#define editorViewPortWidth     16
#define editorViewPortHeight    16

int FShoverPixelX = -1, FShoverPixelY = -1; // FSelector View
int FEhoverPixelX = -1, FEhoverPixelY = -1; // FEditor View


int selectedFontID = 0;

extern unsigned char SYSFONT[256][8];

//FontEditor::FontEditor(QGraphicsView *fnSelectorView, QGraphicsView *fnEditorView, QObject *parent){
FontEditor::FontEditor(QGraphicsView *fnSelectorView, QGraphicsView *fnEditorView, QObject *parent) : QObject(parent)
{

    gfxSelectorView = fnSelectorView;
    gfxEditorView = fnEditorView;
    host = parent;

    fontSelectScene = new QGraphicsScene(parent);
    fontSelectImg = QImage(256, 256, QImage::Format_ARGB32);
    fontSelectImg.fill(Qt::transparent);
    fontSelectPixmap = fontSelectScene->addPixmap(QPixmap::fromImage(fontSelectImg));

    fontEditScene = new QGraphicsScene(parent);
    fontEditImg = QImage(256, 256, QImage::Format_ARGB32);
    fontEditImg.fill(Qt::transparent);
    fontEditPixmap = fontEditScene->addPixmap(QPixmap::fromImage(fontEditImg));


    fnSelectorView->setScene(fontSelectScene);
    fnSelectorView->setSceneRect(0, 0, 256, 256);
    fnSelectorView->setMouseTracking(true);
    fnSelectorView->viewport()->setMouseTracking(true); // important for QGraphicsView
    fnSelectorView->viewport()->installEventFilter(this); // important


    fnEditorView->setScene(fontEditScene);
    fnEditorView->setSceneRect(0, 0, 256, 256);
    fnEditorView->setMouseTracking(true);
    fnEditorView->viewport()->setMouseTracking(true); // important for QGraphicsView
    fnEditorView->viewport()->installEventFilter(this); // important

    RenderFontSelect();
    RenderFontEdit();
}


bool FontEditor::eventFilter(QObject *obj, QEvent *event){
    // Only handle events for the view’s viewport

    //if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)
    // Only the Font Select Eat My Events!!
    if(obj == gfxSelectorView->viewport()){
        if (event->type() == QEvent::MouseButtonPress){
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePos = gfxSelectorView->mapToScene(mouseEvent->pos());
            if(mouseEvent->buttons() == Qt::LeftButton){
                FShoverPixelX = int(scenePos.x()) / 16;
                FShoverPixelY = int(scenePos.y()) / 16;

                selectedFontID = (FShoverPixelY * 16) + FShoverPixelX;

                if(FShoverPixelX < 0) FShoverPixelX = 0;
                if(FShoverPixelY < 0) FShoverPixelY = 0;
                if(FShoverPixelX > editorViewPortWidth - 1)  FShoverPixelX = editorViewPortWidth-1;
                if(FShoverPixelY > editorViewPortHeight - 1) FShoverPixelY = editorViewPortHeight-1;
                FShoverPixelX = FShoverPixelX * 16;
                FShoverPixelY = FShoverPixelY * 16;
                handleMouse(scenePos, event); // your internal handler
                RenderFontEdit();
            }
            return true; // mark as handled
        }
    } else
    if(obj == gfxEditorView->viewport()){
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove){
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePos = gfxEditorView->mapToScene(mouseEvent->pos());
            FEhoverPixelX = int(scenePos.x()) / 32;
            FEhoverPixelY = int(scenePos.y()) / 32;

            if(FEhoverPixelX < 0) FEhoverPixelX = 0;
            if(FEhoverPixelY < 0) FEhoverPixelY = 0;
            if(FEhoverPixelX > 7) FEhoverPixelX = 7;
            if(FEhoverPixelY > 7) FEhoverPixelY = 7;

            if(mouseEvent->buttons() == Qt::LeftButton){
                SYSFONT[selectedFontID][FEhoverPixelX] |= (1 << FEhoverPixelY);
            } else
            if(mouseEvent->buttons() == Qt::RightButton){
                SYSFONT[selectedFontID][FEhoverPixelX] &= ~(1 << FEhoverPixelY);
            }
            RenderFontEdit();
            RenderFontSelect();
            return true; // mark as handled
        }
    }
    //return QObject::eventFilter(obj, event);
    return false;
}



void FontEditor::MoveFont(char dir){
    switch(dir){
        case font_move_left: {// move left
            unsigned char first = SYSFONT[selectedFontID][0];
            for (int y = 0; y < 7; ++y){
                SYSFONT[selectedFontID][y] = SYSFONT[selectedFontID][y + 1];
            }
            SYSFONT[selectedFontID][7] = first;
        } break;

        case font_move_up: {// move up
            for (int y = 0; y < 8; ++y){
                unsigned char row = SYSFONT[selectedFontID][y];
                unsigned char carry = (row & 0x01) << 7;   // bit 0 → bit 7
                SYSFONT[selectedFontID][y] = (row >> 1) | carry;
            }
        } break;

        case font_move_right: {// move right
            unsigned char last = SYSFONT[selectedFontID][7];
            for (int y = 7; y > 0; --y){
                SYSFONT[selectedFontID][y] = SYSFONT[selectedFontID][y - 1];
            }
            SYSFONT[selectedFontID][0] = last;
        } break;

        case font_move_down: {// move down
            for (int y = 0; y < 8; ++y){
                unsigned char row = SYSFONT[selectedFontID][y];
                unsigned char carry = (row & 0x80) >> 7;   // bit 7 → bit 0
                SYSFONT[selectedFontID][y] = (row << 1) | carry;
            }
        } break;


        case font_rotate_c:{   // counter clockwise rotate
            unsigned char temp[8];
            for (int i = 0; i < 8; ++i)
                temp[i] = 0;

            for (int y = 0; y < 8; ++y){
                unsigned char row = SYSFONT[selectedFontID][y];
                for (int x = 0; x < 8; ++x){
                    if (row & (1 << (7 - x))){
                        temp[x] |= (1 << y);
                    }
                }
            }

            for (int i = 0; i < 8; ++i)
                SYSFONT[selectedFontID][i] = temp[i];
        } break;

        case font_rotate_cc:{    // counter clockwise rotate
            unsigned char temp[8];
            for (int i = 0; i < 8; ++i)
                temp[i] = 0;

            for (int y = 0; y < 8; ++y){
                unsigned char row = SYSFONT[selectedFontID][y];
                for (int x = 0; x < 8; ++x){
                    if (row & (1 << (7 - x))){
                        temp[7 - x] |= (1 << (7 - y));
                    }
                }
            }

            for (int i = 0; i < 8; ++i)
                SYSFONT[selectedFontID][i] = temp[i];
        } break;

        case font_rotate_c_all:{   // counter clockwise rotate
            unsigned char temp[8];

            for(int fontID = 0; fontID < 256; fontID ++){
                for (int i = 0; i < 8; ++i)
                    temp[i] = 0;

                for (int y = 0; y < 8; ++y){
                    unsigned char row = SYSFONT[fontID][y];
                    for (int x = 0; x < 8; ++x){
                        if (row & (1 << (7 - x))){
                            temp[x] |= (1 << y);
                        }
                    }
                }

                for (int i = 0; i < 8; ++i)
                    SYSFONT[fontID][i] = temp[i];
            }
        } break;

        case font_rotate_cc_all:{    // counter clockwise rotate
            unsigned char temp[8];

            for(int fontID = 0; fontID < 256; fontID ++){
                for (int i = 0; i < 8; ++i)
                    temp[i] = 0;

                for (int y = 0; y < 8; ++y){
                    unsigned char row = SYSFONT[fontID][y];
                    for (int x = 0; x < 8; ++x){
                        if (row & (1 << (7 - x))){
                            temp[7 - x] |= (1 << (7 - y));
                        }
                    }
                }

                for (int i = 0; i < 8; ++i)
                    SYSFONT[fontID][i] = temp[i];
            }
        } break;

        case font_flip_h:{
            for (int y = 0; y < 8; ++y){
                unsigned char row = SYSFONT[selectedFontID][y];
                unsigned char flipped = 0;
                for (int x = 0; x < 8; ++x){
                    if (row & (1 << x))
                        flipped |= (1 << (7 - x));
                }
                SYSFONT[selectedFontID][y] = flipped;
            }
        }
        break;


        case font_flip_v:{
            for (int y = 0; y < 4; ++y){
                unsigned char tmp = SYSFONT[selectedFontID][y];
                SYSFONT[selectedFontID][y] = SYSFONT[selectedFontID][7 - y];
                SYSFONT[selectedFontID][7 - y] = tmp;
            }
        }
        break;
    }
    RenderFontEdit();
    RenderFontSelect();
}

void FontEditor::RenderFontEdit(){
    // this is the area for the font_data_area SYSFONT[selectedid][&0];

    fontEditImg.fill(Qt::black);
    const QRgb fg = qRgba(50, 88, 200, 255);

    int c = selectedFontID;
    for (int x = 0; x < 8; ++x){
        unsigned char col = SYSFONT[c][x];
        for (int y = 0; y < 8; ++y){
            bool set = col & (1 << (y));

            int px = x * 32;
            int py = y * 32;

            if (set){
                for(int fx = 0; fx < 32; fx++){
                    for(int fy = 0; fy < 32; fy++){
                        fontEditImg.setPixel(fx + px, fy + py, fg);
                    }
                }

            }
        }
    }

    fontEditPixmap->setPixmap(QPixmap::fromImage(fontEditImg));
    fontEditScene->setSceneRect(fontEditPixmap->boundingRect());
}


void FontEditor::RenderFontSelect(){
    fontSelectImg.fill(Qt::black);
    const QRgb fg = qRgba(240, 160, 100, 255);
    const QRgb bg = qRgba(0, 64, 0, 255);

    if (FShoverPixelX >= 0 && FShoverPixelY >= 0){
        const QRgb highlightColor = qRgba(255, 255, 0, 255); // yellow

        // Draw a 16x16 rectangle (since each glyph is 8x8 scaled 2x)
        for (int y = 0; y < 16; ++y){
            for (int x = 0; x < 16; ++x){
                // Only draw the border (4 sides)
                fontSelectImg.setPixel(FShoverPixelX + x, FShoverPixelY + y, bg);
            }
        }
    }

    for (int c = 0; c < 256; ++c){
        int gx = c % 16;
        int gy = c / 16;
        int baseX = gx * 8;
        int baseY = gy * 8;
        for (int x = 0; x < 8; ++x){
            unsigned char col = SYSFONT[c][x];
            for (int y = 0; y < 8; ++y){
                bool set = col & (1 << (y));
                int px = (baseX + x) * 2;
                int py = (baseY + y) * 2;

                if (set){
                    fontSelectImg.setPixel(px,     py,     fg);
                    fontSelectImg.setPixel(px + 1, py,     fg);
                    fontSelectImg.setPixel(px,     py + 1, fg);
                    fontSelectImg.setPixel(px + 1, py + 1, fg);
                }
            }
        }
    }

    // now for the XY selector box (hover highligher bit here)
    // using hoverPixelX / hoverPixelY
    // Draw hover rectangle if valid
    if (FShoverPixelX >= 0 && FShoverPixelY >= 0){
        const QRgb highlightColor = qRgba(255, 255, 0, 255); // yellow

        // Draw a 16x16 rectangle (since each glyph is 8x8 scaled 2x)
        for (int y = -2; y < 18; ++y){
            for (int x = -2; x < 18; ++x){
                // Only draw the border (4 sides)
                if (x < 0 || x > 15 || y < 0 || y > 15){
                    if( ((FShoverPixelX + x) >= 256) || ((FShoverPixelY + y) >= 256)) continue; // this pixel went over
                    if( (FShoverPixelX + x) >= 0 && (FShoverPixelY + y) >= 0)
                        fontSelectImg.setPixel(FShoverPixelX + x, FShoverPixelY + y, highlightColor);
                }
            }
        }
    }

    fontSelectPixmap->setPixmap(QPixmap::fromImage(fontSelectImg));
    fontSelectScene->setSceneRect(fontSelectPixmap->boundingRect());
}

void FontEditor::handleMouse(const QPointF &pt, QEvent *event){
    RenderFontSelect();
}


void FontEditor::SaveFontAs(){
    // save de palette!!
    static QString lastDir;
    QSettings settings("Electronscape", "SidBox-GraphicsEditV3");
    lastDir = settings.value("lastFontProjectDir", QDir::homePath()).toString();

    QString filename = QFileDialog::getSaveFileName(gfxSelectorView, "Save Font", lastDir, "Font Project (*.pfn)");
    // saveIcon(filename);
    if(!filename.isEmpty()){
        QFileInfo info(filename);
        settings.setValue("lastFontProjectDir", info.absolutePath());

        FILE *f = fopen(filename.toUtf8().constData(), "wb");
        if(!f) {
            QMessageBox::warning(gfxSelectorView, "Save Palette Fail", "Cannot open file for writing!");
            return;
        }
        fwrite(SYSFONT, sizeof(uint8_t), (256 * 8), f);
        fclose(f);
    }
}

void FontEditor::LoadFont(){
    static QString lastDir;
    QSettings settings("Electronscape", "SidBox-GraphicsEditV3");
    lastDir = settings.value("lastFontProjectDir", QDir::homePath()).toString();
    QString filename = QFileDialog::getOpenFileName(gfxSelectorView, "Load Font", lastDir, "Font Project (*.pfn)");

    if(!filename.isEmpty()){
        QFileInfo info(filename);
        settings.setValue("lastFontProjectDir", info.absolutePath());

        FILE *f = fopen(filename.toUtf8().constData(), "rb");
        if(!f) {
            QMessageBox::warning(gfxSelectorView, "Save Palette Fail", "Cannot open file for writing!");
            return;
        }
        //fwrite(SYSFONT, sizeof(uint8_t), (256 * 8), f);
        fread(SYSFONT, sizeof(uint8_t), (256 * 8), f);
        fclose(f);

        RenderFontEdit();
        RenderFontSelect();
    }
}



QString FontEditor::hex8(uint8_t value){
    // returns a string like "0xAF" (0x lowercase, digits uppercase)
    return QString("0x%1").arg(QString::number(value, 16).toUpper().rightJustified(2, '0'));
}


void FontEditor::ExportFont(QPlainTextEdit *tb){
    // this is where we export the font to a text box
    //txtOutputText

    const char* symbols[] = {
        "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        ":", ";", "<", "=", ">", "?", "@",
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "["
    };

    const char* symbols2[] = {
        "]", "^", "_", "`",
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
        "{", "|", "}", "~"
    };

    tb->clear();  // clear previous content

    QString output;
    output += "#include <stdint.h>\n\n";
    output += "uint8_t font[256][8] = {\n";

    for (int c = 0; c < 256; ++c){
        output += "    ";  // indent + start of glyph row
        for (int y = 0; y < 8; ++y){
            output += hex8(SYSFONT[c][y]);
            if (y < 7) output += ", ";
        }

        if (c < 255)
            output += ",";  // trailing comma for all but last glyph

        // add tab + comment with index
        if( c > 32 && c < 92)
            output += "   // (" + QString("%1").arg(c, 3) + ")  [ " + symbols[c-33] + " ]";
        else if( c > 92 && c < 127)
            output += "   // (" + QString("%1").arg(c, 3) + ")  [ " + symbols2[c-93] + " ]";
        else if( c == 92)
            output += "   // (" + QString("%1").arg(c, 3) + ")  [BACKSPACE]";
        else if( c == 32)
            output += "   // (" + QString("%1").arg(c, 3) + ")  [SPACE]";
        else
            output += "   // (" + QString("%1").arg(c, 3) + ")   0x" + QString("%1").arg(c, 2, 16, QChar('0')).toUpper();

        output += "\n";
    }

    output += "};\n";
    tb->setPlainText(output);
}

