#ifndef FONTEDITOR_H
#define FONTEDITOR_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QPlainTextEdit>

enum {
    font_move_up,
    font_move_down,
    font_move_left,
    font_move_right,
    font_rotate_cc,
    font_rotate_c,
    font_rotate_cc_all,
    font_rotate_c_all,
    font_flip_h,
    font_flip_v
};

void clearFontBank();

class FontEditor : public QObject
{
public:
    FontEditor(QGraphicsView *fnSelectorView, QGraphicsView *fnEditorView, QObject *parent = nullptr);
    QGraphicsScene *fontEditScene;
    QGraphicsScene *fontSelectScene;
    QGraphicsPixmapItem *fontEditPixmap;
    QGraphicsPixmapItem *fontSelectPixmap;
    QImage fontEditImg;
    QImage fontSelectImg;

    void RenderFontSelect();
    void RenderFontEdit();
    void MoveFont(char dir);

    void SaveFontAs();
    void LoadFont();

    void ExportFont(QPlainTextEdit *tb);


private:
    void handleMouse(const QPointF &pt, QEvent *event);
    bool eventFilter(QObject *obj, QEvent *event) ;
    QString hex8(uint8_t value);

};

#endif // FONTEDITOR_H
