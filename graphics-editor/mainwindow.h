#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QObject>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;



    QGraphicsScene *scene;
    QGraphicsScene *editorScene;

    QGraphicsPixmapItem *palettePixmap;
    QGraphicsPixmapItem *editorPixmap;
    QImage paletteImg;
    QImage editorImg;

    void UpdatePrePaletteMixer();

    void renderPaletteCanvas();
    void renderEditorCanvas();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

signals:
    void tileClicked(int x, int y);

};
#endif // MAINWINDOW_H
