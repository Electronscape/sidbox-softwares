#include "dialog.h"
#include "ui_dialog.h"
#include <QTimer>
#include <QFileDialog>
#include <QString>
#include <QMessageBox>
#include <QSettings>
#include <QMouseEvent>
#include <QPoint>  // for the QPoint objects

#include "sbapi_graphics.h"
#include "sbx_windowex.h"
#include "sbx_input.h"


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

    ui->gfxPort->setRenderHint(QPainter::SmoothPixmapTransform, false);
    ui->gfxPort->setRenderHint(QPainter::Antialiasing, false); // don't need antialias for pixels

    //ui->gfxPort->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    //ui->gfxPort->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    ui->gfxPort->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->gfxPort->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);


    ui->gfxPort->setMouseTracking(true);
    ui->gfxPort->viewport()->setMouseTracking(true); // important for QGraphicsView
    ui->gfxPort->installEventFilter(this);
    ui->gfxPort->viewport()->installEventFilter(this);
    ui->gfxPort->setFocusPolicy(Qt::StrongFocus);
    ui->gfxPort->setFocus();


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


    QTimer *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [=](){


        windowingTest();


    });
    t->start(22);   // every 22ms/ about 60hz?


    timer = new QTimer(this);
    QTimer::singleShot(500, this, [=]() {

    });

    QTimer *frameTimer = new QTimer(this);
    connect(frameTimer, &QTimer::timeout, this, [=]() {

    });
    frameTimer->start(16);   // 0ms = run every cycle

    sbgfx_fill(5);

    //sbgfx_drawbox(00,0,320,256, 3);

    initWb();
    //uint32_t winID = createWindow(320, 256, (char *)"Test Window V1.0 - yey");
    SBXWindowId workbench = SBOS_createWindow(0, 0, SCR_WIDTH, SCR_HEIGHT, "Workbench", SBX_WF_ALWAYS_TO_BACK | SBX_WF_VISIBLE | SBX_WF_NOBORDER);
    SBXWindowId winMain =  SBOS_createWindow(10, 20, 320, 200, "NoBorder 1", SBX_WF_RESIZABLE | SBX_WF_VISIBLE );
    //SBWindowId winMain1 = SBOS_createWindow(340, 10, 100, 250, "Main", SBW_VISIBLE | SBW_NOBORDER);
    SBXWindowId winMain3 = SBOS_createWindow(40, 30, 320, 200, "TEST test #x/y \xff", SBX_WF_MOVEABLE | SBX_WF_VISIBLE | SBX_WF_TITLE_BAR | SBX_WF_ZORDER );
    SBXWindowId winMain2 = SBOS_createWindow(100, 100, 220, 100, "Adjustable Drawer window", SBX_WF_RESIZABLE | SBX_WF_SCREENBOUND | WIN_DEFAULT_FLAGS);
    if (winMain2 == SBW_INVALID_ID) {
        // no free window slots — OS politely shrugs
        printf("No more windows left\n");
    } else
        printf("Window ID %d\n", winMain2);


    //sbx_window_t w = SBOS_getWindow(winMain2);

    SBOS_addButton(winMain2, 6,  6,  170, 26, "a simple text test", GAD_TOOL_DEFAULT);//GAD_TOOL_DOCKED_RIGHT
    SBOS_addButton(winMain2, -6,  66,  170, 26, "a simple text test", GAD_TOOL_DEFAULT);//GAD_TOOL_DOCKED_BOTTOM

