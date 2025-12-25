#include "dialog.h"
#include "ui_dialog.h"
#include <QTimer>
#include <QFileDialog>
#include <QString>
#include <QMessageBox>
#include <QSettings>

#include "sbapi_graphics.h"
#include "windowex.h"


float winScale = 1.0f;
int currentScale = 1;

void prepXYs();

Dialog *g_dialog = nullptr;
char frame_db = 0;
char EmuReady = 0;

void sms_keydown(int keycode);
void sms_keyup(int keycode);

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{

    ui->setupUi(this);

    printf("Hello world\n");
    prepXYs();

    scene = new QGraphicsScene(this);
    ui->gfxPort->setScene(scene);

    // Your framebuffer image (480×320)
    screenImageF = QImage(SCR_WIDTH, SCR_HEIGHT, QImage::Format_RGB32);
    screenImageB = QImage(SCR_WIDTH, SCR_HEIGHT, QImage::Format_RGB32);

    ui->gfxPort->setRenderHint(QPainter::SmoothPixmapTransform, true);
    ui->gfxPort->setRenderHint(QPainter::Antialiasing, false); // don't need antialias for pixels

    //ui->gfxPort->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    //ui->gfxPort->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    ui->gfxPort->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->gfxPort->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);


    // Add it to the scene
    pixmapItem = scene->addPixmap(QPixmap::fromImage(screenImageF));
    pixmapItem->setTransformationMode(Qt::SmoothTransformation);

    pixmapItem->setPos(0, 0);

    g_dialog = this;

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

    setScreenScale(1);

/*
    QTimer *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [=](){
        processAudio();
    });
    t->start(1);   // every 1ms
*/

    timer = new QTimer(this);
    QTimer::singleShot(500, this, [=]() {

    });

    QTimer *frameTimer = new QTimer(this);
    connect(frameTimer, &QTimer::timeout, this, [=]() {

    });
    frameTimer->start(16);   // 0ms = run every cycle

    ui->gfxPort->installEventFilter(this);
    ui->gfxPort->setFocusPolicy(Qt::StrongFocus);
    ui->gfxPort->setFocus();

    sbgfx_fill(5);

    //sbgfx_drawbox(00,0,320,256, 3);

    createWindow(320, 256, "CRAP WINDOW V1.0 - yey");

    //gfx_setcolour(1);
    //draw_text816(10, 10, (const unsigned char *)"Hello world");

    updateGFXScreen();
}


bool Dialog::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->gfxPort) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            //handleKeyPress(keyEvent->key());
            if (!keyEvent->isAutoRepeat()) { // only handle first press
            }
            return true; // stop further processing
        }
        if (event->type() == QEvent::KeyRelease) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            //handleKeyRelease(keyEvent->key());
            if (!keyEvent->isAutoRepeat()) { // only handle first press
            }
            return true; // stop further processing
        }
    }
    return QDialog::eventFilter(obj, event);
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


    //int currentY = ui->gfxPort->y();   // get current Y position
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

void Dialog::closeEvent(QCloseEvent *event){

}



Dialog::~Dialog()
{
    delete ui;
}

/////////// ------------------------------- SID BOXY STUFF ----------------------------------------------------------------------------

char strText[128];

void Dialog::swapBuffers() {
    std::swap(screenImageF, screenImageB);
    pixmapItem->setPixmap(QPixmap::fromImage(screenImageF));
}

void Dialog::updateGFXScreen(){
    // ------- TRANSFER VRAM to IMAGE output ----------- //
    // simulates the SIDBOX Graphics view LCD
    uint8_t *p = PROJ_VRAM;

    for (int x = 0; x < SCR_WIDTH; x++){
        for (int y = 0; y < SCR_HEIGHT; y++){
            QRgb *scan = reinterpret_cast<QRgb*>(screenImageB.scanLine(y));
            scan[x] = PROJ_CRAM[*p++];
        }
    }
    sbgfx_drawbox(476, 0, 479, 320, 63);
    sbgfx_drawbox(0, 319, 479, 319, 63);

    swapBuffers();
    //processAudio();
    //printf("piss\n");
}


void Dialog::clearSMSScreen(){
    sbgfx_fill(0);
    uint8_t *p = PROJ_VRAM;

    for (int x = 0; x < SCR_WIDTH; x++){
        for (int y = 0; y < SCR_HEIGHT; y++){
            QRgb *scan = reinterpret_cast<QRgb*>(screenImageB.scanLine(y));
            scan[x] = PROJ_CRAM[*p++];
        }
    }
    swapBuffers();
}
