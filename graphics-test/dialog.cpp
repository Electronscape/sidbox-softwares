#include "dialog.h"
#include "ui_dialog.h"
#include <QTimer>

#include "sbapi_graphics.h"

float winScale = 1.0f;
int currentScale = 1;


Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    printf("Hello world\n");

    scene = new QGraphicsScene(this);
    ui->gfxPort->setScene(scene);

    // Your framebuffer image (480×320)
    screenImage = QImage(SCR_WIDTH, SCR_HEIGHT, QImage::Format_RGB32);

    ui->gfxPort->setRenderHint(QPainter::SmoothPixmapTransform, true);
    ui->gfxPort->setRenderHint(QPainter::Antialiasing, false); // don't need antialias for pixels

    //ui->gfxPort->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    //ui->gfxPort->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    ui->gfxPort->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->gfxPort->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);


    // Add it to the scene
    pixmapItem = scene->addPixmap(QPixmap::fromImage(screenImage));
    pixmapItem->setTransformationMode(Qt::SmoothTransformation);

    pixmapItem->setPos(0, 0);

    // Timer to update at ~60Hz
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Dialog::updateScreen);
    timer->start(16);

    connect(ui->cmdClose, &QPushButton::clicked, this, [=]() {
        this->close();
    });

    //ui->gfxPort->scale(2.0, 2.0);    // 2× zoom

    connect(ui->cmdZoom1x, &QPushButton::clicked, this, [=](){
        setScreenScale(1);
    });

    connect(ui->cmdZoom2x, &QPushButton::clicked, this, [=](){
        setScreenScale(2);
    });

    connect(ui->cmdZoomWx, &QPushButton::clicked, this, [=](){
        setScreenScale(winScale);
    });

}

void Dialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);

    // Calculate available size for the graphics view, accounting for window frame
    QSize available = this->frameSize();
    int marginX = 8; // tweak if needed
    int marginY = 44;

    float scaleX = static_cast<float>(available.width() - marginX) / SCR_WIDTH;
    float scaleY = static_cast<float>(available.height() - marginY) / SCR_HEIGHT;

    // Keep aspect ratio by using the smaller scale
    float scale = qMin(scaleX, scaleY);
    winScale = scale;

    setScreenScale(winScale);
}

void Dialog::setScreenScale(float factor)
{
    currentScale = factor;

    ui->gfxPort->resetTransform();
    ui->gfxPort->scale(factor, factor);


    int currentY = ui->gfxPort->y();   // get current Y position
    //ui->gfxPort->move(10, currentY);   // move to X=10, keep Y the same

    // Update the graphics view size to match scaled content
    ui->gfxPort->setFixedSize(static_cast<int>(SCR_WIDTH * factor),
                              static_cast<int>(SCR_HEIGHT * factor));


    int winW = this->width();
    int winH = this->height();
    int gfxW = ui->gfxPort->width();
    int gfxH = ui->gfxPort->height();

    int posX = (winW - gfxW) / 2;
    int posY = (winH - gfxH) / 2;

    ui->gfxPort->move(posX, posY+18);



}


Dialog::~Dialog()
{
    delete ui;
}


/////////// ------------------------------- SID BOXY STUFF ----------------------------------------------------------------------------

char strText[128];


void Dialog::updateScreen()
{
    uint8_t *v = VRAM;

    sbgfx_fill(0);

    dopalletecycle();

    uint8_t colind = 0;
    uint16_t tbx = 0, tby = 0;
    for(tby = 0; tby < 16; tby++){
        for(tbx = 0; tbx < 16; tbx++){
            sbgfx_drawbox(tbx * 20, 1+tby * 20, 18, 18, colind);
            colind++;
        }
    }

    sprintf(strText, "Hello world testing ...\nI hope new line also works!");
    gfx_setcolour(208);
    draw_text816(4, 5, (const unsigned char *)strText);
    draw_text816(6, 5, (const unsigned char *)strText);
    draw_text816(5, 4, (const unsigned char *)strText);
    draw_text816(5, 6, (const unsigned char *)strText);

    gfx_setcolour(7);
    draw_text816(5, 5, (const unsigned char *)strText);


    // ------- TRANSFER VRAM to IMAGE output ----------- //
    // simulates the SIDBOX Graphics view LCD
    uint8_t *p = VRAM;
    for (int x = 0; x < SCR_WIDTH; x++){
        for (int y = 0; y < SCR_HEIGHT; y++){
            QRgb *scan = reinterpret_cast<QRgb*>(screenImage.scanLine(y));
            scan[x] = CRAM[*p++];
        }
    }

    pixmapItem->setPixmap(QPixmap::fromImage(screenImage));
}
