#include "dialog.h"
#include "ui_dialog.h"
#include <QTimer>

#include "sbapi_graphics.h"

long offsetty = 0;

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

    // Add it to the scene
    pixmapItem = scene->addPixmap(QPixmap::fromImage(screenImage));
    pixmapItem->setPos(0, 0);

    // Timer to update at ~60Hz
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Dialog::updateScreen);
    timer->start(16);

    connect(ui->cmdClose, &QPushButton::clicked, this, [=]() {
        this->close();
    });





    sbgfx_fill(9);

    uint8_t colind = 0;
    uint16_t tbx = 0, tby = 0;
    for(tby = 0; tby < 16; tby++){
        for(tbx = 0; tbx < 16; tbx++){
            sbgfx_drawbox(1+tbx * 20, 1+tby * 20, 18, 18, colind);
            colind++;
        }
    }


}

Dialog::~Dialog()
{
    delete ui;
}


void Dialog::updateScreen()
{
    uint8_t *v = VRAM;

    sbgfx_fill(9);

    offsetty++;
    if(offsetty>200)
        offsetty=0;

    uint8_t colind = 0;
    uint16_t tbx = 0, tby = 0;
    for(tby = 0; tby < 16; tby++){
        for(tbx = 0; tbx < 16; tbx++){
            sbgfx_drawbox(offsetty+tbx * 20, 1+tby * 20, 18, 18, colind);
            colind++;
        }
    }

    for (int y = 0; y < SCR_HEIGHT; y++)
    {
        QRgb *scan = reinterpret_cast<QRgb*>(screenImage.scanLine(y));
        for (int x = 0; x < SCR_WIDTH; x++)
        {
            scan[x] = CRAM[*v++]; // look up RGB from color index
        }
    }


    pixmapItem->setPixmap(QPixmap::fromImage(screenImage));
}