/*
    sbx_window_t *w = SBOS_getWindow(workbench);
    SBControlHandle hWorkBench = SBOS_CreateButton(w, 6,  6,  220, 26, "Workbench button");
    SBControlHandle hBigButty  = SBOS_CreateButton(w, 300,  200,  180, 120, "BLOB TEST");



    w = SBOS_getWindow(winMain2);

    SBControlHandle hOk     = SBOS_CreateButton(w, 6,  6,  70, 26, "OK");
    SBControlHandle hCancel = SBOS_CreateButton(w, 80,  6,  70, 26, "Cancel");
    SBControlHandle hLong   = SBOS_CreateButton(w, 160, 6, 160, 26, "Long button name");
    SBControlHandle hLab    = SBOS_CreateLabel (w, 6, 36, "LABEL #1");

    SBControlHandle hV      = SBOS_CreateScrollbar(w, SBX_SB_VERT, SBX_DOCK_RIGHT,  20, 0, 100, 0, 20);
    SBControlHandle hH      = SBOS_CreateScrollbar(w, SBX_SB_HORZ, SBX_DOCK_BOTTOM, 20, 0, 100, 0, 20);

    SBControlHandle rad1    = SBOS_CreateRadioButton(w, 16, 80, 0, "Test1", 1);
    SBControlHandle rad2    = SBOS_CreateRadioButton(w, 16, 110, 0, "Test2", 0);
    SBControlHandle check1  = SBOS_CreateCheckbox(w, 100, 110, "SIDBOX OS!", 0);

    SBControlHandle hFree = SBOS_CreateScrollbar(w, SBX_SB_HORZ, SBX_DOCK_NONE, 20, 0, 100, 0, 1);

    // move by USER ID still works:
    SBOS_MoveScrollbar(w, hFree, 10, 60, 100, 16, SBX_SB_HORZ);
    printf("Scroll is: %hu\n", hFree);
    printf("Scroll Docked is: %hu\n", hV);

    // or better: move by handle (if you add it):
    // SBOX_MoveScrollbarH(w, hFree, 10, 60, 100, 16, SBX_SB_HORZ);
*/
    SBOS_setFocus(winMain2);
    SBOS_paintAllWindows();
    updateGFXScreen();
}

static bool bMouseDown = false;
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
            printf("key pressed\n");
            return true; // stop further processing
        }
    }
    if (obj == ui->gfxPort->viewport()){
        if(event->type() == QEvent::MouseButtonPress) {
            // will need these for the tool buttons that need the left and right bitsies
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePt = ui->gfxPort->mapToScene(mouseEvent->pos());
            int16_t mx = (int16_t)scenePt.x();
            int16_t my = (int16_t)scenePt.y();

            SBOS_MouseInterface(MOUSE_DOWN, mx, my);
            updateGFXScreen();
            //printf("mouse clicked_down x:%d, y:%d\n", mx, my);
            bMouseDown = true;
            return true; // stop further processing
        }
        if(event->type() == QEvent::MouseMove) {
            // will need these for the tool buttons that need the left and right bitsies
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePt = ui->gfxPort->mapToScene(mouseEvent->pos());
            int16_t mx = (int16_t)scenePt.x();
            int16_t my = (int16_t)scenePt.y();

            if(mx < 0) mx = 0;
            if(my < 0) my = 0;
            if(mx > SCR_WIDTH)  mx = SCR_WIDTH;
            if(my > SCR_HEIGHT) my = SCR_HEIGHT;


            SBOS_MouseInterface(MOUSE_MOVE, mx, my);
            updateGFXScreen();
            //printf("mouse move x:%d, y:%d\n", mx, my);
            //printf("mouse move view(%d,%d) scene(%.1f,%.1f)\n",                   viewPt.x(), viewPt.y(), scenePt.x(), scenePt.y());

            return true; // stop further processing
        }
        if(event->type() == QEvent::MouseButtonRelease) {
            // will need these for the tool buttons that need the left and right bitsies
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePt = ui->gfxPort->mapToScene(mouseEvent->pos());
            int16_t mx = (int16_t)scenePt.x();
            int16_t my = (int16_t)scenePt.y();

            SBOS_MouseInterface(MOUSE_UP, mx, my);
            updateGFXScreen();
            //printf("mouse clicked_release x:%d, y:%d\n", mx, my);
            bMouseDown = false;
            return true; // stop further processing
        }
        if(event->type() == QEvent::MouseButtonDblClick) {
            // will need these for the tool buttons that need the left and right bitsies
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePt = ui->gfxPort->mapToScene(mouseEvent->pos());
            int16_t mx = (int16_t)scenePt.x();
            int16_t my = (int16_t)scenePt.y();

            updateGFXScreen();
            //printf("mouse double clicked x:%d, y:%d\n", mx, my);
            bMouseDown = false;
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
    //sbgfx_drawbox(476, 0, 479, 320, 63);
    //sbgfx_drawbox(0, 319, 479, 319, 63);

    swapBuffers();
    //processAudio();
    //printf("piss\n");
}


void Dialog::clearSMSScreen(){
    sbgfx_fill(5);
    uint8_t *p = PROJ_VRAM;

    for (int x = 0; x < SCR_WIDTH; x++){
        for (int y = 0; y < SCR_HEIGHT; y++){
            QRgb *scan = reinterpret_cast<QRgb*>(screenImageB.scanLine(y));
            scan[x] = PROJ_CRAM[*p++];
        }
    }
    swapBuffers();
}








/////////////////////////////
void Dialog::windowingTest(){
    //clearSMSScreen();
    //paintAllWindows();
    //updateGFXScreen();
}
