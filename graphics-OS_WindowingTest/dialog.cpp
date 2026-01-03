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
#include "codergirl/cg_windowex.h"
#include "codergirl/cg_input.h"

#include "fastram.h"
#include "codergirl/cg_msghandler.h"
#include "codergirl/cg_resources.h"
#include "codergirl/cg_glyphs.h"

#include "codergirl/cg_gad_listbox.h"
#include "codergirl/cg_itemlist.h"
#include "codergirl/cg_gad_button.h"
//#include "codergirl/cg_gad_canvas.h"
#include "codergirl/cg_gad_scrollbar.h"
#include "codergirl/cg_gad_progbar.h"
#include "codergirl/cg_gad_label.h"
#include "codergirl/cg_gad_gridselect.h"

#include "codergirl/lib_filerequest.h"
#include "codergirl/lib_msgbox.h"

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



/*
////////////////////////////////////////////////////////////////////////////////////////////////
//  DESKTOP TEST - START  //////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
*/

static ItemLists_t listbox;
static ItemLists_t demolist;

static CGGadgetHandle    theListBox;
static SBXWindowId      titleBar;

static CGGadgetHandle   MenuBarTitle;

static CGGadgetHandle   btnCloseMe;
static CGGadgetHandle   btnAboutUs;

static CGGadgetHandle   btnFileRequest;
static CGGadgetHandle   btnMessageTest;

// a CUSTOM function for a scroll bar
void BitDemoScrolly1(void *s){
    GAD_SCROLLBAR_T *sb = (GAD_SCROLLBAR_T*)s;  // cast FIRST
    if (!sb) return;

    printf("Custom Function!: %d\n", sb->value);
}


// a CUSTOM function for a scroll bar that will interact with a listbox
void ListBoxTopSet(void *s){
    GAD_SCROLLBAR_T *src_sb = (GAD_SCROLLBAR_T*)s;
    if (!src_sb) return;
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(theListBox);
    if (!g) return;
    if (!g->gadget) return;
    SBOS_setListbox_top(g, src_sb->value);
}

void UpdateMenuLabel(void *s){
    char txt[32];
    // source gadget expected
    GAD_SCROLLBAR_T *src_sb = (GAD_SCROLLBAR_T*)s;
    if (!src_sb) return;
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(MenuBarTitle);
    if (!g) return;
    if (!g->gadget) return;
    //SBOS_setListbox_top(g, src_sb->value);

    sprintf(txt, "scr: %d", src_sb->value);
    SBOS_setLabelText(MenuBarTitle, txt);
}

void doCreateAWindow(void *s){
    SBXWindowId newWindowThing = SBOS_createWindow(&newWindowThing, 20, 30, 360, 380, "Gridselect is a palette!", SBX_WF_DOCKBOTTOM | SBX_WF_RESIZABLE | SBX_WF_SCREENBOUND | WIN_DEFAULT_FLAGS);
    if(newWindowThing == SBW_INVALID_ID){
        printf("Think we ran out of windows -- how about closing some!\n");
        return;
    }

    CGGadgetHandle grid1 = SBOS_CreateGridSelect(newWindowThing, 10,10, 20, 20, 16, 16, GAD_GRIDSEL_JUST_ONE | GAD_GRIDSEL_TEXT_INVERT, GAD_TOOL_DEFAULT);

    //SBOS_enableGadget(grid1, 0);
    char txt[4];
    for(int i = 0; i < 256; i++){
        SBOS_setCellColour(grid1, i, i);


        sprintf(txt, "%02X", i);
        SBOS_setCellText(grid1, txt, i);
    }




    SBOS_setFocus(newWindowThing);
}

SBXWindowId aboutWin;

void closeAboutWindow(void *){
    printf("Close the about window? HOW DARE YOU!\n");
    SBOS_destroyWindow(aboutWin);
    aboutWin = 0;
}

