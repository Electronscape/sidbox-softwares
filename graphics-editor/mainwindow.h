#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "fonteditor.h"
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QObject>



// basic C
enum imageHandlerPos {
    cHandleTL,  // Top left
    cHandleTM,  // Top Middle
    cHandleTR,  // Top Right
    cHandleML,  // Middle Left
    cHandleMM,  // Middle Middle
    cHandleMR,  // Middle Right
    cHandleBL,  // Bottom Left
    cHandleBM,  // Bottom Middle
    cHandleBR   // Bottom Right
};


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



    // font system

private:
    Ui::MainWindow *ui;



    QGraphicsScene *scene;
    QGraphicsScene *editorScene;

    QGraphicsPixmapItem *palettePixmap;
    QGraphicsPixmapItem *editorPixmap;

    QImage paletteImg;
    QImage editorImg;

    void UpdatePrePaletteMixer();

    void SelectedPaletteID();
    void renderPaletteCanvas();
    void renderEditorCanvas();
    void reSize();
    uint32_t colourSqueeze(uint32_t srcColour);
    void saveProjectIcon(const char *filename);
    void loadProjectIcon(const char *filename);
    void updateEditorScrollBars();
    void onEditorScrollChanged();
    void rotateIcon(int direction = 0);
    void ProcessClickPaint(int sx, int sy, unsigned char flags);
    void floodFillGradient(int startX, int startY, uint8_t colStart, uint8_t colEnd, int length, float angleDegrees);
    void floodFill(int startX, int startY, uint8_t fillColor);
    bool importGif(const QString &path);
    void doReassignedPalette(uint8_t targetPalID);
    void doSwapPalette(uint8_t targetPalID);
    void doSpreadPalette(uint8_t targetID);
    void SavePaletteData(const char *filename);
    void LoadPaletteData(const char *filename);

    void ExportToILBM(const char *filename);
    void ExportImageToH(const char *filename, const uint16_t modes);

    void readToolXY(int *rx, int *ry);

    void getTextCenterHandle(int sx, int sy, int* outX, int* outY);
    void drawIconAreaPenHover(int sx, int sy, int size, QImage *edImg, bool filled=false);
    void drawTextHover(int sx, int sy, QImage *edImg);
    void drawText(int sx, int sy, bool setPixel);
    void doColourCycle();
    void onColourCycleTick();
    void onSprayCanTick();

    void getCenterHandle(int sx, int sy, int* outX, int* outY, int width, int height);
    void PlaceBrush(int gridX, int gridY);
    void drawCopyBrushHover(int sx, int sy, QImage *edImg);
    void CopySelectionToBrush();
    void ResizeIconArea(int newWidth, int newHeight, int oldWidth, int oldHeight);
    void clearToolButtons();
    void clearHandlerButtons(char handleMode = cHandleMM);
    void clearBrushSizeButtons(char brushSize = 1);
    void doAlterBrush(int mode);

    // clipboard brush import
    bool clipboardHasImage();
    uint8_t findNearestPaletteColor(QRgb rgb);
    uint8_t findNearestPaletteIndex(int r, int g, int b);
    void setAAPixel(int x, int y, uint8_t fg);
    void pasteClipboardAsBrush();
    void convertImageToBrush(const QImage &img);


    FontEditor *fontEditor;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

signals:
    void tileClicked(int x, int y);

};
#endif // MAINWINDOW_H
