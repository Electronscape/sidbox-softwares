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

#include "fastram.h"
#include "sbos_itemlist.h"



float winScale = 1.0f;
int currentScale = 1;

void prepXYs();

Dialog *g_dialog = nullptr;
char frame_db = 0;
char EmuReady = 0;

extern uint8_t testpix[];
extern uint8_t backdrop[];

void sms_keydown(int keycode);
void sms_keyup(int keycode);

void SBOS_print_ui_usage(void);

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

    // DESKTOP WINDOW (yes it IS a window)
    SBXWindowId workbench = SBOS_createWindow(0, 0, SCR_WIDTH, SCR_HEIGHT, "Workbench", SBX_WF_ALWAYS_TO_BACK | SBX_WF_VISIBLE | SBX_WF_NOBORDER);
    SBOS_addBitmapView(workbench, 0, 0, 480, 320, backdrop, 480, 320, 480, BVF_SRC_ROWMAJOR, GAD_TOOL_DEFAULT);



    SBXWindowId winMain =  SBOS_createWindow(10, 20, 320, 200, "Sharks with lasers!", SBX_WF_CLOSE | SBX_WF_ZORDER | SBX_WF_MOVEABLE | SBX_WF_TITLE_BAR | SBX_WF_RESIZABLE | SBX_WF_VISIBLE );

    // a bitmap viewable window ;) lets see if this works!!
    SBXWindowId winMain3 = SBOS_createWindow(40, 30, 320, 200, "TEST test #x/y \xff", SBX_WF_RESIZABLE | SBX_WF_MOVEABLE | SBX_WF_VISIBLE | SBX_WF_TITLE_BAR | SBX_WF_ZORDER );


    SBOS_addBitmapView(winMain3, 0, 0, 200, 200, testpix, 480, 320, 480, BVF_PAN | BVF_SHOW_FRAME | BVF_SRC_ROWMAJOR, GAD_TOOL_DEFAULT);

    SBOS_addButton(winMain3, 6,  6,  170, 26, "a simple text test", GAD_TOOL_DEFAULT);//GAD_TOOL_DOCKED_RIGHT


    SBXWindowId winMain2 = SBOS_createWindow(100, 100, 310, 200, "Adjustable Drawer window", SBX_WF_DOCKBOTTOM | SBX_WF_RESIZABLE | SBX_WF_SCREENBOUND | WIN_DEFAULT_FLAGS);
    if (winMain2 == SBW_INVALID_ID) {
        printf("No more windows left\n");
    } else
        printf("Window ID %d\n", winMain2);


    SBOS_addButton(workbench, 6,  6,  170, 26, "a workbench button", GAD_TOOL_DEFAULT);//GAD_TOOL_DOCKED_RIGHT

    SBOS_addButton(winMain, 6,  6,  200, 26, "a workbench button|desktop icons|where are my lasers?|ok gerbils instead!", GAD_TOOL_CYCLEBUTTON);//GAD_TOOL_DOCKED_RIGHT

    SBOS_addButton(winMain2, 6,  6,  170, 26, "a simple text test", GAD_TOOL_DEFAULT);//GAD_TOOL_DOCKED_RIGHT
    SBOS_addCheckbox(winMain2, 10, 45, 160, 24, "Enable lasers", 0, GAD_TOOL_DEFAULT);
    SBOS_addCheckbox(winMain2, 10, 65, 160, 24, "Enable gerbils", 0, GAD_TOOL_DEFAULT);


    SBOS_addRadioButton(winMain2, 180, 10, 100, 18, "Easy",   0, 1, GAD_TOOL_DEFAULT);
    SBOS_addRadioButton(winMain2, 180, 30, 100, 18, "Medium", 0, 0, GAD_TOOL_DEFAULT);
    SBOS_addRadioButton(winMain2, 180, 50, 100, 18, "Hard",   0, 0, GAD_TOOL_DEFAULT);

    SBOS_addRadioButton(winMain2, 180, 100, 100, 18, "PAL",    1, 1, GAD_TOOL_DEFAULT);
    SBOS_addRadioButton(winMain2, 180, 120, 100, 18, "NTSC",   1, 0, GAD_TOOL_DEFAULT);

    SBOS_addScrollbar(winMain2,
                      10, 120, 150, 16,
                      SB_ORIENT_HORZ,
                      0, 1000,
                      250,
                      0,
                      GAD_TOOL_SCROLLARROWS);

    SBOS_addScrollbar(winMain2,
                      260, 6, 20, 150,
                      SB_ORIENT_VERT,
                      0, 150,
                      50,
                      0,
                      GAD_TOOL_SCROLLARROWS | GAD_TOOL_DEFAULT);



    SBOS_addScrollbar(winMain2,
                      0,0,0,0,                // ignored if docked
                      SB_ORIENT_HORZ,
                      0, 100,                // “meaning range” just for thumb sizing + app mapping
                      25,                     // step in value units (thumb size + step pct)
                      0,                      // initial percent
                      GAD_TOOL_SCROLLARROWS | GAD_TOOL_DOCKED_BOTTOM);

    SBOS_addScrollbar(winMain2,
                      0,0,0,0,                // ignored if docked
                      SB_ORIENT_VERT,
                      0, 100,                // “meaning range” just for thumb sizing + app mapping
                      25,                     // step in value units (thumb size + step pct)
                      0,                      // initial percent
                      GAD_TOOL_SCROLLARROWS | GAD_TOOL_DOCKED_RIGHT);


    SBOS_addScrollbar(winMain3,
                      0,0,0,0,                // ignored if docked
                      SB_ORIENT_VERT,
                      0, 100,                // “meaning range” just for thumb sizing + app mapping
                      25,                     // step in value units (thumb size + step pct)
                      0,                      // initial percent
                      GAD_TOOL_DOCKED_RIGHT);

    SBOS_addScrollbar(winMain,
                      0,0,0,0,                // ignored if docked
                      SB_ORIENT_VERT,
                      0, 100,                // “meaning range” just for thumb sizing + app mapping
                      25,                     // step in value units (thumb size + step pct)
                      0,                      // initial percent
                          GAD_TOOL_DOCKED_RIGHT);


    SBOS_print_ui_usage();
    SBOS_setFocus(winMain2);
    SBOS_paintAllWindows();
    updateGFXScreen();


    // FAUX FAST RAM TESTING //

    initFastRam();


    char *txt5 = (char *)fastAlloc(512);
    strcpy(txt5, "Hello world");


    ItemLists_t listbox;

    listitem_init(&listbox);

    listitem_insert(&listbox, 0, "hello host LISTBOX one!");
    listitem_add(&listbox, "random text 2");
    listitem_add(&listbox, "CAMMELS!");
    listitem_add(&listbox, "I still don't have my lasers!!");
    listitem_add(&listbox, "GERBILS EVERYWHERE!!!! HELP!!!");


    listitem_dump(&listbox);

    listitem_delete(&listbox, 2);

    listitem_dump(&listbox);

    fastDump();

    fastDumpHex();

    printf("In memory: %s\n", txt5);

    listitem_free(&listbox);











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
        if(event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
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
            if(mx > SCR_WIDTH-1)  mx = SCR_WIDTH-1;
            if(my > SCR_HEIGHT-1) my = SCR_HEIGHT-1;


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