#define ABOUTWIN_WIDTH  280
#define ABOUTWIN_HEIGHT 200
#define ABOUTWIN_TEXTWIDTH  (ABOUTWIN_WIDTH - 30)
#define ABOUTWIN_TEXTHEIGHT (ABOUTWIN_HEIGHT - 70)
#define SYS_MENU_BAR_HEIGHT     20
void doAboutWindow(void *s){
    if(aboutWin) {
        // its already loaded, just bring it to the front and focus
        SBOS_setFocus(aboutWin);
        SBOS_bringToFront(aboutWin);
        return;
    }

    int32_t wx, wy;

    wx = (SCR_WIDTH / 2) - (ABOUTWIN_WIDTH /2);
    wy = (SCR_HEIGHT / 4) - (ABOUTWIN_HEIGHT /4);

    aboutWin = SBOS_createWindow(&aboutWin, wx, wy, ABOUTWIN_WIDTH, ABOUTWIN_HEIGHT, "SIDBOX OS!", SBX_WF_SCREENBOUND | WIN_DEFAULT_FLAGS);
    SBOS_setWindowBackColour(aboutWin, 6);


    if(aboutWin == SBW_INVALID_ID){
        printf("Think we ran out of windows -- how about closing some!\n");
        return;
    }

    //SBOS_CreateBitmapView(aboutWin, 0, 0, ABOUTWIN_WIDTH, ABOUTWIN_HEIGHT, baseGrid, 32, 32, 32, BVF_WRAP | BVF_SRC_ROWMAJOR, GAD_TOOL_NOBORDER);
    CGGadgetHandle lbl = SBOS_CreateLabel(aboutWin, 10,10, ABOUTWIN_TEXTWIDTH, ABOUTWIN_TEXTHEIGHT, "", GAD_TOOL_INSET);
    SBOS_setGadgetBPen(lbl, PEN_WIN_BG);
    CGGadgetHandle txt = SBOS_CreateLabel(aboutWin, 14,14, ABOUTWIN_TEXTWIDTH-10, 32,
                                          "SIDBOX OS - Version 1.0\n"
                                          "Window Manager: \"" UI_NAME "\"\n"
                                          , GAD_TOOL_DEFAULT);

    SBOS_setGadgetBPen(txt, PEN_WIN_BG);

    txt = SBOS_CreateLabel(aboutWin, 14, 54, ABOUTWIN_TEXTWIDTH-10, 64,
                           "Resource Pool System v0.1\n"
                           "Built by Electronscape\n\n"
                           "Licensing pending\n"
                           "\xa9 2025 - 2099"
                           , GAD_TOOL_DEFAULT);

    SBOS_setGadgetBPen(txt, PEN_WIN_BG);

    int16_t PerfX = (ABOUTWIN_WIDTH/2) - 35;
    CGGadgetHandle closeBtn = SBOS_CreateButton(aboutWin, PerfX,  ABOUTWIN_HEIGHT - 55,  70, 26, "CLOSE", GAD_TOOL_DEFAULT);//GAD_TOOL_DOCKED_RIGHT

    //SBOS_setLabelColour(txt, 2, BPEN_NOCHANGE);
    SBOS_setButtonCallBack(closeBtn, closeAboutWindow);

    SBOS_setFocus(aboutWin);
    SBOS_bringToFront(aboutWin);
    printf("ABOUT WINDOW  ");
}

////////// this section would be in our client software anyway so treat it as such -------------

static SBXWindowId g_filerqWin = SBW_INVALID_ID;
static CGWindowProcRes BasicWindowMAIN(SBXWindowId win, const CGMessage_t *m)
{
    if (!m) return(CGPROC_DEFAULT);

    //printf("btnAboutUs: %lu\n", btnAboutUs);
    //printf("EVENT : message_type %lu, class:%lu, gadget_id:%lu\n", m->mtype, m->eventClass, m->gadget);

    switch (m->mtype) {
        case CGMSG_GADGET:
            switch (m->eventClass) {
                /// FILE REQUEST DEMO ///////////////////////////////////////////////////////////////////
                case CGEVT_GAD_BUTTON_HIT:
                    // Identify which button fired (by gadget handle)
                    if (m->gadget == btnCloseMe)
                        SBOS_destroyWindow(win);

                    if(m->gadget == btnAboutUs)
                        doAboutWindow(NULL);

                    if(m->gadget == btnMessageTest){
                        /*
                            MSGBOXF_OK          = 0x00,   // OK only
                            MSGBOXF_OKCANCEL    = 0x01,   // OK + Cancel
                            MSGBOXF_YESNO       = 0x02,   // Yes + No
                            MSGBOXF_YESNOCANCEL = 0x03    // Yes + No + Cancel (optional)
                        */

                        SBOS_MessageBoxSimple(win, "test message",
                                              "hello this is a text box!\n"
                                              "testing next line!",
                                              MSGBOXF_YESNOCANCEL);
                        /*
                        SBOS_MessageBoxSimple(win, "test message",
                                              "\ngogk\n ",
                                              MSGBOXF_YESNOCANCEL);
                        */
                    }

                    if(m->gadget == btnFileRequest){

                        static char g_filebuf[256];

                        CGFileRqParams p = {0};
                        p.title = "Open File";
                        p.initial_dir = "/music";
                        p.out_path = g_filebuf;
                        p.out_cap  = (int32_t)sizeof(g_filebuf);
                        p.user     = (void*)0xC0FFEE;   // test cookie (optional)

                        g_filebuf[0] = '\0';

                        g_filerqWin = SBOS_OpenFileRequester(win, &p);

                        SBOS_enableGadget(btnFileRequest, 0);// disable for now!

                        printf("[MAIN] OpenFileRequester -> win=%u\n", (unsigned)g_filerqWin);
                    }
                    break;
                /////////////////////////////////////////////////////////////////////////////////////////

                case CGEVT_GAD_SCROLL_CHANGED:
                    printf("SCROLL: %d\n", m->a);
                    break;

                case CGEVT_GAD_CHECK_CHANGED:
                    printf("CHECK BOX: %d\n", m->a);
                    break;

                case CGEVT_GAD_RADIO_CHANGED:
                    printf("RADIO checked %d, grp:%d, gad_id:%d\n", m->a, m->b, m->gadget);
                    break;
                default:
                    break;
                }
                break;

        case CGMSG_WINDOW:
            // e.g. resize/minimize events later
            switch(m->eventClass){
                case CGEVT_WIN_RESIZED:
                    //printf("WINDOW RESIZE\n");
                break;
                case CGEVT_WIN_RESIZE:
                    //printf("WINDOW RESIZING\n");
                    break;

                case CGEVT_WIN_MOVED:
                    //printf("WINDOW MOVED\n");
                    break;

                case CGEVT_WIN_MOVE:
                    //printf("WINDOW MOVING...\n");
                    break;

                case CGEVT_WIN_CLOSE_REQUEST:
                    //SBOS_destroyWindow(win);
                    return(CGPROC_NORMAL); // PREVENT window from closing!
                    break;


                /// FILE REQUEST DEMO ///////////////////////////////////////////////////////////////////
                case CGEVT_SYS_FILERQ_DONE: {
                    int ok = (int)m->a;
                    void *cookie = MSG_AS_PTR(void, m->b);
                    char *outp = MSG_AS_PTR(char, m->c);
                    SBXWindowId rq = (SBXWindowId)m->d;

                    SBOS_enableGadget(btnFileRequest, 1);// re-enable

                    printf("[MAIN] FILERQ_DONE ok=%d rq=%u cookie=%p path='%s'\n",
                           ok, (unsigned)rq, cookie, outp ? outp : "(null)");

                    // optional: track/clear
                    if (rq == g_filerqWin) g_filerqWin = SBW_INVALID_ID;
                } break;

                /// MESSAGEBOX RESULTS //////////////////////////////////////////////////////////////////
                case CGEVT_SYS_MSGBOX_DONE: {
                    int choice = (int)m->a;
                    void *cookie = MSG_AS_PTR(void, m->b);
                    SBXWindowId msgboxWin = (SBXWindowId)m->d;

                    printf("MsgBox done: choice=%d win=%u cookie=%p\n", choice, msgboxWin, cookie);
                } break;
                /////////////////////////////////////////////////////////////////////////////////////////

                default: break;
            }


            break;

        default:
            break;
    }


    return(CGPROC_COMPLETE);
}

SBXWindowId winMain;
void createBasicDesktopTest(){
    initWb();
    initFastRam();  // REALLY IMPORTANT BEFORE LAUNCHING ANYTHING NEEDING LIST!

    // DESKTOP WINDOW (yes it IS a window)
    SBXWindowId workbench = SBOS_createWindow(&workbench, 0, 0, SCR_WIDTH, SCR_HEIGHT, "Workbench", SBX_WF_ALWAYS_TO_BACK | SBX_WF_VISIBLE | SBX_WF_NOBORDER);
    SBOS_CreateBitmapView(workbench, 0, 0, SCR_WIDTH, SCR_HEIGHT, backdrop, 480, 320, 480, BVF_WRAP | BVF_SRC_ROWMAJOR, GAD_TOOL_NOBORDER);
    CGGadgetHandle newWindowBtn = SBOS_CreateButton(workbench, 6,  60,  170, 26, "a workbench button", GAD_TOOL_DEFAULT);//GAD_TOOL_DOCKED_RIGHT
    SBOS_setButtonCallBack(newWindowBtn, doCreateAWindow);

    titleBar = SBOS_createWindow(&titleBar, 0, 0, SCR_WIDTH, SYS_MENU_BAR_HEIGHT, "MenuSystem", SBX_WF_NOBORDER | SBX_WF_VISIBLE | SBX_WF_NOAUTOZORDER);
    SBOS_setWindowBackColour(titleBar, 2);
    SBOS_CreateCanvas(titleBar, 0, SYS_MENU_BAR_HEIGHT-1, SCR_WIDTH, 0, CNV_LINE,  GAD_TOOL_DEFAULT);

    SBOS_CreateLabel(titleBar, 5, 2, 100, 16, "SIDBOX DESKTOP V1.0", GAD_TOOL_DEFAULT);
    MenuBarTitle = SBOS_CreateLabel(titleBar, 185, 2, 100, 16, "hello world", GAD_TOOL_DEFAULT);
    SBOS_setLabelText(MenuBarTitle, "Initialisting...");
    //SBOS_setGadgetBPen(MenuBarTitle, 7);

    // WINDOW 1 -----------------------
    // cycle button test window
    SBXWindowId winMain =  SBOS_createWindow(0, 80, 70, 260, 280, "Sharks with lasers!", SBX_WF_CLOSE | SBX_WF_ZORDER | SBX_WF_MOVEABLE | SBX_WF_TITLE_BAR | SBX_WF_RESIZABLE | SBX_WF_VISIBLE );
    //SBOS_CreateScrollbar(winMain,   0,0,0,0,           SB_ORIENT_VERT,  0, 100, 1,  25,  0, GAD_TOOL_DOCKED_RIGHT, 0);
    SBOS_CreateButton(winMain, 6,  6,  200, 26, "a workbench button|desktop icons|where are my lasers?|ok gerbils instead!", GAD_TOOL_CYCLEBUTTON);//GAD_TOOL_DOCKED_RIGHT



    // WINDOW 2 -----------------------
    SBXWindowId winMain2 = SBOS_createWindow(&winMain2, 100, 100, 340, 260, "Adjustable Drawer window", SBX_WF_DOCKBOTTOM | SBX_WF_RESIZABLE | SBX_WF_SCREENBOUND | WIN_DEFAULT_FLAGS);
    btnAboutUs = SBOS_CreateButton(winMain2, 6,  6,  170, 26, "About us event post", GAD_TOOL_DEFAULT);//GAD_TOOL_DOCKED_RIGHT
    SBOS_setWindowProc(winMain2, BasicWindowMAIN);
    btnMessageTest = SBOS_CreateButton(winMain2, 180,  70,  70, 26, "demo", GAD_TOOL_DEFAULT);//GAD_TOOL_DOCKED_RIGHT
    //SBOS_enableGadget(btn1, 0);// disable the button

    btnCloseMe = SBOS_CreateButton(winMain2, 120, 100, 50, 24, "CLOSE", GAD_TOOL_DEFAULT);

    btnFileRequest = SBOS_CreateButton(winMain2, 10, 160, 150, 24, "FILE_REQ", GAD_TOOL_DEFAULT);

    CGGadgetHandle pb1 = SBOS_CreateProgBar(winMain2, 10, 190, 150, 26, GAD_TOOL_DEFAULT);
    SBOS_setProgBarMinMax(pb1, 0, 100);
    SBOS_setProgBarValue(pb1, 75);

    CGGadgetHandle chk1 = SBOS_CreateCheckbox(winMain2, 10, 45, 160, 24, "Enable lasers", 0, GAD_TOOL_DEFAULT);
    SBOS_enableGadget(chk1, 0);
    SBOS_CreateCheckbox(winMain2, 10, 65, 160, 24, "Enable gerbils", 0, GAD_TOOL_DEFAULT);
    CGGadgetHandle rad2 =
    SBOS_CreateRadioButton(winMain2, 180, 10,  80, 18, "Easy",   0, 1, GAD_TOOL_DEFAULT);
    SBOS_enableGadget(rad2, 0);
    SBOS_CreateRadioButton(winMain2, 180, 30,  80, 18, "Medium", 0, 0, GAD_TOOL_DEFAULT);
    SBOS_CreateRadioButton(winMain2, 180, 50,  80, 18, "Hard",   0, 0, GAD_TOOL_DEFAULT);
    SBOS_CreateRadioButton(winMain2, 180, 100, 80, 18, "PAL",    1, 1, GAD_TOOL_DEFAULT);
    SBOS_CreateRadioButton(winMain2, 180, 120, 80, 18, "NTSC",   1, 0, GAD_TOOL_DEFAULT);
    SBOS_CreateScrollbar(winMain2,
                         10, 140, 150, 16,
                         SB_ORIENT_HORZ,
                         0, 100,
                         50,  250,
                         GAD_TOOL_SCROLLARROWS);

    CGGadgetHandle aboutBtn = SBOS_CreateButton(winMain2, 10, 100, 100, 25, "About...", GAD_TOOL_DEFAULT);
    SBOS_setButtonCallBack(aboutBtn, doAboutWindow);


    CGGadgetHandle scrrb =
    SBOS_CreateScrollbar(winMain2,
                        280, 6, 20, 150,
                        SB_ORIENT_VERT,
                        0, 120,
                        5,  50,
                        GAD_TOOL_SCROLLARROWS | GAD_TOOL_DEFAULT);
    SBOS_setScrollBarCallBack(scrrb, &BitDemoScrolly1); // attach this scrollbar to the function
    SBOS_CreateScrollbar(winMain2,
                         0,0,0,0,
                         SB_ORIENT_HORZ,
                         0, 100,
                         1,  50,
                         GAD_TOOL_SCROLLARROWS | GAD_TOOL_DOCKED_BOTTOM);

    CGGadgetHandle winScroll =
    SBOS_CreateScrollbar(winMain2,
                        0,0,0,0,
                        SB_ORIENT_VERT,
                        0, 100,
                        1,  25,
                        GAD_TOOL_SCROLLARROWS | GAD_TOOL_DOCKED_RIGHT);

    printf("winScroll: %lu\n", winScroll);


    //SBOS_setScrollBarCallBack(winScroll, &UpdateMenuLabel); // attach this scrollbar to the function






    // WINDOW 3 -----------------------
    // a bitmap viewable window ;) lets see if this works!!
    SBXWindowId winMain3 = SBOS_createWindow(&winMain3, 40, 40, 320, 200, "TEST test #x/y \xff", SBX_WF_RESIZABLE | SBX_WF_MOVEABLE | SBX_WF_VISIBLE | SBX_WF_TITLE_BAR | SBX_WF_ZORDER );
    CGGadgetHandle piccy1 = SBOS_CreateBitmapView(winMain3, 0, 0, 200, 200, testpix, 480, 320, 480, BVF_PAN | BVF_SHOW_FRAME | BVF_SRC_ROWMAJOR, GAD_TOOL_DEFAULT);
    //SBOS_enableGadget(piccy1, 0);

    SBOS_CreateButton(winMain3, 6,  6,  170, 26, "a simple text test", GAD_TOOL_INSET);//GAD_TOOL_DOCKED_RIGHT
    SBOS_CreateScrollbar(winMain3,
                         0,0,0,0,
                         SB_ORIENT_VERT,
                         0, 100,
                         1,  25,
                         GAD_TOOL_DOCKED_RIGHT);



    SBOS_setFocus(winMain2);
    SBOS_bringToFront(winMain2);

    // FAUX FAST RAM TESTING //
    char *txt5 = (char *)fastAlloc(200);
    if(txt5)// it was given a pointer so lets do this
        strcpy(txt5, "Hello world");


    int32_t isok;


    listitem_init(&listbox);

    isok = listitem_insert(&listbox, 0, "hello host LISTBOX one!");
    isok = listitem_add(&listbox, "random text 2");
    isok = listitem_add(&listbox, "CAMMELS!");
    isok = listitem_add(&listbox, "I still don't have my lasers!!");
    isok = listitem_add(&listbox, "GERBILS EVERYWHERE!!!! HELP!!!");

    listitem_dump(&listbox);
    listitem_delete(&listbox, 2);
    listitem_dump(&listbox);


    char txt[64];

    listitem_init(&demolist);
    for(int i = 0; i < 32; i++){
        sprintf(txt, "Listbox test %d", i);
        listitem_add(&demolist, txt);
    }


    theListBox = SBOS_CreateListBox(winMain, 6, 40, 200, 196, &demolist, GAD_TOOL_DEFAULT);



    CGGadgetHandle ListBoxScroll =
    SBOS_CreateScrollbar(winMain,
                                                        206, 40, 16, 196,
                                                        SB_ORIENT_VERT,
                                                        0, 31 - 11,
                                                        1,  5,
                                                        GAD_TOOL_SCROLLARROWS | GAD_TOOL_DEFAULT);
    SBOS_setScrollBarCallBack(ListBoxScroll, &ListBoxTopSet);

    //SBOS_enableGadget(theListBox, 0);
    //SBOS_enableGadget(ListBoxScroll, 0);

    printf("theListBox handle = %u (0x%08x)\n",    (uint32_t)theListBox,    (uint32_t)theListBox);
    printf("ListBoxScroll handle = %u (0x%08x)\n", (uint32_t)ListBoxScroll, (uint32_t)ListBoxScroll);


    //fastDumpHex(0x800);
    //fastDump();

    char *text;

    //SBOS_print_ui_usage();
    printf("In memory: %s\n", txt5);



    //listitem_free(&listbox);
    //listitem_free(&demolist);

    SBOS_paintAllWindows();
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////
//  DESKTOP TEST - END  ////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
*/












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
        pixmapItem->setTransformationMode(Qt::FastTransformation);
    });

    connect(ui->cmdZoom2x, &QPushButton::clicked, this, [=](){
        setScreenScale(2);
        pixmapItem->setTransformationMode(Qt::FastTransformation);
    });

    connect(ui->cmdZoomWx, &QPushButton::clicked, this, [=](){
        setScreenScale(winScale);
        pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    });

    setScreenScale(1);


    // Resource Memory Monitor Heart beat
    QTimer *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [=](){
        uint32_t chipRes, fastRes;
        getMemAvailChipNFast(&chipRes, &fastRes);

        char chipBuf[16];
        char fastBuf[16];
        char ramText[64];

        fmt_commas_u32(chipBuf, chipRes);
        fmt_commas_u32(fastBuf, fastRes);

        sprintf(ramText, "CHIP: %s  FAST: %s", chipBuf, fastBuf);
        SBOS_setLabelText(MenuBarTitle, ramText);

        SBOS_paintAllWindows();
        updateGFXScreen();
    });

    QTimer *osMessageHandlerTMR = new QTimer(this);
    connect(osMessageHandlerTMR, &QTimer::timeout, this, [=](){
        if(cg_os_messagehandler(4)){
            SBOS_paintAllWindows();
            updateGFXScreen();
        }
        ;    // one message at a time, FOR now
    });



    timer = new QTimer(this);
    QTimer::singleShot(500, this, [=]() {

    });

    QTimer *frameTimer = new QTimer(this);
    connect(frameTimer, &QTimer::timeout, this, [=]() {

    });
    frameTimer->start(16);   // 0ms = run every cycle

    sbgfx_fill(5);

    //sbgfx_drawbox(00,0,320,256, 3);



    createBasicDesktopTest();
    updateGFXScreen();


    t->start(1500);   // every 22ms/ about 60hz?
    osMessageHandlerTMR->start(8);// 1 second so we can see things in the queue
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
