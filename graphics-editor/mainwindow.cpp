#include "mainwindow.h"
#include "fonteditor.h"
#include "ui_mainwindow.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QThread> // for QThread::msleep
#include <QIcon>
#include <QImage>
#include <QMouseEvent>
#include <QPushButton>
#include <QTimer>
#include <QMessageBox>
#include <QSettings>
#include <QFileDialog>
//#include <functional>
//#include <cstring>
//#include <string.h>
#include <stack>
#include <QPoint>  // for the QPoint objects

#include <stdio.h>

#define PALETTE_BOX_HSIZE   16
#define PALETTE_BOX_VSIZE   16
#define PALETTE_WIDTH       PALETTE_BOX_HSIZE
#define PALETTE_HEIGHT      PALETTE_BOX_VSIZE

#define PALETTE_VRAM_SIZE   (PALETTE_WIDTH * PALETTE_BOX_HSIZE * PALETTE_HEIGHT * PALETTE_BOX_VSIZE)
int SelectedX = 0;    // this will be clickable later
int SelectedY = 0;
int colourCycleSpeed = 0;
uint8_t     numSelectedPaletteID = 0, numPrevSelectedPaletteID = 0;
int         paletteDepth    = 256;
uint8_t     pltColourPreset[3] = {0,0,0};

uint8_t     capturedPaletteIndex;
bool        bReassignedPaletteIndex = false;
bool        bSwapColours    = false;
bool        bSpreadPalette  = false;

QPalette pal;
QTimer *tmrColourCycle;
int cyclefrom, cycleto, cyclelength = 8;
bool selectingCycle = false;   // true when CTRL is held
bool waitingForEnd = false;    // true after choosing cycle_from
int clickedIndex = 256; // way off grid


// for previewing new primatives, lines, circles, rectangles
bool captureXYStart = false;
int ctcapturedX = -1, ctcapturedY = -1;  // current location target
int ltcapturedX = -1, ltcapturedY = -1;  // last location target
int capturedX = -1, capturedY = -1;

bool paletteRestrictor = false;
int paletteRangerOffset, paletteRangerLength;

int hoverPixelX = -1;
int hoverPixelY = -1;


bool gridEnabled        = false;
#define gridRed          128
#define gridGreen        128
#define gridBlue         128

// default settings at startup
uint16_t icon_zoom       = 25;
uint16_t icon_width      = 32;
uint16_t icon_height     = 32;

int editorViewPortWidth  = 8;   // editor width grid
int editorViewPortHeight = 8;

//uint8_t icon_area[8][8] = {0};  // all to paletteID 0
std::vector<std::vector<uint8_t>> icon_area;

// only rastering 8x8 pixel font, nothing advanced
uint8_t fontedit_area[8][8];


// prototype calls
void loadDefaultFont();


enum ImageExportConfig {
    ExportRLE           = 1,  // chkExportRLE
    ExportSidBoxVRAM    = 2,  // chkExportSBVRAM
};
uint16_t    ExportBits  = 0;    // just basic bits




uint32_t CLUT[256] = {
    0x00000000, 0xFFAFAFAF, 0xFFFFFFFF, 0xFF3B67A2, 0xFFAA907C, 0xFF959595, 0xFF7B7B7B, 0xFFFFA997,
    0xFF37A91D, 0xFF7CA9FF, 0xFFBF8112, 0xFFEBBF66, 0xFF78C178, 0xFF3D9318, 0xFFB33418, 0xFFD9311C,
    0xFF000000, 0xFF00000E, 0xFF00001D, 0xFF00002B, 0xFF000139, 0xFF000147, 0xFF000156, 0xFF000164,
    0xFF0001D2, 0xFF0001FF, 0xFFCECECE, 0xFF00FF00, 0xFFB2FF00, 0xFFFFE700, 0xFFFF9600, 0xFFFF1100,
    0xFF491200, 0xFF491355, 0xFF4914AA, 0xFF4916FF, 0xFF5B1700, 0xFF5B1855, 0xFF5B19AA, 0xFF5B1AFF,
    0xFF6D1B00, 0xFF6D1C55, 0xFF00E300, 0xFF85FF54, 0xFFC4FF00, 0xFFFFD900, 0xFFFFA41F, 0xFFE05400,
    0xFFFF0000, 0xFF922655, 0xFF9227AA, 0xFF9228FF, 0xFFA42900, 0xFFA42A55, 0xFFA42BAA, 0xFFA42CFF,
    0xFFB62D00, 0xFFB62F55, 0xFFB630AA, 0xFFB631FF, 0xFFC93200, 0xFFC93355, 0xFFC934AA, 0xFFC935FF,
    0xFFDB3700, 0xFFDB3855, 0xFFDB39AA, 0xFFDB3AFF, 0xFFED3B00, 0xFFED3C55, 0xFFED3DAA, 0xFFED3FFF,
    0xFFFF4000, 0xFFFF4155, 0xFFFF42AA, 0xFFFF43FF, 0xFF004400, 0xFF004555, 0xFF0046AA, 0xFF0048FF,
    0xFFFFFF00, 0xFF12FF55, 0xFF12EE55, 0xFF12B6FF, 0xFF001FFF, 0xFF9D0EC7, 0xFFF10000, 0xFFFF7700,
    0xFF375200, 0xFF375355, 0xFF3754AA, 0xFF3755FF, 0xFF495600, 0xFF495855, 0xFF4959AA, 0xFF495AFF,
    0xFF5B5B00, 0xFF5B5C55, 0xFF5B5DAA, 0xFF5B5EFF, 0xFF6D6000, 0xFF6D6155, 0xFF6D62AA, 0xFF6D63FF,
    0xFF6D6400, 0xFF806555, 0xFF8066AA, 0xFF8067FF, 0xFF926900, 0xFF926A55, 0xFF926BAA, 0xFF926CFF,
    0xFFA46D00, 0xFFA46E55, 0xFFA46FAA, 0xFFA471FF, 0xFFB67200, 0xFFB67355, 0xFFB674AA, 0xFFB675FF,
    0xFFC97600, 0xFFC97755, 0xFFC979AA, 0xFFC97AFF, 0xFFDB7B00, 0xFFDB7C55, 0xFFDB7DAA, 0xFFDB7EFF,
    0xFFED7F00, 0xFFED8055, 0xFFED82AA, 0xFFED83FF, 0xFFFF8400, 0xFFFF8555, 0xFFFF86AA, 0xFFFF87FF,
    0xFF008800, 0xFF008A55, 0xFF008BAA, 0xFF008CFF, 0xFF128D00, 0xFF128E55, 0xFF128FAA, 0xFF1290FF,
    0xFF249200, 0xFF249355, 0xFF2494AA, 0xFF2495FF, 0xFF379600, 0xFF379755, 0xFF3798AA, 0xFF3799FF,
    0xFF499B00, 0xFF499C55, 0xFF499DAA, 0xFF499EFF, 0xFF5B9F00, 0xFF5BA055, 0xFF5BA1AA, 0xFF5BA3FF,
    0xFFA4B5D5, 0xFFA0B0F8, 0xFF94A3E6, 0xFF7C89C1, 0xFF6281C0, 0xFF1C62A1, 0xFF4254EA, 0xFF62A1BD,
    0xFF7093C0, 0xFF4977A1, 0xFF003FAA, 0xFF1554FF, 0xFF1C50B9, 0xFF00B3FF, 0xFF0088AA, 0xFF00B5FF,
    0xFF0E62FF, 0xFF5EB7E3, 0xFFBDC0B9, 0xFF85B9FF, 0xFF006CAF, 0xFF1F81B9, 0xFF3F5BAA, 0xFFC9BEFF,
    0xFF5BAFCB, 0xFFDBC055, 0xFFDBC1AA, 0xFFBDC0C0, 0xFFEDC400, 0xFFEDC555, 0xFFEDC6AA, 0xFFEDC7FF,
    0xFFFFC800, 0xFFFFC955, 0xFFFFCAAA, 0xFFFFCCFF, 0xFF00CD00, 0xFF00CE55, 0xFF00CFAA, 0xFF00D0FF,
    0xFF12D100, 0xFF12D255, 0xFF12D3AA, 0xFF12D5FF, 0xFF24D600, 0xFF24D755, 0xFF24D8AA, 0xFF24D9FF,
    0xFF37DA00, 0xFF37DB55, 0xFF37DDAA, 0xFF37DEFF, 0xFF49DF00, 0xFF49E055, 0xFF49E1AA, 0xFF49E2FF,
    0xFF5BE300, 0xFF5BE555, 0xFF5BE6AA, 0xFF5BE7FF, 0xFF6DE800, 0xFF6DE955, 0xFF6DEAAA, 0xFF6DEBFF,
    0xFF6DEC00, 0xFF80EE55, 0xFF80EFAA, 0xFF80F0FF, 0xFF93CEA2, 0xFF92F255, 0xFF92F3AA, 0xFF92F4FF,
    0xFFA4F600, 0xFFA4F755, 0xFFA4F8AA, 0xFFA4F9FF, 0xFFB6FA00, 0xFFB6FB55, 0xFFB6FCAA, 0xFFB6FEFF,
    0xFFC9FF00, 0xFFC9FF55, 0xFFC9FFAA, 0xFFC9FFFF, 0xFFDBFF00, 0xFFDBFF55, 0xFFDBFFAA, 0xFFDBFFFF,
    0xFFEDFF00, 0xFFEDFF55, 0xFFEDFFAA, 0xFFEDFFFF, 0xFFFFFF00, 0xFFFFFF55, 0xFFFFFFAA, 0xFFFFFFFF,
};

const uint32_t DEFAULT_CLUT[256] = {
    0x00000000, 0xFFAFAFAF, 0xFFFFFFFF, 0xFF3B67A2, 0xFFAA907C, 0xFF959595, 0xFF7B7B7B, 0xFFFFA997,
    0xFF37A91D, 0xFF7CA9FF, 0xFFBF8112, 0xFFEBBF66, 0xFF78C178, 0xFF3D9318, 0xFFB33418, 0xFFD9311C,
    0xFF000000, 0xFF00000E, 0xFF00001D, 0xFF00002B, 0xFF000139, 0xFF000147, 0xFF000156, 0xFF000164,
    0xFF0001D2, 0xFF0001FF, 0xFFCECECE, 0xFF00FF00, 0xFFB2FF00, 0xFFFFE700, 0xFFFF9600, 0xFFFF1100,
    0xFF491200, 0xFF491355, 0xFF4914AA, 0xFF4916FF, 0xFF5B1700, 0xFF5B1855, 0xFF5B19AA, 0xFF5B1AFF,
    0xFF6D1B00, 0xFF6D1C55, 0xFF00E300, 0xFF85FF54, 0xFFC4FF00, 0xFFFFD900, 0xFFFFA41F, 0xFFE05400,
    0xFFFF0000, 0xFF922655, 0xFF9227AA, 0xFF9228FF, 0xFFA42900, 0xFFA42A55, 0xFFA42BAA, 0xFFA42CFF,
    0xFFB62D00, 0xFFB62F55, 0xFFB630AA, 0xFFB631FF, 0xFFC93200, 0xFFC93355, 0xFFC934AA, 0xFFC935FF,
    0xFFDB3700, 0xFFDB3855, 0xFFDB39AA, 0xFFDB3AFF, 0xFFED3B00, 0xFFED3C55, 0xFFED3DAA, 0xFFED3FFF,
    0xFFFF4000, 0xFFFF4155, 0xFFFF42AA, 0xFFFF43FF, 0xFF004400, 0xFF004555, 0xFF0046AA, 0xFF0048FF,
    0xFFFFFF00, 0xFF12FF55, 0xFF12EE55, 0xFF12B6FF, 0xFF001FFF, 0xFF9D0EC7, 0xFFF10000, 0xFFFF7700,
    0xFF375200, 0xFF375355, 0xFF3754AA, 0xFF3755FF, 0xFF495600, 0xFF495855, 0xFF4959AA, 0xFF495AFF,
    0xFF5B5B00, 0xFF5B5C55, 0xFF5B5DAA, 0xFF5B5EFF, 0xFF6D6000, 0xFF6D6155, 0xFF6D62AA, 0xFF6D63FF,
    0xFF6D6400, 0xFF806555, 0xFF8066AA, 0xFF8067FF, 0xFF926900, 0xFF926A55, 0xFF926BAA, 0xFF926CFF,
    0xFFA46D00, 0xFFA46E55, 0xFFA46FAA, 0xFFA471FF, 0xFFB67200, 0xFFB67355, 0xFFB674AA, 0xFFB675FF,
    0xFFC97600, 0xFFC97755, 0xFFC979AA, 0xFFC97AFF, 0xFFDB7B00, 0xFFDB7C55, 0xFFDB7DAA, 0xFFDB7EFF,
    0xFFED7F00, 0xFFED8055, 0xFFED82AA, 0xFFED83FF, 0xFFFF8400, 0xFFFF8555, 0xFFFF86AA, 0xFFFF87FF,
    0xFF008800, 0xFF008A55, 0xFF008BAA, 0xFF008CFF, 0xFF128D00, 0xFF128E55, 0xFF128FAA, 0xFF1290FF,
    0xFF249200, 0xFF249355, 0xFF2494AA, 0xFF2495FF, 0xFF379600, 0xFF379755, 0xFF3798AA, 0xFF3799FF,
    0xFF499B00, 0xFF499C55, 0xFF499DAA, 0xFF499EFF, 0xFF5B9F00, 0xFF5BA055, 0xFF5BA1AA, 0xFF5BA3FF,
    0xFFA4B5D5, 0xFFA0B0F8, 0xFF94A3E6, 0xFF7C89C1, 0xFF6281C0, 0xFF1C62A1, 0xFF4254EA, 0xFF62A1BD,
    0xFF7093C0, 0xFF4977A1, 0xFF003FAA, 0xFF1554FF, 0xFF1C50B9, 0xFF00B3FF, 0xFF0088AA, 0xFF00B5FF,
    0xFF0E62FF, 0xFF5EB7E3, 0xFFBDC0B9, 0xFF85B9FF, 0xFF006CAF, 0xFF1F81B9, 0xFF3F5BAA, 0xFFC9BEFF,
    0xFF5BAFCB, 0xFFDBC055, 0xFFDBC1AA, 0xFFBDC0C0, 0xFFEDC400, 0xFFEDC555, 0xFFEDC6AA, 0xFFEDC7FF,
    0xFFFFC800, 0xFFFFC955, 0xFFFFCAAA, 0xFFFFCCFF, 0xFF00CD00, 0xFF00CE55, 0xFF00CFAA, 0xFF00D0FF,
    0xFF12D100, 0xFF12D255, 0xFF12D3AA, 0xFF12D5FF, 0xFF24D600, 0xFF24D755, 0xFF24D8AA, 0xFF24D9FF,
    0xFF37DA00, 0xFF37DB55, 0xFF37DDAA, 0xFF37DEFF, 0xFF49DF00, 0xFF49E055, 0xFF49E1AA, 0xFF49E2FF,
    0xFF5BE300, 0xFF5BE555, 0xFF5BE6AA, 0xFF5BE7FF, 0xFF6DE800, 0xFF6DE955, 0xFF6DEAAA, 0xFF6DEBFF,
    0xFF6DEC00, 0xFF80EE55, 0xFF80EFAA, 0xFF80F0FF, 0xFF93CEA2, 0xFF92F255, 0xFF92F3AA, 0xFF92F4FF,
    0xFFA4F600, 0xFFA4F755, 0xFFA4F8AA, 0xFFA4F9FF, 0xFFB6FA00, 0xFFB6FB55, 0xFFB6FCAA, 0xFFB6FEFF,
    0xFFC9FF00, 0xFFC9FF55, 0xFFC9FFAA, 0xFFC9FFFF, 0xFFDBFF00, 0xFFDBFF55, 0xFFDBFFAA, 0xFFDBFFFF,
    0xFFEDFF00, 0xFFEDFF55, 0xFFEDFFAA, 0xFFEDFFFF, 0xFFFFFF00, 0xFFFFFF55, 0xFFFFFFAA, 0xFFFFFFFF,
};

static unsigned char clut_cycle_index[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
};


// this keeps the original colour so can revert the selected palette,
// commiting the change, simply just click on another colour item
uint32_t BACKUP_CLUT[256];

uint8_t palleteCanvas[PALETTE_VRAM_SIZE];

QTimer *updateTimer;
QTimer *scrollUpdateTimer;

enum DrawMode { Plot, Line, Rect, Circle, FloodFill } currentDrawMode = Plot;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    loadDefaultFont();


    scene = new QGraphicsScene(this);
    ui->gfxPalleteSelect->setScene(scene);

    // Your framebuffer image (480×320)
    paletteImg = QImage(PALETTE_WIDTH * PALETTE_BOX_HSIZE, PALETTE_HEIGHT * PALETTE_BOX_VSIZE, QImage::Format_RGB32);

    ui->gfxPalleteSelect->setRenderHint(QPainter::SmoothPixmapTransform, true);
    ui->gfxPalleteSelect->setRenderHint(QPainter::Antialiasing, false); // don't need antialias for pixels
    ui->gfxPalleteSelect->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->gfxPalleteSelect->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->gfxPalleteSelect->setMouseTracking(true);

    ui->gfxPalleteSelect->installEventFilter(this);

    ui->gfxPalleteSelect->viewport()->installEventFilter(this);
    ui->gfxPalleteSelect->setFocusPolicy(Qt::StrongFocus);
    ui->gfxPalleteSelect->setFocus();


    // Add it to the scene
    palettePixmap = scene->addPixmap(QPixmap::fromImage(paletteImg));
    palettePixmap->setTransformationMode(Qt::SmoothTransformation);
    renderPaletteCanvas();


    ///---------------- EDITOR GRAPHICS PORT ----------------------------//
    // the editor Canvas
    editorScene = new QGraphicsScene(this);
    ui->gfxEditor->setScene(editorScene);

    ui->gfxEditor->setMouseTracking(true);
    ui->gfxEditor->viewport()->setMouseTracking(true); // important for QGraphicsView
    ui->gfxEditor->installEventFilter(this);
    ui->gfxEditor->viewport()->installEventFilter(this);

    // Font Selector/Editor Canvas setup --------------------------------//
    fontEditor = new FontEditor(ui->gfxFontSelector, ui->gfxFontEditbox, this);  // the object of FontEditor
    fontEditor->RenderFontSelect();


    // assume old icon_area
    // new size

    // resize rows first
    icon_area.resize(icon_height);

    // resize each row (columns)
    for (auto &row : icon_area)
        row.resize(icon_width, 0);  // new cells initialized to 0, existing cells preserved



    /*
    for(int y=0; y<icon_height; y++){
        for(int x=0; x<icon_width; x++){
            icon_area[y][x] = rand() & 0x1;
        }
    }
    */

    //editorImg = QImage(32, 32, QImage::Format_RGB32);
    editorPixmap = editorScene->addPixmap(QPixmap::fromImage(editorImg));
    renderEditorCanvas();

    for(int i=0; i<256; i++){
        BACKUP_CLUT[i] = CLUT[i];
    }
    //

    updateTimer = new QTimer(this);
    updateTimer->setSingleShot(true);

    connect(updateTimer, &QTimer::timeout, this, &MainWindow::renderEditorCanvas);

    ui->chkShowGrid->setChecked(gridEnabled);

    connect(ui->cmdReorgPalette, &QPushButton::clicked, this, [this](){
        // Create a vector of indices for sorting
        std::vector<int> indices(256);
        for (int i = 0; i < 256; ++i) indices[i] = i;

        // Sort indices by visual brightness
        std::sort(indices.begin(), indices.end(), [this](int a, int b) {
            uint32_t ca = CLUT[a];
            uint32_t cb = CLUT[b];

            int ra = (ca >> 16) & 0xFF;
            int ga = (ca >> 8) & 0xFF;
            int ba = ca & 0xFF;

            int rb = (cb >> 16) & 0xFF;
            int gb = (cb >> 8) & 0xFF;
            int bb = cb & 0xFF;

            // Use max channel as brightness metric
            int brightA = std::max({ra, ga, ba});
            int brightB = std::max({rb, gb, bb});

            return brightA < brightB; // ascending
        });

        // Rearrange CLUT based on sorted indices
        //std::vector<uint32_t> newCLUT(256);
        uint32_t newCLUT[256];
        for (int i = 0; i < 256; ++i) {
            newCLUT[i] = CLUT[indices[i]];
        }
        for (int i = 0; i < 256; ++i) {
            CLUT[i] = newCLUT[i];
        }
        //CLUT = newCLUT;

        // Redraw palette
        renderEditorCanvas();
        renderPaletteCanvas();
    });

    connect(ui->txtHEXcolour32Bit, &QLineEdit::returnPressed, this, [this](){
        QString text = ui->txtHEXcolour32Bit->text().trimmed();
        bool ok = false;
        uint32_t lngColour = 0;
        uint8_t r, g, b;


        // Remove leading '#' if present
        if (text.startsWith("#")) text = text.mid(1);
        if (text.startsWith("0x") || text.startsWith("0X")) {
            // Hex with 0x prefix
            lngColour = text.mid(2).toUInt(&ok, 16);
        } else {
            // Try hex first
            lngColour = text.toUInt(&ok, 16);
            if (!ok) {
                // fallback: decimal
                lngColour = text.toUInt(&ok, 10);
            }
        }

        if (!ok) {
            QMessageBox::warning(this, "Invalid Input", "Enter a valid color number (hex or decimal).\nEG 0xFF1122 or 123456");
            return;
        }
        CLUT[numSelectedPaletteID] = lngColour;

        r = lngColour >> 16;
        g = lngColour >> 8;
        b = lngColour & 0xff;

        ui->txtPaletteR->setText(QString::number(r));
        ui->txtPaletteG->setText(QString::number(g));
        ui->txtPaletteB->setText(QString::number(b));

        ui->horizontalScrollBarR->setValue(r);
        ui->horizontalScrollBarG->setValue(g);
        ui->horizontalScrollBarB->setValue(b);

        renderPaletteCanvas();
        renderEditorCanvas();
    });


    pal = ui->lblPaletteColour->palette();
    pal.setColor(QPalette::Window, QColor(0, 0, 0)); // RGB
    ui->lblPaletteColour->setAutoFillBackground(true);   // must enable for background
    ui->lblPaletteColour->setPalette(pal);

    //connect(ui->txtPaletteR, &QPushButton::clicked(), this, [=](){
    connect(ui->horizontalScrollBarR, &QScrollBar::valueChanged, this, [=](int value){
        ui->txtPaletteR->setText(QString::number(value));
        pltColourPreset[0] = value;
        UpdatePrePaletteMixer();
        updateTimer->start(1);  // restart timer for a one-shot update
    });
    connect(ui->horizontalScrollBarG, &QScrollBar::valueChanged, this, [=](int value){
        ui->txtPaletteG->setText(QString::number(value));
        pltColourPreset[1] = value;
        UpdatePrePaletteMixer();
        updateTimer->start(1);  // restart timer for a one-shot update
    });
    connect(ui->horizontalScrollBarB, &QScrollBar::valueChanged, this, [=](int value){
        ui->txtPaletteB->setText(QString::number(value));
        pltColourPreset[2] = value;
        UpdatePrePaletteMixer();
        updateTimer->start(1);  // restart timer for a one-shot update
    });

    connect(ui->cmdRevertPalette, &QPushButton::clicked, this, [=](){
        uint8_t r,g,b;
        r = BACKUP_CLUT[numSelectedPaletteID] >> 16;
        g = BACKUP_CLUT[numSelectedPaletteID] >> 8;
        b = BACKUP_CLUT[numSelectedPaletteID] & 0xff;

        pltColourPreset[0] = r;
        pltColourPreset[1] = g;
        pltColourPreset[2] = b;

        ui->txtPaletteR->setText(QString::number(r));
        ui->txtPaletteG->setText(QString::number(g));
        ui->txtPaletteB->setText(QString::number(b));

        ui->horizontalScrollBarR->setValue(r);
        ui->horizontalScrollBarG->setValue(g);
        ui->horizontalScrollBarB->setValue(b);

        UpdatePrePaletteMixer();
        renderEditorCanvas();
    });

    connect(ui->rad24BitMode, &QRadioButton::clicked, this, [=](){
        UpdatePrePaletteMixer();
        renderEditorCanvas();
    });
    connect(ui->rad16BitMode, &QRadioButton::clicked, this, [=](){
        UpdatePrePaletteMixer();
        renderEditorCanvas();
    });



    QTimer *allDoneTimer;
    allDoneTimer = new QTimer(this);
    allDoneTimer->setSingleShot(true);

    connect(allDoneTimer, &QTimer::timeout, this, &MainWindow::reSize);
    //icon_width
    ui->txtProjectImageWidth->setText(QString("%1").arg(icon_width));
    ui->txtProjectImageHeight->setText(QString("%1").arg(icon_height));

    allDoneTimer->start(100);

    connect(ui->chkShowGrid, &QCheckBox::clicked, this, [this](){
        gridEnabled = ui->chkShowGrid->isChecked();
        renderEditorCanvas();
    });

    ui->lblEditorZoomLevel->setText(QString("%1").arg(icon_zoom));  // load the default value
    connect(ui->scrEditorZoomVal, &QScrollBar::valueChanged, this, [this](){
        icon_zoom = ui->scrEditorZoomVal->value();
        ui->lblEditorZoomLevel->setText(QString("%1").arg(icon_zoom));
        reSize();
        renderEditorCanvas();
    });

    connect(ui->cmdZoomPreX1, &QPushButton::clicked, this, [this](){
        ui->scrEditorZoomVal->setValue(1);
        icon_zoom = ui->scrEditorZoomVal->value();
        ui->lblEditorZoomLevel->setText(QString("%1").arg(icon_zoom));
        reSize();
        renderEditorCanvas();
    });
    connect(ui->cmdZoomPreX4, &QPushButton::clicked, this, [this](){
        ui->scrEditorZoomVal->setValue(4);
        icon_zoom = ui->scrEditorZoomVal->value();
        ui->lblEditorZoomLevel->setText(QString("%1").arg(icon_zoom));
        reSize();
        renderEditorCanvas();
    });
    connect(ui->cmdZoomPreX8, &QPushButton::clicked, this, [this](){
        ui->scrEditorZoomVal->setValue(8);
        icon_zoom = ui->scrEditorZoomVal->value();
        ui->lblEditorZoomLevel->setText(QString("%1").arg(icon_zoom));
        reSize();
        renderEditorCanvas();
    });
    connect(ui->cmdZoomPreX16, &QPushButton::clicked, this, [this](){
        ui->scrEditorZoomVal->setValue(16);
        icon_zoom = ui->scrEditorZoomVal->value();
        ui->lblEditorZoomLevel->setText(QString("%1").arg(icon_zoom));
        reSize();
        renderEditorCanvas();
    });
    connect(ui->cmdZoomPreX32, &QPushButton::clicked, this, [this](){
        ui->scrEditorZoomVal->setValue(32);
        icon_zoom = ui->scrEditorZoomVal->value();
        ui->lblEditorZoomLevel->setText(QString("%1").arg(icon_zoom));
        reSize();
        renderEditorCanvas();
    });

    connect(ui->cmdPushImageUp, &QPushButton::clicked, this, [this](){
        // Temporary copy of the top row
        uint8_t topRow[icon_width];
        for (int x = 0; x < icon_width; x++)
            topRow[x] = icon_area[0][x];

        // Shift all rows up
        for (int y = 0; y < icon_height - 1; y++)
            for (int x = 0; x < icon_width; x++)
                icon_area[y][x] = icon_area[y + 1][x];

        // Put the top row at the bottom
        for (int x = 0; x < icon_width; x++)
            icon_area[icon_height - 1][x] = topRow[x];

        renderEditorCanvas();
    });

    connect(ui->cmdPushImageDown, &QPushButton::clicked, this, [this](){
        uint8_t bottomRow[icon_width];
        for (int x = 0; x < icon_width; x++) bottomRow[x] = icon_area[icon_height - 1][x];

        for (int y = icon_height - 1; y > 0; y--)
            for (int x = 0; x < icon_width; x++)
                icon_area[y][x] = icon_area[y - 1][x];

        for (int x = 0; x < icon_width; x++)
            icon_area[0][x] = bottomRow[x];

        renderEditorCanvas();
    });

    connect(ui->cmdPushImageLeft, &QPushButton::clicked, this, [this](){
        for (int y = 0; y < icon_height; y++) {
            uint8_t leftPixel = icon_area[y][0];
            for (int x = 0; x < icon_width - 1; x++)
                icon_area[y][x] = icon_area[y][x + 1];
            icon_area[y][icon_width - 1] = leftPixel;
        }
        renderEditorCanvas();
    });

    connect(ui->cmdPushImageRight, &QPushButton::clicked, this, [this](){
        for (int y = 0; y < icon_height; y++) {
            uint8_t rightPixel = icon_area[y][icon_width - 1];
            for (int x = icon_width - 1; x > 0; x--)
                icon_area[y][x] = icon_area[y][x - 1];
            icon_area[y][0] = rightPixel;
        }
        renderEditorCanvas();
    });

    connect(ui->cmdFlipH, &QPushButton::clicked, this, [this](){
        for(int y = 0; y < icon_height / 2; y++){
            for(int x = 0; x < icon_width; x++){
                uint8_t tmp = icon_area[y][x];
                icon_area[y][x] = icon_area[icon_height - 1 - y][x];
                icon_area[icon_height - 1 - y][x] = tmp;
            }
        }
        renderEditorCanvas();
    });

    connect(ui->cmdFlipV, &QPushButton::clicked, this, [this](){
        for(int y = 0; y < icon_height; y++){
            for(int x = 0; x < icon_width / 2; x++){
                uint8_t tmp = icon_area[y][x];
                icon_area[y][x] = icon_area[y][icon_width - 1 - x];
                icon_area[y][icon_width - 1 - x] = tmp;
            }
        }
        renderEditorCanvas();
    });

    connect(ui->cmdClsIconImage, &QPushButton::clicked, this, [this](){
        auto reply = QMessageBox::question(
            this,
            "Clear Icon",
            "Are you sure you want to clear the icon?",
            QMessageBox::Yes | QMessageBox::No
            );

        if(reply == QMessageBox::Yes) {
            // Clear icon_area
            for(int y = 0; y < icon_height; y++)
                for(int x = 0; x < icon_width; x++)
                    icon_area[y][x] = 0;

            renderEditorCanvas(); // redraw empty icon
        }
    });

    connect(ui->cmdSaveIconProject, &QPushButton::clicked, this, [this](){
        static QString lastDir;
        QSettings settings("Electronscape", "SidBox-GraphicsEditV3");
        lastDir = settings.value("lastProjectDir", QDir::homePath()).toString();

        QString filename = QFileDialog::getSaveFileName(this, "Save Icon", lastDir, "Icon Files (*.icn)");
        // saveIcon(filename);
        if(!filename.isEmpty()){
            QFileInfo info(filename);
            QSettings settings("Electronscape", "SidBox-GraphicsEditV3");
            settings.setValue("lastProjectDir", info.absolutePath());
            saveProjectIcon(filename.toUtf8().constData());
        }

    });

    connect(ui->cmdLoadIconProject, &QPushButton::clicked, this, [this](){
        static QString lastDir;
        QSettings settings("Electronscape", "SidBox-GraphicsEditV3");
        lastDir = settings.value("lastProjectDir", QDir::homePath()).toString();
        QString filename = QFileDialog::getOpenFileName(this, "Load Icon", lastDir, "Icon Files (*.icn)");
        if(!filename.isEmpty()) {
            QFileInfo info(filename);
            QSettings settings("Electronscape", "SidBox-GraphicsEditV3");
            settings.setValue("lastProjectDir", info.absolutePath());
            loadProjectIcon(filename.toUtf8().constData());
        }
    });

    connect(ui->cmdSetIconAreaSize, &QPushButton::clicked, this, [=](){
        icon_width = ui->txtProjectImageWidth->text().toInt();
        icon_height = ui->txtProjectImageHeight->text().toInt();

        if(icon_width>2048) icon_width = 2048;
        if(icon_height>2048) icon_height = 2048;

        if(icon_width  < 8) icon_width = 8;
        if(icon_height < 8) icon_height = 8;

        ui->txtProjectImageWidth->setText(QString("%1").arg(icon_width));
        ui->txtProjectImageHeight->setText(QString("%1").arg(icon_height));

        // resize rows first
        icon_area.resize(icon_height);

        // resize each row (columns)
        for (auto &row : icon_area)
            row.resize(icon_width, 0);  // new cells initialized to 0, existing cells preserved

        reSize();
        renderEditorCanvas();
    });

    scrollUpdateTimer = new QTimer(this);
    scrollUpdateTimer->setSingleShot(true);

    connect(scrollUpdateTimer, &QTimer::timeout, this, &MainWindow::renderEditorCanvas);

    connect(ui->scrEditorH, &QScrollBar::valueChanged, this, &MainWindow::onEditorScrollChanged);
    connect(ui->scrEditorV, &QScrollBar::valueChanged, this, &MainWindow::onEditorScrollChanged);

    connect(ui->cmdRotateCC90, &QPushButton::clicked, this, [this](){
        rotateIcon(1);
    });
    connect(ui->cmdRotateC90, &QPushButton::clicked, this, [this](){
        rotateIcon(0);
    });

    //enum DrawMode { Plot, Line, Rect, Circle, FloodFill } currentDrawMode = Plot;
    connect(ui->radDrawModePlot,      &QPushButton::clicked, this, [this](){ currentDrawMode = Plot;      });
    connect(ui->radDrawModeLine,      &QPushButton::clicked, this, [this](){ currentDrawMode = Line;      });
    connect(ui->radDrawModeCircle,    &QPushButton::clicked, this, [this](){ currentDrawMode = Circle;    });
    connect(ui->radDrawModeRect,      &QPushButton::clicked, this, [this](){ currentDrawMode = Rect;      });
    connect(ui->radDrawModeFloodfill, &QPushButton::clicked, this, [this](){ currentDrawMode = FloodFill; });

    connect(ui->cmdImportImage, &QPushButton::clicked, this, [this](){
        static QString lastDir;
        QSettings settings("Electronscape", "SidBox-GraphicsEditV3");
        lastDir = settings.value("lastImportDir", QDir::homePath()).toString();


        QString filename = QFileDialog::getOpenFileName(this, "Import Image...", lastDir,
                "Acceptable Images [*.bmp, *.png, *.gif, *.jpg, *.iff, *.ilbm](*.bmp *.png *.gif *.iff *.ilbm *.jpg);;"
                "bitmap [*.bmp](*.bmp)");
        if(!filename.isEmpty()) {

            QFileInfo info(filename);
            QSettings settings("Electronscape", "SidBox-GraphicsEditV3");
            settings.setValue("lastImportDir", info.absolutePath());


            importGif(filename);
        }
    });

    connect(ui->chkImportPalette, &QCheckBox::clicked, this, [this](){
        if(ui->chkImportPalette->isChecked()){
            ui->chkUsePalette->setChecked(false);
        }
    });
    connect(ui->chkUsePalette, &QCheckBox::clicked, this, [this](){
        if(ui->chkUsePalette->isChecked()){
            ui->chkImportPalette->setChecked(false);
        }
    });

    connect(ui->cmdDefaultPalette, &QPushButton::clicked, this, [this](){
        auto reply = QMessageBox::question(
            this,
            "Palette",
            "Are you sure you want to reset this palette?",
            QMessageBox::Yes | QMessageBox::No
            );

        if(reply == QMessageBox::Yes) {
            // Clear icon_area

            for(int i = 0; i < 256; i++){
                CLUT[i] = DEFAULT_CLUT[i];
                BACKUP_CLUT[i] = DEFAULT_CLUT[i];
            }
            renderPaletteCanvas();
            renderEditorCanvas(); // redraw empty icon
        }
    });

    connect(ui->chkColourBits1, &QRadioButton::clicked, this, [this](){ paletteDepth = 2;   });
    connect(ui->chkColourBits2, &QRadioButton::clicked, this, [this](){ paletteDepth = 4;   });
    connect(ui->chkColourBits4, &QRadioButton::clicked, this, [this](){ paletteDepth = 16;  });
    connect(ui->chkColourBits8, &QRadioButton::clicked, this, [this](){ paletteDepth = 256; });

    connect(ui->cmdReassignColour, &QPushButton::clicked, this, [this](){
        //QMessageBox::warning(this, "Reassign", "To re-assign, click on the new palette index!\nThis will change the currently selected palette ID to the new one OnClick");
        bReassignedPaletteIndex = true;
        capturedPaletteIndex = numSelectedPaletteID;
        ui->cmdReassignColour->setEnabled(false);
    });

    connect(ui->cmdSwapColours, &QPushButton::clicked, this, [this](){
        bSwapColours = true;
        capturedPaletteIndex = numSelectedPaletteID;
        ui->cmdSwapColours->setEnabled(false);
    });

    connect(ui->cmdSpreadPalette, &QPushButton::clicked, this, [this](){
        bSpreadPalette = true;
        capturedPaletteIndex = numSelectedPaletteID;
    });

    connect(ui->cmdExportClut16, &QPushButton::clicked, this, [this](){
        ui->txtOutputText->clear();  // clear previous content

        QString output;
        output += "#include <stdint.h>\n\n";
        output += "uint16_t clut[256] = {\n";

        for (int y = 0; y < 32; ++y) {
            output += "    ";
            for (int x = 0; x < 8; ++x) {
                int i = y * 8 + x;
                if (i >= 256) break;        // safety check

                uint32_t rgb = CLUT[i];
                uint8_t r = (rgb >> 16) & 0xFF;
                uint8_t g = (rgb >> 8)  & 0xFF;
                uint8_t b = rgb & 0xFF;

                // Convert to RGB565
                uint16_t rgb565 = ((r & 0xF8) << 8) |  // 5 bits red
                                  ((g & 0xFC) << 3) |  // 6 bits green
                                  ((b & 0xF8) >> 3);   // 5 bits blue

                output += "0x" + QString::number(rgb565, 16).rightJustified(4, '0').toUpper();
                if (i < 255) output += ",";
                if(x != 7) output += " ";

            }
            output += "\n";
        }
        output += "};\n";

        // Show in the text view
        ui->txtOutputText->setPlainText(output);
        ui->outputTextView->show();
    });


    connect(ui->cmdExportClut32, &QPushButton::clicked, this, [this](){
        ui->txtOutputText->clear();  // clear previous content
        QString output;
        uint32_t rgb;
        output += "#include <stdint.h>\n\n";
        output += "uint32_t clut[256] = {\n";

        for (int y = 0; y < 32; ++y) {      // 32 lines
            output += "    ";
            for (int x = 0; x < 8; ++x) {   // 8 entries per line
                int i = y * 8 + x;
                if (i >= 256) break;        // safety check

                if(i == 0)
                    rgb = CLUT[i] & 0xFFFFFF; // no alpha on this palette entry
                else
                    rgb = CLUT[i] | 0xFF000000; // force alpha 255

                output += "0x" + QString::number(rgb, 16).rightJustified(8, '0').toUpper();
                if (i < 255) output += ",";
                if(x != 7) output += " ";
            }
            output += "\n";
        }
        output += "};\n";

        // Show in the text view
        ui->txtOutputText->setPlainText(output);
        ui->outputTextView->show();
    });

    connect(ui->cmdCloseOutputText, &QPushButton::clicked, this, [this](){
        ui->outputTextView->hide();
    });

    connect(ui->cmdSavePalette, &QPushButton::clicked, this, [this](){
        // save de palette!!
        QString filename = QFileDialog::getSaveFileName(this, "Save Palette", "", "Paltette Files (*.pal)");
        // saveIcon(filename);
        if(!filename.isEmpty())
            SavePaletteData(filename.toUtf8().constData());
    });

    connect(ui->cmdLoadPalette, &QPushButton::clicked, this, [this](){
        // save de palette!!
        QString filename = QFileDialog::getOpenFileName(this, "Open Palette", "", "Paltette Files (*.pal)");
        // saveIcon(filename);
        if(!filename.isEmpty())
            LoadPaletteData(filename.toUtf8().constData());
    });

    connect(ui->cmdExportH, &QPushButton::clicked, this, [this](){
        uint16_t bits;   // collect the config bits
        bits = ui->chkExportRLE->isChecked() * ExportRLE;
        bits += ui->chkExportSBVRAM->isChecked() * ExportSidBoxVRAM;
        //printf("Bits checked: %x\n", bits);
        ExportImageToH("", bits);
    });

    ui->outputTextView->hide();
    ui->frmFontWorkbench->hide();

    connect(ui->cmdOpenFontWorkbench, &QPushButton::clicked, this, [this](){
        ui->frmFontWorkbench->show();
    });

    connect(ui->cmdCloseFontWorkbench, &QPushButton::clicked, this, [this](){
        ui->frmFontWorkbench->hide();
    });



    colourCycleSpeed = ui->scrColourCycleSpeed->value() * 22;
    ui->lblColourCycle->setText(QString("%1 ms").arg(colourCycleSpeed));

    tmrColourCycle = new QTimer(this);          // create the timer
    tmrColourCycle->setInterval(colourCycleSpeed);  // initial interval


    connect(ui->scrColourCycleSpeed, &QScrollBar::valueChanged, this, [this](){
        colourCycleSpeed = ui->scrColourCycleSpeed->value() * 22;
        ui->lblColourCycle->setText(QString("%1 ms").arg(colourCycleSpeed));
        // todo: change the QTimer
        if (tmrColourCycle) {
            tmrColourCycle->setInterval(colourCycleSpeed);
        }

    });

    connect(ui->chkColourCycleEn, &QCheckBox::clicked, this, [this](){
        if(!(ui->chkColourCycleEn->isChecked())){
            for(int i = 0; i < 256; i++){
                CLUT[i] = BACKUP_CLUT[i];
                renderPaletteCanvas();
                renderEditorCanvas(); // redraw empty icon
            }
        }
        //doColourCycles();

    });


    ///// FONTEditor ////
    connect(ui->cmdLoadDefaultFont, &QPushButton::clicked, this, [this](){
        auto reply = QMessageBox::question(
            this,
            "Default Font",
            "Are you sure you want to reset this font?",
            QMessageBox::Yes | QMessageBox::No
            );

        if(reply == QMessageBox::Yes){
            loadDefaultFont();
            fontEditor->RenderFontEdit();
            fontEditor->RenderFontSelect();
        }
    });

    // FONT EDIT Buttons
    connect(ui->cmdPushFontLeft,    &QPushButton::clicked, this, [this](){ fontEditor->MoveFont(font_move_left);});
    connect(ui->cmdPushFontRight,   &QPushButton::clicked, this, [this](){ fontEditor->MoveFont(font_move_right);});
    connect(ui->cmdPushFontUp,      &QPushButton::clicked, this, [this](){ fontEditor->MoveFont(font_move_up);});
    connect(ui->cmdPushFontDown,    &QPushButton::clicked, this, [this](){ fontEditor->MoveFont(font_move_down);});
    connect(ui->cmdRotateFontCC90,  &QPushButton::clicked, this, [this](){ fontEditor->MoveFont(font_rotate_cc);});
    connect(ui->cmdRotateFontC90,   &QPushButton::clicked, this, [this](){ fontEditor->MoveFont(font_rotate_c);});
    connect(ui->cmdFlipFontV,       &QPushButton::clicked, this, [this](){ fontEditor->MoveFont(font_flip_v);});
    connect(ui->cmdFlipFontH,       &QPushButton::clicked, this, [this](){ fontEditor->MoveFont(font_flip_h);});

    connect(ui->cmdRotateFontsCC90, &QPushButton::clicked, this, [this](){ fontEditor->MoveFont(font_rotate_cc_all);});
    connect(ui->cmdRotateFontsC90,  &QPushButton::clicked, this, [this](){ fontEditor->MoveFont(font_rotate_c_all);});


    connect(ui->cmdClearFontBank,  &QPushButton::clicked, this, [this](){
        auto reply = QMessageBox::question(
            this,
            "Clear Font",
            "Are you sure you want to clear the entire font?\n(NOTE: use set default if you want them all back, this WILL over write what you've done though!)",
            QMessageBox::Yes | QMessageBox::No
            );

        if(reply == QMessageBox::Yes){
            clearFontBank();
            fontEditor->RenderFontEdit();
            fontEditor->RenderFontSelect();
        }
    });

    connect(ui->cmdLoadFont, &QPushButton::clicked, this, [this](){ fontEditor->LoadFont();   });
    connect(ui->cmdSaveFont, &QPushButton::clicked, this, [this](){ fontEditor->SaveFontAs(); });

    connect(ui->cmdExportFont, &QPushButton::clicked, this, [this](){
        fontEditor->ExportFont(ui->txtOutputText);
        //
        ui->outputTextView->show();
        ui->frmFontWorkbench->hide();
    });

    connect(ui->sldPaletteOffset, &QScrollBar::valueChanged, this, [this](){
        paletteRangerOffset = ui->sldPaletteOffset->value();
        ui->lblPaletteOffset->setText( QString("%1").arg(paletteRangerOffset));
    });
    connect(ui->sldPaletteSize, &QScrollBar::valueChanged, this, [this](){
        paletteRangerLength = ui->sldPaletteSize->value();
        ui->lblPaletteSizer->setText( QString("%1").arg(paletteRangerLength));
    });
    connect(ui->chkPaletteUseRestrictor, &QCheckBox::clicked, this, [this](){
        paletteRestrictor = ui->chkPaletteUseRestrictor->isChecked();
    });


    paletteRangerOffset = 128;
    paletteRangerLength = 16;

    ui->lblPaletteOffset->setText( QString("%1").arg(paletteRangerOffset));
    ui->lblPaletteSizer->setText( QString("%1").arg(paletteRangerLength));

    ui->sldPaletteOffset->setValue(paletteRangerOffset);
    ui->sldPaletteSize->setValue(paletteRangerLength);


    loadDefaultFont();

    connect(tmrColourCycle, &QTimer::timeout, this, &MainWindow::onColourCycleTick);  // your slot
    tmrColourCycle->start();

}


void MainWindow::doColourCycle(){
    static char cbd;
    static unsigned char i;
    static unsigned long tmp;
    static unsigned char tmpold;

    static unsigned short SpeedStep;



    //if(cyclespeed==255) return;

    int bCycleDirection = 0;


    //cyclefrom = 80;
    //cycleto = cyclefrom + (length - 1);

    if (bCycleDirection == 0) {
        tmp = CLUT[cyclefrom];
        tmpold = clut_cycle_index[cyclefrom];
        for (i = cyclefrom; i < cycleto; i++) {
            CLUT[i] = CLUT[i + 1];
            clut_cycle_index[i] = clut_cycle_index[i + 1];
        }
        CLUT[i] = tmp;
        clut_cycle_index[i] = tmpold;

    } else {
        tmp = CLUT[cycleto];
        tmpold = clut_cycle_index[cycleto];
        for (i = cycleto; i > cyclefrom; i--) {
            CLUT[i] = CLUT[i - 1];
            clut_cycle_index[i] = clut_cycle_index[i - 1];
        }
        CLUT[i] = tmp;
        clut_cycle_index[i] = tmpold;
    }

    renderPaletteCanvas();
    renderEditorCanvas(); // redraw empty icon
}

void MainWindow::onColourCycleTick() {
    // do your colour cycling here
    if(!(ui->chkColourCycleEn->isChecked())) return;    // do nothing
    doColourCycle();
}

QString hex8(uint8_t value){
    // returns a string like "0xAF" (0x lowercase, digits uppercase)
    return QString("0x%1").arg(QString::number(value, 16).toUpper().rightJustified(2, '0'));
}


QString generateRLE(
    const std::vector<std::vector<uint8_t>>& icon_area,
    uint16_t width,
    uint16_t height,
    int colWidthMax,
    int& outRLESize)   // returns compressed byte count
{
    QString output;
    int columnstep = 0;
    bool firstElement = true;
    uint8_t lastPixel = 0xFF;
    uint8_t runLength = 0;

    outRLESize = 0;

    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            uint8_t pixel = icon_area[y][x];

            if (pixel == lastPixel && runLength < 255) {
                runLength++;
            } else {
                if (runLength > 0) {
                    // line wrap first
                    if (columnstep >= colWidthMax) {
                        output += "\n    ";
                        columnstep = 0;
                    }

                    // prepend comma if needed
                    if (!firstElement && columnstep > 0) output += ", ";

                    // output RLE pair
                    output += hex8(runLength) + ", " + hex8(lastPixel);
                    columnstep += 2;

                    outRLESize += 2; // count + value

                    firstElement = false;
                }

                lastPixel = pixel;
                runLength = 1;
            }
        }
    }

    // flush final run
    if (runLength > 0) {
        if (columnstep >= colWidthMax) {
            output += "\n    ";
            columnstep = 0;
        }
        if (!firstElement && columnstep > 0) output += ", ";
        output += hex8(runLength) + ", " + hex8(lastPixel);
        outRLESize += 2;
    }

    return output;
}


void MainWindow::ExportImageToH(const char *filename, const uint16_t modes){
    // export the icon_area according to colour bit size and rotation and memory mapping. GONNA be some funky crap


    uint16_t    imgW, imgH; // 16bit never should see an image 64k wide or in height! IMAGING the storage size!!
    uint32_t    imgLen;

    // temp local vars, might need to manipulate these later

    uint8_t w_lo = (icon_width >> 8) & 0xFF;
    uint8_t w_hi = icon_width & 0xFF;

    uint8_t h_lo = (icon_height >> 8) & 0xFF;
    uint8_t h_hi = icon_height & 0xFF;

    // the size will be affected by the bit depth
    uint8_t bitDepth;

    switch (paletteDepth) {
        case 2:   bitDepth = 1; break;
        case 4:   bitDepth = 2; break;
        case 16:  bitDepth = 4; break;
        case 256: bitDepth = 8; break;
    }

    bitDepth = paletteDepth;
    imgLen = icon_width * icon_height;

    uint8_t il_v0 = (imgLen >> 24) & 0xff;
    uint8_t il_v1 = (imgLen >> 16) & 0xff;
    uint8_t il_v2 = (imgLen >> 8) & 0xff;
    uint8_t il_v3 = (imgLen >> 0) & 0xff;


    QString output;
    QString rleData;
    output += "#include <stdint.h>\n\n";
    output += "// Image Params //\n";
    if(!(modes & ExportRLE))
        output += "// non compressed \n";
    else
        output += "// RLE compressed bytes are now (how-many), (pixel colour index), ...\n";
    output += "\n";
    output += "uint8_t image[] = {\n";

    uint8_t configbits = 0;
    configbits = (!!(modes & ExportRLE) << 4);

    //output += "    0x00,                   // Colour depth (1=2colours, 2=4colours, 4=16colours, 8=256colours\n";   // colour depth 1, 2, 4, 8
    output += QString("    %1,                 // Colour depth (1,2,4,8 bit colour modes) + 0x10 if RLE\n")
                  .arg(hex8(configbits));
    output += QString("    %1, %2,             // image width (%3)\n")
                  .arg(hex8(w_lo))
                  .arg(hex8(w_hi))
                  .arg(imgW);

    // Image height
    output += QString("    %1, %2,             // image height (%3)\n")
                  .arg(hex8(h_lo))
                  .arg(hex8(h_hi))
                  .arg(imgH);

    // Total image array size
    if(!(modes & ExportRLE)){
        output += QString("    %1, %2, %3, %4, // total image array size: %5 bytes\n")
                  .arg(hex8(il_v0))
                  .arg(hex8(il_v1))
                  .arg(hex8(il_v2))
                  .arg(hex8(il_v3))
                  .arg(imgLen);
    } else {
        // will need to pre-compress this to know the size wont we?

        // TODO: a precompress helper needed i think
        int rleSize;
        rleData = generateRLE(icon_area, icon_width, icon_height, 16, rleSize);

        il_v0 = (rleSize >> 24) & 0xff;
        il_v1 = (rleSize >> 16) & 0xff;
        il_v2 = (rleSize >> 8) & 0xff;
        il_v3 = (rleSize >> 0) & 0xff;

        output += QString("    %1, %2, %3, %4, // total image array size: %5 bytes\n")
                      .arg(hex8(il_v0))
                      .arg(hex8(il_v1))
                      .arg(hex8(il_v2))
                      .arg(hex8(il_v3))
                      .arg(rleSize);
    }

    const uint8_t colWidthMax = 16;
    uint8_t     colDat = 0;
    int columnstep = 0;


    if(!(modes & ExportRLE)){
        /// Uncompressed, unchanged
        // the loop of the actual data to be spat out
        // for now spit out the full byte, no packing bits yet
        output += "    ";
        for (uint16_t y = 0; y < icon_height; y++) {
            for (uint16_t x = 0; x < icon_width; x++) {
                uint8_t colDat = icon_area[y][x];

                // Add comma only if this is NOT the first element
                if (!(x == 0 && y == 0)) {
                    output += ", ";
                }
                // Insert newline+indent when exceeding column width
                if (columnstep >= colWidthMax) {
                    output += "\n    ";
                    columnstep = 0;
                }
                output += QString("%1").arg(hex8(colDat));
                columnstep++;
            }
        }
    } else {
        output += "    ";
        output += rleData;
    }

    output += "\n};\n";

    // Show in the text view
    ui->txtOutputText->setPlainText(output);
    ui->outputTextView->show();
}



void MainWindow::SavePaletteData(const char *filename){
        FILE *f = fopen(filename, "wb");
        if(!f) {
            QMessageBox::warning(this, "Save Palette Fail", "Cannot open file for writing!");
            return;
        }
        fwrite(CLUT, sizeof(uint32_t), 256, f);
        fclose(f);
}

void MainWindow::LoadPaletteData(const char *filename){
    FILE *f = fopen(filename, "rb");
    if(!f) {
        QMessageBox::warning(this, "Load Palette Fail", "Cannot open file for read!");
        return;
    }
    fread(CLUT, sizeof(uint32_t), 256, f);
    fclose(f);

    renderPaletteCanvas();
    renderEditorCanvas(); // redraw empty icon
}

void MainWindow::doSpreadPalette(uint8_t targetID){
    uint8_t r, g, b,
            r1, g1, b1,
            r2, g2, b2;

    int spreadLength;
    uint32_t fromColour = CLUT[capturedPaletteIndex];
    uint32_t toColour   = CLUT[targetID];

    int start = capturedPaletteIndex;
    int end   = targetID;

    bSpreadPalette = false;


    // Ensure start < end
    if (start > end) {
        std::swap(start, end);
        std::swap(fromColour, toColour);
    }

    spreadLength = end - start;

    // Extract RGB
    r1 = (fromColour >> 16) & 0xFF;
    g1 = (fromColour >> 8)  & 0xFF;
    b1 = (fromColour)       & 0xFF;

    r2 = (toColour >> 16) & 0xFF;
    g2 = (toColour >> 8)  & 0xFF;
    b2 = (toColour)       & 0xFF;

    for (int i = 0; i <= spreadLength; i++) {
        float t = (spreadLength == 0) ? 1.0f : float(i) / float(spreadLength);
        r = r1 + (int)((r2 - r1) * t);
        g = g1 + (int)((g2 - g1) * t);
        b = b1 + (int)((b2 - b1) * t);
        CLUT[start + i] = (r << 16) | (g << 8) | b;
    }
    renderPaletteCanvas();
}



bool extractGifPalette(const QString &path, uint32_t CLUT[256])
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;

    QByteArray data = f.readAll();
    f.close();

    if (data.size() < 13) return false; // minimal GIF header + LSD

    // Check header "GIF87a" or "GIF89a"
    if (!(data.startsWith("GIF87a") || data.startsWith("GIF89a"))) return false;

    // Logical Screen Descriptor starts at byte 6
    quint8 packed = quint8(data[10]);

    bool hasGlobalPalette = packed & 0x80; // 1 = global palette exists
    int colorRes = (packed & 0x07);        // size = 2^(N+1)
    int paletteSize = 2 << colorRes;

    if (!hasGlobalPalette) return false;

    //CLUT.resize(paletteSize);
    const uchar *p = reinterpret_cast<const uchar*>(data.constData());
    int offset = 13; // header (6) + LSD (7)

    for (int i = 0; i < paletteSize; i++) {
        int r = p[offset + i*3 + 0];
        int g = p[offset + i*3 + 1];
        int b = p[offset + i*3 + 2];
        CLUT[i] = (r << 16) | (g << 8) | b;
    }

    return true;
}

bool extractPngPalette(const QString &path, uint32_t CLUT[256])
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;

    QByteArray data = f.readAll();
    f.close();

    if (data.size() < 8) return false; // PNG signature
    if (!(data.startsWith("\x89PNG\r\n\x1a\n"))) return false;

    const uchar* p = reinterpret_cast<const uchar*>(data.constData());
    int pos = 8; // after signature
    int clutIndex = 0;

    while (pos + 8 <= data.size()) {
        quint32 chunkLen = (p[pos+0]<<24) | (p[pos+1]<<16) | (p[pos+2]<<8) | p[pos+3];
        QByteArray chunkType = data.mid(pos+4, 4);

        if(paletteRestrictor){
            if (chunkType == "PLTE") {
                int totalCols = chunkLen / 3;
                if (totalCols <= 0) return false;

                struct Col {
                    int r, g, b;
                    int lum;
                };

                QVector<Col> cols;
                cols.reserve(totalCols);

                // 1) Read full palette
                for (int i = 0; i < totalCols; i++) {
                    int base = pos + 8 + i * 3;
                    int r = p[base + 0];
                    int g = p[base + 1];
                    int b = p[base + 2];

                    // perceptual-ish luminance
                    int lum = r * 30 + g * 59 + b * 11;

                    cols.push_back({ r, g, b, lum });
                }

                // 2) Sort by luminance (groups similar shades)
                std::sort(cols.begin(), cols.end(),
                          [](const Col &a, const Col &b) {
                              return a.lum < b.lum;
                          });

                int outCount = qMin(paletteRangerLength, totalCols);
                float step = float(totalCols) / float(outCount);

                // 3) Average groups
                for (int i = 0; i < outCount; i++) {

                    int start = int(i * step);
                    int end   = int((i + 1) * step);
                    if (end <= start) end = start + 1;
                    if (end > totalCols) end = totalCols;

                    int rs = 0, gs = 0, bs = 0;
                    int cnt = 0;

                    for (int j = start; j < end; j++) {
                        rs += cols[j].r;
                        gs += cols[j].g;
                        bs += cols[j].b;
                        cnt++;
                    }

                    if (cnt == 0) cnt = 1;

                    int r = rs / cnt;
                    int g = gs / cnt;
                    int b = bs / cnt;

                    CLUT[paletteRangerOffset + i] =
                        (r << 16) | (g << 8) | b;
                }

                return true;
            }
        } else {
            if (chunkType == "PLTE") {
                int n = qMin(int(chunkLen / 3), 256);
                for (int i = 0; i < n; i++) {
                    int r = p[pos+8 + i*3 + 0];
                    int g = p[pos+8 + i*3 + 1];
                    int b = p[pos+8 + i*3 + 2];
                    CLUT[i] = (r << 16) | (g << 8) | b;
                }
                // fill rest with black
                for (int i = n; i < 256; i++) CLUT[i] = 0;
                return true;
            }
        }

        pos += 8 + chunkLen + 4; // move to next chunk (length + type + data + CRC)
    }

    return false; // no PLTE found
}


bool MainWindow::importGif(const QString &path){

    bool isGIF, isPNG;
    QImage img(path);
    if (img.isNull()) return false;

    img = QImage(path);

    // Create a 256-colour palette (your own, no dither!)
    QVector<QRgb> pal;
    QVector<QRgb> ct;


    int r, g, b, rf, gf, bf;
    int w = img.width();
    int h = img.height();
    int ictoffset = 0;

    pal.resize(256);

    if (ui->chkImportPalette->isChecked()) {
        bool isGood;
        isGood = false;
        isGIF = extractGifPalette(path, CLUT);
        if(!isGIF) {
            printf("Not Gif\n");
            isPNG = extractPngPalette(path, CLUT);
            if(!isPNG)
                printf("not png\n");
        }

        isGood = isGIF + isPNG;

        if(!isGood){    // no pallete data good enough, will have to faff for it our selves
            img = img.convertToFormat(QImage::Format_Indexed8);
            ct = img.colorTable();
            for (int i = 0; i < ct.size(); i++) {
                int r = qRed(ct[i + ictoffset]), g = qGreen(ct[i + ictoffset]), b = qBlue(ct[i + ictoffset]);
                CLUT[i] = (r << 16) | (g << 8) | b;
                BACKUP_CLUT[i] = CLUT[i];
            }
        }

        renderPaletteCanvas();
    }


    icon_area.assign(h, std::vector<uint8_t>(w, 0));
    icon_width  = w;
    icon_height = h;
    ui->txtProjectImageWidth->setText(QString("%1").arg(icon_width));
    ui->txtProjectImageHeight->setText(QString("%1").arg(icon_height));


    if(paletteRestrictor){
        int palStart = 0;
        int palEnd   = paletteDepth;

        if (paletteRestrictor) {
            palStart = paletteRangerOffset;
            palEnd   = paletteRangerOffset + paletteRangerLength;
            if (palEnd > 256) palEnd = 256;
        }

        // ===== PIXELS =====
        for (int y = 0; y < h; y++) {
            const uchar* row = img.scanLine(y);
            for (int x = 0; x < w; x++) {

                uint8_t colourIndex = row[x];

                //if (ui->chkUsePalette->isChecked())
                {

                    ct = img.colorTable();

                    int rf = (ct[colourIndex] >> 16) & 0xFF;
                    int gf = (ct[colourIndex] >> 8)  & 0xFF;
                    int bf =  ct[colourIndex]        & 0xFF;

                    int bestIndex = palStart;
                    int bestDist  = INT_MAX;

                    // 🔥 SEARCH ONLY WITHIN RANGE
                    for (int pi = palStart; pi < palEnd; pi++) {

                        int r2 = (CLUT[pi] >> 16) & 0xFF;
                        int g2 = (CLUT[pi] >> 8)  & 0xFF;
                        int b2 =  CLUT[pi]        & 0xFF;

                        int dr = rf - r2;
                        int dg = gf - g2;
                        int db = bf - b2;

                        int dist = dr*dr + dg*dg + db*db;

                        if (dist < bestDist) {
                            bestDist  = dist;
                            bestIndex = pi;
                        }
                    }

                    colourIndex = bestIndex;
                }

                icon_area[y][x] = colourIndex;
            }
        }




    } else { // not using the restrictor

        for (int i = 0; i < 256; i++)
            pal[i] = CLUT[i];//qRgb(i, i, i);

        // Apply your palette
        img = img.convertToFormat(QImage::Format_Indexed8, pal, Qt::AvoidDither);

        // ===== PIXELS =====
        for (int y = 0; y < h; y++) {
            const uchar* row = img.scanLine(y);
            for (int x = 0; x < w; x++) {
                uint8_t colourIndex;

                colourIndex = row[x];
                // ===== IMPORT PALETTE =====

                if (ui->chkUsePalette->isChecked()) {
                    ct = img.colorTable();    // get the image data
                    // RGB of pixel *in GIF palette*
                    rf = (ct[colourIndex] >> 16) & 0xFF;
                    gf = (ct[colourIndex] >> 8)  & 0xFF;
                    bf =  ct[colourIndex]        & 0xFF;

                    int bestIndex = 0;
                    int bestDist  = INT_MAX;

                    // Compare to *current editor palette* CLUT[]
                    for (int pi = 0; pi < paletteDepth; pi++) {

                        int r2 = (CLUT[pi] >> 16) & 0xFF;
                        int g2 = (CLUT[pi] >> 8)  & 0xFF;
                        int b2 =  CLUT[pi]        & 0xFF;

                        int dr = rf - r2;
                        int dg = gf - g2;
                        int db = bf - b2;

                        int dist = dr*dr + dg*dg + db*db;

                        if (dist < bestDist) {
                            bestDist  = dist;
                            bestIndex = pi;
                        }
                    }
                    colourIndex = bestIndex;
                }
                icon_area[y][x] = colourIndex;
            }
        }
    }

    reSize();
    renderEditorCanvas();
    return true;
}


void MainWindow::onEditorScrollChanged(){
    scrollUpdateTimer->start(1);
}

void MainWindow::saveProjectIcon(const char *filename){
    // file structure
    FILE *f = fopen(filename, "wb");
    if(!f) {
        QMessageBox::warning(this, "Save Icon", "Cannot open file for writing!");
        return;
    }

    fwrite(&icon_width, sizeof(uint16_t), 1, f);
    fwrite(&icon_height, sizeof(uint16_t), 1, f);

    for(int y = 0; y < icon_height; y++) {
        fwrite(icon_area[y].data(), sizeof(uint8_t), icon_width, f);
    }

    fclose(f);
}

void MainWindow::loadProjectIcon(const char *filename){
    FILE *f = fopen(filename, "rb");
    if(!f) { QMessageBox::warning(this, "Load Icon", "Cannot open file!"); return; }

    // read width and height
    uint16_t w, h;
    fread(&w, sizeof(uint16_t), 1, f);
    fread(&h, sizeof(uint16_t), 1, f);
    icon_width = w;
    icon_height = h;

    ui->txtProjectImageWidth->setText(QString("%1").arg(icon_width));
    ui->txtProjectImageHeight->setText(QString("%1").arg(icon_height));

    // resize rows first
    icon_area.resize(icon_height);

    // resize each row (columns)
    for (auto &row : icon_area)
        row.resize(icon_width, 0);  // new cells initialized to 0, existing cells preserved

    reSize();

    // optionally resize your icon_area here if variable size
    for(int y = 0; y < h; y++) {
        fread(icon_area[y].data(), sizeof(uint8_t), w, f);
    }

    fclose(f);
    renderEditorCanvas();
}

// 0 clockwise, 1 = counter-clockwise
void MainWindow::rotateIcon(int direction){
    if(icon_width == 0 || icon_height == 0) return;
    std::vector<std::vector<uint8_t>> buffer = icon_area;

    int oldW = icon_width;
    int oldH = icon_height;

    int newW = oldH;
    int newH = oldW;

    icon_area.assign(newH, std::vector<uint8_t>(newW, 0));
    if(direction == 0) {  // clockwise
        for(int y = 0; y < newH; ++y){
            for(int x = 0; x < newW; ++x){
                icon_area[y][x] = buffer[oldH - 1 - x][y];
            }
        }
    }
    else if(direction == 1) { // counter-clockwise
        for(int y = 0; y < newH; ++y){
            for(int x = 0; x < newW; ++x){
                icon_area[y][x] = buffer[x][oldW - 1 - y];
            }
        }
    }

    icon_width  = newW;
    icon_height = newH;

    reSize();
    renderEditorCanvas();
}

uint32_t MainWindow::colourSqueeze(uint32_t srcColour){
    if(ui->rad24BitMode->isChecked())
        return srcColour; // 24-bit untouched

    uint8_t r = (srcColour >> 16) & 0xFF;
    uint8_t g = (srcColour >> 8) & 0xFF;
    uint8_t b = srcColour & 0xFF;

    // simulate RGB565 in 8 bits per channel
    r = (r & 0xF8);            // 5 bits
    g = (g & 0xFC);            // 6 bits
    b = (b & 0xF8);            // 5 bits

    return (r << 16) | (g << 8) | b;
}

void MainWindow::UpdatePrePaletteMixer(){
    uint32_t bit32col;
    uint16_t bit16col;

    pal.setColor(QPalette::Window, QColor(pltColourPreset[0], pltColourPreset[1], pltColourPreset[2])); // RGB
    ui->lblPaletteColour->setAutoFillBackground(true);   // must enable for background
    ui->lblPaletteColour->setPalette(pal);
    CLUT[numSelectedPaletteID] = (pltColourPreset[0] << 16) | (pltColourPreset[1] << 8) | (pltColourPreset[2]);

    renderPaletteCanvas();

    bit32col = CLUT[numSelectedPaletteID] | (255 << 24);    // making sure that the alpha channel is full solid

    // ---- RGB888 to RGB565 conversion ---- // TODO: convert the 32bit value (ignoring the alpha channel not important, to a RGB565 format)
    int r = pltColourPreset[0];
    int g = pltColourPreset[1];
    int b = pltColourPreset[2];
    bit16col =
        ((r & 0xF8) << 8) |   // 5 bits red
        ((g & 0xFC) << 3) |   // 6 bits green
        ((b & 0xF8) >> 3);    // 5 bits blue

    //ui->txtHEXcolour32Bit->setText();
    ui->txtHEXcolour32Bit->setText("0x" + QString("%1").arg(bit32col & 0xFFFFFF, 6, 16, QChar('0')).toUpper());
    ui->txtHEXcolour16Bit->setText("0x" + QString("%1").arg(bit16col & 0x00FFFF, 4, 16, QChar('0')).toUpper());

    ui->txtDECcolour32Bit->setText(QString("%1").arg(bit32col & 0xffffff));
    ui->txtDECcolour16Bit->setText(QString("%1").arg(bit16col & 0x00ffff));
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::SelectedPaletteID(){
    uint8_t r,g,b;
    ui->grpPaletteBox->setTitle(
        QString("Colour Palette Data: %1 (0x%2)")
            .arg(numSelectedPaletteID)
            .arg(QString("%1").arg(numSelectedPaletteID, 2, 16, QChar('0')).toUpper())
        );

    r = CLUT[numSelectedPaletteID] >> 16;
    g = CLUT[numSelectedPaletteID] >> 8;
    b = CLUT[numSelectedPaletteID] & 0xff;

    ui->txtPaletteR->setText(QString::number(r));
    ui->txtPaletteG->setText(QString::number(g));
    ui->txtPaletteB->setText(QString::number(b));

    ui->horizontalScrollBarR->setValue(r);
    ui->horizontalScrollBarG->setValue(g);
    ui->horizontalScrollBarB->setValue(b);
}

void MainWindow::doReassignedPalette(uint8_t targetPalID){
    int ix, iy;
    uint8_t paletteId;
    bReassignedPaletteIndex = false;
    //icon_area.assign(h, std::vector<uint8_t>(w, 0));
    //icon_width  = w;
    //icon_height = h;
    for(iy = 0; iy < icon_height; iy++){
        for(ix = 0; ix < icon_width; ix++){
            paletteId = icon_area[iy][ix];
            if(capturedPaletteIndex == paletteId){
                icon_area[iy][ix] = targetPalID;
            }
        }
    }
    ui->cmdReassignColour->setEnabled(true);
    renderEditorCanvas();
}

void MainWindow::doSwapPalette(uint8_t targetPalID){
    uint8_t br, bg, bb;
    uint32_t    backClut;

    //br = (CLUT[capturedPaletteIndex] >> 16) & 0xff;
    //bg = (CLUT[capturedPaletteIndex] >> 8) & 0xff;
    //bb = CLUT[capturedPaletteIndex] & 0xff;

    backClut = CLUT[capturedPaletteIndex];

    CLUT[capturedPaletteIndex] = CLUT[targetPalID];

    CLUT[targetPalID] = backClut;

    ui->cmdSwapColours->setEnabled(true);
    renderPaletteCanvas();
    renderEditorCanvas();
}

void setIconArea(int x, int y){
    if(x<0) return;
    if(y<0) return;
    if(x > icon_width-1) return;
    if(y > icon_height-1) return;
    icon_area[y][x] = numSelectedPaletteID;
}


// in your MainWindow class
bool grabbing = false;
QPoint grabStartMouse;      // screen pos when SPACE pressed
int grabStartScrollH, grabStartScrollV;

bool MainWindow::eventFilter(QObject *obj, QEvent *event){
    uint8_t r,g,b;

    // Palette Selector
    if(obj == ui->gfxPalleteSelect){
        if (event->type() == QEvent::KeyPress){ // capture current Colour Palette Index.
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            if (ke->isAutoRepeat()) return true; // ignore repeats
            printf("Palette keypress event\n");
            if (ke->key() == Qt::Key_Control){
                clickedIndex = numSelectedPaletteID;
                selectingCycle = true;
                waitingForEnd = false;
                cyclefrom = clickedIndex;
                //printf("Cycle Setting Start: Start pos %lu\n", clickedIndex);
                return true;
            }
        }

        if (event->type() == QEvent::KeyRelease){   // finished setting the Colour Cycle range.
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Control) {
                selectingCycle = false;
                waitingForEnd = false;
                return true;
            }
        }
    }

    if(obj == ui->gfxPalleteSelect->viewport()){

        if(event->type() == QEvent::Wheel){
        // Ignore wheel events
            return true; // this prevents the default scrolling
        }

        if (event->type() == QEvent::MouseButtonPress){
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            ui->gfxPalleteSelect->setFocus();


            QPoint viewPos = mouseEvent->pos(); // position in the QGraphicsView
            QPointF scenePos = ui->gfxPalleteSelect->mapToScene(viewPos); // map to scene coordinates

            int x = int(scenePos.x());
            int y = int(scenePos.y());

            SelectedX = x / PALETTE_BOX_HSIZE;
            SelectedY = y / PALETTE_BOX_VSIZE;

            renderPaletteCanvas();

            numSelectedPaletteID = (SelectedX + (SelectedY * PALETTE_WIDTH));

            if(selectingCycle == true){
                cycleto = numSelectedPaletteID;
                //cyclelen = 8;
                if(cycleto < cyclefrom){
                    std::swap(cycleto, cyclefrom);
                    //printf("New Cycle Setting From: %lu to: %ld\n", cyclefrom, cycleto);
                }// else
                   // printf("Cycle Setting End: pos %ld\n", cycleto);
            }

            // clicked on another colour - ONLY if selected another colour
            if(numPrevSelectedPaletteID != numSelectedPaletteID){
                for(int i=0; i<256; i++){
                    BACKUP_CLUT[i] = CLUT[i];
                }
                //printf("Commited new pallete\n");
            }
            numPrevSelectedPaletteID = numSelectedPaletteID;
            if(bReassignedPaletteIndex == true){
                bReassignedPaletteIndex = false;
                doReassignedPalette(numSelectedPaletteID);
            }

            if(bSwapColours == true){
                bSwapColours = false;
                doSwapPalette(numSelectedPaletteID);
            }

            if(bSpreadPalette == true){
                bSpreadPalette = false;
                doSpreadPalette(numSelectedPaletteID);
            }

            SelectedPaletteID();
            return true; // mark event as handled
        }
    }

    int xOffset = ui->scrEditorH->value();
    int yOffset = ui->scrEditorV->value();

    if(obj == ui->gfxEditor){   // keyboard interfacing blob
        if (event->type() == QEvent::KeyPress){
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            //if (ke->isAutoRepeat()) return true; // ignore repeats
            if(ke->key() == Qt::Key_Space && !grabbing && !ke->isAutoRepeat()){

                grabbing = true;

                setCursor(Qt::ClosedHandCursor);
                return true;
            }
        }
        if(event->type() == QEvent::KeyRelease){
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            if(ke->key() == Qt::Key_Space && grabbing && !ke->isAutoRepeat()){
                grabbing = false;
                setCursor(Qt::ArrowCursor);
                return true;
            }
        }


        return QObject::eventFilter(obj, event);
    }

    if(obj == ui->gfxEditor->viewport()){

        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (event->type() == QEvent::MouseButtonPress){
            if(grabbing) {
                if(mouseEvent->buttons() & Qt::LeftButton) {
                    grabStartMouse = QCursor::pos();
                    grabStartScrollH = ui->scrEditorH->value();
                    grabStartScrollV = ui->scrEditorV->value();
                }
            }
        }


        if (event->type() == QEvent::MouseMove){
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePos = ui->gfxEditor->mapToScene(mouseEvent->pos());
            ui->gfxEditor->setFocus();

            if(grabbing) {
                if(mouseEvent->buttons() & Qt::LeftButton) {
                    QPointF current = mouseEvent->globalPosition();

                    int newH = grabStartScrollH + (grabStartMouse.x() - current.x()) / icon_zoom;
                    int newV = grabStartScrollV + (grabStartMouse.y() - current.y()) / icon_zoom;

                    newH = std::clamp(newH, ui->scrEditorH->minimum(), ui->scrEditorH->maximum());
                    newV = std::clamp(newV, ui->scrEditorV->minimum(), ui->scrEditorV->maximum());

                    ui->scrEditorH->setValue(newH);
                    ui->scrEditorV->setValue(newV);

                    renderEditorCanvas();
                }
                return true;
            } else {

                hoverPixelX = int(scenePos.x()) / icon_zoom;
                hoverPixelY = int(scenePos.y()) / icon_zoom;

                if(hoverPixelX < 0) hoverPixelX = 0;
                if(hoverPixelY < 0) hoverPixelY = 0;
                if(hoverPixelX > editorViewPortWidth - 1)  hoverPixelX = editorViewPortWidth-1;
                if(hoverPixelY > editorViewPortHeight - 1) hoverPixelY = editorViewPortHeight-1;

                ui->lblCoords->setText(QString("Coords: x:%1, y:%2")
                    .arg(hoverPixelX, 4, 10, QChar('0'))
                    .arg(hoverPixelY, 4, 10, QChar('0'))
                );

                // Only draw if a button is pressed
                if(currentDrawMode == Plot){    // this will only work with Motion Drawing (plot)
                    if (mouseEvent->buttons() & Qt::LeftButton) {
                        icon_area[hoverPixelY + yOffset][hoverPixelX + xOffset] = numSelectedPaletteID;//currentPaletteID; // paint
                    } else if (mouseEvent->buttons() & Qt::RightButton) {
                        icon_area[hoverPixelY + yOffset][hoverPixelX + xOffset] = 0; // erase
                    }
                }

                if(captureXYStart==true){
                    ltcapturedX = ctcapturedX;
                    ltcapturedY = ctcapturedY;
                    readToolXY(&ctcapturedX, &ctcapturedY); // process where the capturey point is
                }
            }

            updateTimer->start(1);
            return true; // event handled
        } else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                //printf( "Left click at %lu\n",  mouseEvent->pos());

                // check if we're in a tool that needs point to point interaction
                // if() {
                if(!grabbing){
                    if(captureXYStart==true){
                        int xOffset = ui->scrEditorH->value();
                        int yOffset = ui->scrEditorV->value();
                        if(currentDrawMode == Line){
                            int x1 = ctcapturedX + xOffset;
                            int y1 = ctcapturedY + yOffset;

                            // Draw the actual line into the icon area
                            int x0 = capturedX + xOffset;
                            int y0 = capturedY + yOffset;

                            // Bresenham line (cells)
                            int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
                            int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
                            int err = dx + dy, e2;
                            int x = x0, y = y0;

                            while(true){
                                // Commit pixel color (for example, foreground color)
                                icon_area[y][x] = numSelectedPaletteID;

                                if(x == x1 && y == y1) break;
                                e2 = 2 * err;
                                if(e2 >= dy){ err += dy; x += sx; }
                                if(e2 <= dx){ err += dx; y += sy; }
                            }
                        }
                        if(currentDrawMode == Rect){


                            // Rectangle corners
                            int x0 = capturedX + xOffset;
                            int y0 = capturedY + yOffset;
                            int x1 = hoverPixelX + xOffset;
                            int y1 = hoverPixelY + yOffset;

                            // Ensure top-left -> bottom-right ordering
                            int left   = std::min(x0, x1);
                            int right  = std::max(x0, x1);
                            int top    = std::min(y0, y1);
                            int bottom = std::max(y0, y1);

                            // Draw top and bottom edges
                            for(int x = left; x <= right; x++){
                                setIconArea(x, top);
                                setIconArea(x, bottom);
                            }

                            // Draw left and right edges
                            for(int y = top; y <= bottom; y++){
                                setIconArea(left, y);
                                setIconArea(right, y);
                            }

                            if(ui->chkFillIt->isChecked()){
                                for(int x = left; x <= right; x++){
                                    for(int y = top; y <= bottom; y++)
                                        setIconArea(x, y);
                                }
                            }
                        }
                        if(currentDrawMode == Circle){
                            int cx = capturedX + xOffset;
                            int cy = capturedY + yOffset;
                            int radius = std::max(abs((hoverPixelX  + xOffset) - cx), abs((hoverPixelY  + yOffset) - cy));
                            bool fill = ui->chkFillIt->isChecked();

                            if(radius <= 3){ // small radius -> distance check
                                for(int y = cy - radius; y <= cy + radius; y++){
                                    for(int x = cx - radius; x <= cx + radius; x++){
                                        int dx = x - cx;
                                        int dy = y - cy;
                                        float dist = std::sqrt(dx*dx + dy*dy);
                                        // Outline
                                        if(dist >= radius - 0.5f && dist <= radius + 0.5f){
                                            setIconArea(x, y);
                                        }
                                        // Fill
                                        else if(fill && dist < radius - 0.5f){
                                            setIconArea(x, y);
                                        }
                                    }
                                }
                            } else { // large radius -> classic Midpoint Circle
                                int x = radius;
                                int y = 0;
                                int err = 0;
                                while(x >= y){
                                    setIconArea(cx + x, cy + y);
                                    setIconArea(cx + y, cy + x);
                                    setIconArea(cx - y, cy + x);
                                    setIconArea(cx - x, cy + y);
                                    setIconArea(cx - x, cy - y);
                                    setIconArea(cx - y, cy - x);
                                    setIconArea(cx + y, cy - x);
                                    setIconArea(cx + x, cy - y);

                                    // Fill: draw horizontal spans inside the circle
                                    if(fill){
                                        for(int fx = cx - x + 1; fx < cx + x; fx++){
                                            setIconArea(fx, cy + y);
                                            setIconArea(fx, cy - y);
                                        }
                                        for(int fx = cx - y + 1; fx < cx + y; fx++){
                                            setIconArea(fx, cy + x);
                                            setIconArea(fx, cy - x);
                                        }
                                    }

                                    y++;
                                    if(err <= 0){
                                        err += 2*y + 1;
                                    }
                                    if(err > 0){
                                        x--;
                                        err -= 2*x + 1;
                                    }
                                }
                            }
                        }
                    }
                }
                renderPaletteCanvas();
                captureXYStart = false;
            }
        } else if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            if (mouseEvent->button() == Qt::LeftButton) {
                //printf( "Left click at %lu\n",  mouseEvent->pos());

                // check if we're in a tool that needs point to point interaction
                // if() {
                captureXYStart = true;
                readToolXY(&ctcapturedX, &ctcapturedY); // process where the capturey point is
                readToolXY(&capturedX, &capturedY); // process where the capturey point is
                ltcapturedX = ctcapturedX;
                ltcapturedY = ctcapturedY;
                //printf("P2P Tool: capture point CX: %d, CY: %d\n", capturedX, capturedY);
                // }
                // else //its likely just to a pixel draw
                if(!grabbing)
                    ProcessLeftClickPaint();
                updateTimer->start(1);
            }
            else if (mouseEvent->button() == Qt::RightButton) {
                //printf( "Right click atlu\n",  mouseEvent->pos());
                icon_area[hoverPixelY + yOffset][hoverPixelX + xOffset] = 0;
                updateTimer->start(1);

            }
            else if (mouseEvent->button() == Qt::MiddleButton){
                numSelectedPaletteID = icon_area[hoverPixelY + yOffset][hoverPixelX + xOffset];
                SelectedX = numSelectedPaletteID % PALETTE_WIDTH;
                SelectedY = (numSelectedPaletteID / PALETTE_WIDTH) % PALETTE_HEIGHT;

                SelectedPaletteID();
                renderPaletteCanvas();
            }

            return true; // event handled
        } else if (event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            int newVal;

            int delta = wheelEvent->angleDelta().y(); // vertical wheel
            newVal = ui->scrEditorZoomVal->value();
            //newVal += delta;

            if(delta > 0) {
                //printf("Wheel up\n");
                newVal ++;
            } else if(delta < 0) {
                //printf("Wheel down\n");
                newVal --;
            }

            if(newVal>32) newVal = 32;
            if(newVal<1) newVal = 1;

            ui->scrEditorZoomVal->setValue(newVal);
            icon_zoom = ui->scrEditorZoomVal->value();
            ui->lblEditorZoomLevel->setText(QString("%1").arg(icon_zoom));
            reSize();
            renderEditorCanvas();

            return true; // event handled
        }
    }
    return QMainWindow::eventFilter(obj, event);
}


void MainWindow::ProcessLeftClickPaint(){
    int xOffset = ui->scrEditorH->value();
    int yOffset = ui->scrEditorV->value();
    //
    switch(currentDrawMode){
        case Plot:
            icon_area[hoverPixelY + yOffset][hoverPixelX + xOffset] = numSelectedPaletteID;
            break;

        case FloodFill:
            floodFill(hoverPixelX + xOffset, hoverPixelY + yOffset, numSelectedPaletteID);
            break;
        default:
            return;
    }
}

void MainWindow::readToolXY(int *rx, int *ry){
    int resx, resy;
    int xOffset = ui->scrEditorH->value();
    int yOffset = ui->scrEditorV->value();

    resx = hoverPixelX;// + xOffset;
    resy = hoverPixelY;// + yOffset;

    *rx = resx;
    *ry = resy;

}


void MainWindow::floodFill(int startX, int startY, uint8_t fillColor){
    int tick=0;
    if(startX < 0 || startX >= icon_width || startY < 0 || startY >= icon_height)
        return;

    uint8_t targetColor = icon_area[startY][startX];
    if (targetColor == fillColor) return;

    std::stack<QPoint> s;
    s.push(QPoint(startX, startY));

    while (!s.empty())
    {
        QPoint p = s.top();
        s.pop();

        int x = p.x();
        int y = p.y();

        if (x < 0 || x >= icon_width || y < 0 || y >= icon_height) continue;
        if (icon_area[y][x] != targetColor) continue;

        icon_area[y][x] = fillColor;

        // --- redraw step ---
        if(!ui->chkInstaFill->isChecked()){
            tick++;
            if(tick>250){
                renderEditorCanvas();
                QCoreApplication::processEvents(); // allow GUI to update
                QThread::msleep(3);               // slow down (ms)
                tick=0;
            }
        }

        // push neighbors
        //s.push(QPoint(x+1, y));
        //s.push(QPoint(x-1, y));
        //s.push(QPoint(x, y+1));
        //s.push(QPoint(x, y-1));

        // push vertical neighbors first (processed later)
        s.push(QPoint(x, y+1)); // down
        s.push(QPoint(x, y-1)); // up
        // push horizontal neighbors last (processed first)
        s.push(QPoint(x+1, y)); // right
        s.push(QPoint(x-1, y)); // left

    }

    renderEditorCanvas(); // update display after fill
}

int pendingScrollX = -1;
int pendingScrollY = -1;

void MainWindow::updateEditorScrollBars()
{
    int totalWidth  = icon_width;
    int totalHeight = icon_height ;

    int viewportWidth  = editorViewPortWidth ;
    int viewportHeight = editorViewPortHeight ;

    // horizontal scrollbar
    ui->scrEditorH->setRange(0, totalWidth - viewportWidth);
    ui->scrEditorH->setPageStep(viewportWidth);

    // vertical scrollbar
    ui->scrEditorV->setRange(0, totalHeight - viewportHeight);
    ui->scrEditorV->setPageStep(viewportHeight);
}

void MainWindow::reSize(){
    int WinXW, WinXH;
    int PWinXW, PWinXH;

    QWidget *wincontainer = ui->verticalLayoutWidget->parentWidget();
    QWidget *container = ui->verticalLayoutWidget;
    QWidget *vboxh = ui->vboxTextoutputv;
    QWidget *fonteditBox = ui->frmFontWorkbench;

    WinXW = wincontainer->width() - 2;
    WinXH = wincontainer->height() - 28;

    if(container){
        container->resize(WinXW, WinXH);  // or any size you want
    }
    if(vboxh){
        ui->outputTextView->resize(WinXW, WinXH);
        vboxh->resize(WinXW-16, WinXH-64);
    }
    if(fonteditBox){
        //ui->frmFontWorkbench->resize((WinXW))
        fonteditBox->resize(WinXW, WinXH);
    }

    PWinXW = ui->gfxEditor->width()  - 4;
    PWinXH = ui->gfxEditor->height() - 4;

    editorViewPortWidth = PWinXW / icon_zoom;
    editorViewPortHeight = PWinXH / icon_zoom;

    if(editorViewPortWidth > icon_width) editorViewPortWidth = icon_width;
    if(editorViewPortHeight > icon_height) editorViewPortHeight = icon_height;

    //printf("EditorCanv: W:%lu, H:%lu\n", editorViewPortWidth, editorViewPortHeight);
    updateEditorScrollBars();
    renderEditorCanvas();
}

void MainWindow::resizeEvent(QResizeEvent *event){
    QMainWindow::resizeEvent(event);
    reSize();
}

void MainWindow::renderEditorCanvas(){
    int visibleWidth  = editorViewPortWidth * icon_zoom;   // in pixels
    int visibleHeight = editorViewPortHeight * icon_zoom;

    int xOffset = ui->scrEditorH->value();
    int yOffset = ui->scrEditorV->value();

    editorImg = QImage(visibleWidth, visibleHeight, QImage::Format_RGB32);

    QRgb gridColor = gridEnabled ? QColor(gridRed, gridGreen, gridBlue).rgb() : 0; // choose color
    bool drawGrid = gridEnabled;

    for(int y = 0; y < visibleHeight; y++) {
        QRgb *scan = reinterpret_cast<QRgb*>(editorImg.scanLine(y));
        int imgY = (y) / icon_zoom;
        for(int x = 0; x < visibleWidth; x++) {
            int imgX = (x) / icon_zoom;

            QRgb base = colourSqueeze(CLUT[icon_area[imgY + yOffset][imgX + xOffset]]);
            scan[x] = base;

            // -------- GRID --------
            if(drawGrid) {
                if ((x % icon_zoom == 0 && x != 0) ||
                    (y % icon_zoom == 0 && y != 0)){
                    scan[x] = gridColor;
                }
            }
        }
    }

    if(!grabbing){
        // ---------------- DRAW HOVER BOX OVER THE TOP ---------------- //
        if(ui->scrEditorZoomVal->value() > 3){
            if (hoverPixelX >= 0 && hoverPixelY >= 0){
                int px = hoverPixelX * icon_zoom;
                int py = hoverPixelY * icon_zoom;

                int inner = 1;             // offset from gridline
                int thick = 2;             // border thickness

                int left   = px + inner;
                int right  = px + icon_zoom - inner ;
                int top    = py + inner;
                int bottom = py + icon_zoom - inner ;

                // Clamp to viewport (for safety)
                if (left >= 0 && right < visibleWidth &&
                    top >= 0 && bottom < visibleHeight){
                    // Invert border color
                    auto invert = [&](int xx, int yy){
                        QRgb *scan = reinterpret_cast<QRgb*>(editorImg.scanLine(yy));
                        scan[xx] = 0xFFFFFFFF - scan[xx];
                    };

                    // TOP
                    for (int t = 0; t < thick; t++) for (int x = left; x <= right; x++) invert(x, top + t);

                    // BOTTOM
                    for (int t = 0; t < thick; t++) for (int x = left; x <= right; x++) invert(x, bottom - t);

                    // LEFT
                    for (int t = 0; t < thick; t++) for (int y = top; y <= bottom; y++) invert(left + t, y);

                    // RIGHT
                    for (int t = 0; t < thick; t++) for (int y = top; y <= bottom; y++) invert(right - t, y);
                }
            }
        }


        if(captureXYStart==true){
            //ltcapturedX = ctcapturedX;
            //ltcapturedY = ctcapturedY;
            //readToolXY(&ctcapturedX, &ctcapturedY); // process where the capturey point is
            //printf("P2P Tool: Target point TX: %d, TY: %d\n", ctcapturedX, ctcapturedY);
            if(currentDrawMode == Line){
                auto invertCell = [&](int cellX, int cellY){
                    int startX = cellX * icon_zoom;
                    int startY = cellY * icon_zoom;
                    for(int yy = 0; yy < icon_zoom; yy++){
                        int py = startY + yy;
                        if(py < 0 || py >= visibleHeight) continue;
                        QRgb *scan = reinterpret_cast<QRgb*>(editorImg.scanLine(py));
                        for(int xx = 0; xx < icon_zoom; xx++){
                            int px = startX + xx;
                            if(px < 0 || px >= visibleWidth) continue;
                            //scan[px] = 0xFFFFFFFF - scan[px]; // invert
                            scan[px] = CLUT[numSelectedPaletteID];
                        }
                    }
                };

                int x0cell = capturedX;
                int y0cell = capturedY;
                int x1cell = hoverPixelX;
                int y1cell = hoverPixelY;

                // Bresenham using cells
                int dx = abs(x1cell - x0cell), sx = x0cell < x1cell ? 1 : -1;
                int dy = -abs(y1cell - y0cell), sy = y0cell < y1cell ? 1 : -1;
                int err = dx + dy, e2;

                int x = x0cell, y = y0cell;
                while(true){
                    invertCell(x, y);        // invert a full cell instead of 1 pixel
                    if(x == x1cell && y == y1cell) break;
                    e2 = 2 * err;
                    if(e2 >= dy){ err += dy; x += sx; }
                    if(e2 <= dx){ err += dx; y += sy; }
                }
            }

            if(currentDrawMode == Rect){
                auto invertCell = [&](int cellX, int cellY){
                    int startX = cellX * icon_zoom;
                    int startY = cellY * icon_zoom;
                    for(int yy = 0; yy < icon_zoom; yy++){
                        int py = startY + yy;
                        if(py < 0 || py >= visibleHeight) continue;
                        QRgb *scan = reinterpret_cast<QRgb*>(editorImg.scanLine(py));
                        for(int xx = 0; xx < icon_zoom; xx++){
                            int px = startX + xx;
                            if(px < 0 || px >= visibleWidth) continue;
                            //scan[px] = 0xFFFFFFFF - scan[px]; // invert for ghost
                            scan[px] = CLUT[numSelectedPaletteID];
                        }
                    }
                };

                // Rectangle corners
                int x0 = capturedX;
                int y0 = capturedY;
                int x1 = hoverPixelX;
                int y1 = hoverPixelY;

                // Ensure top-left -> bottom-right ordering
                int left   = std::min(x0, x1);
                int right  = std::max(x0, x1);
                int top    = std::min(y0, y1);
                int bottom = std::max(y0, y1);

                // Draw top and bottom edges
                for(int x = left; x <= right; x++){
                    invertCell(x, top);
                    invertCell(x, bottom);
                }

                // Draw left and right edges
                for(int y = top; y <= bottom; y++){
                    invertCell(left, y);
                    invertCell(right, y);
                }

                if(ui->chkFillIt->isChecked()){
                    for(int x = left; x <= right; x++){
                        for(int y = top; y <= bottom; y++)
                            invertCell(x, y);
                    }
                }
            }


            if(currentDrawMode == Circle){
                auto invertCell = [&](int cellX, int cellY){
                    int startX = cellX * icon_zoom;
                    int startY = cellY * icon_zoom;
                    for(int yy = 0; yy < icon_zoom; yy++){
                        int py = startY + yy;
                        if(py < 0 || py >= visibleHeight) continue;
                        QRgb *scan = reinterpret_cast<QRgb*>(editorImg.scanLine(py));
                        for(int xx = 0; xx < icon_zoom; xx++){
                            int px = startX + xx;
                            if(px < 0 || px >= visibleWidth) continue;
                            scan[px] = CLUT[numSelectedPaletteID]; // or invert for ghost
                        }
                    }
                };

                int cx = capturedX;
                int cy = capturedY;
                int radius = std::max(abs(hoverPixelX - cx), abs(hoverPixelY - cy));

                if(radius <= 3){ // small radius -> distance check
                    for(int y = cy - radius; y <= cy + radius; y++){
                        for(int x = cx - radius; x <= cx + radius; x++){
                            int dx = x - cx;
                            int dy = y - cy;
                            float dist = std::sqrt(dx*dx + dy*dy);
                            if(dist >= radius - 0.5f && dist <= radius + 0.5f){
                                invertCell(x, y);
                            }
                            // Fill: inside the radius
                            else if(ui->chkFillIt->isChecked() && dist < radius - 0.5f){
                                invertCell(x, y);
                            }
                        }
                    }
                } else { // large radius -> classic Midpoint Circle
                    int x = radius;
                    int y = 0;
                    int err = 0;
                    while(x >= y){
                        invertCell(cx + x, cy + y);
                        invertCell(cx + y, cy + x);
                        invertCell(cx - y, cy + x);
                        invertCell(cx - x, cy + y);
                        invertCell(cx - x, cy - y);
                        invertCell(cx - y, cy - x);
                        invertCell(cx + y, cy - x);
                        invertCell(cx + x, cy - y);


                        if(ui->chkFillIt->isChecked()){
                            // Fill horizontal spans between the left/right of the circle
                            for(int fillX = cx - x + 1; fillX < cx + x; fillX++){
                                invertCell(fillX, cy + y);
                                invertCell(fillX, cy - y);
                            }
                            for(int fillX = cx - y + 1; fillX < cx + y; fillX++){
                                invertCell(fillX, cy + x);
                                invertCell(fillX, cy - x);
                            }
                        }

                        y++;
                        if(err <= 0){
                            err += 2*y + 1;
                        }
                        if(err > 0){
                            x--;
                            err -= 2*x + 1;
                        }
                    }
                }
            }
        }
    }
    editorPixmap->setPixmap(QPixmap::fromImage(editorImg));
}


void MainWindow::renderPaletteCanvas(){
    const int totalWidth  = PALETTE_WIDTH  * PALETTE_BOX_HSIZE;  // 16 * 16 = 256
    const int totalHeight = PALETTE_HEIGHT * PALETTE_BOX_VSIZE;  // 16 * 16 = 256

    //const int SelectedX = 1;    // this will be clickable later
    //const int SelectedY = 2;

    const int GridX = SelectedX * PALETTE_BOX_HSIZE;
    const int GridY = SelectedY * PALETTE_BOX_VSIZE;

    for (int y = 0; y < totalHeight; y++){
        QRgb *scan = reinterpret_cast<QRgb*>(paletteImg.scanLine(y));

        // which palette row?
        int tileY = y / PALETTE_BOX_VSIZE;

        for (int x = 0; x < totalWidth; x++){
            // which palette column?
            int tileX = x / PALETTE_BOX_HSIZE;

            // palette index (0–255)
            int colorIndex = tileY * PALETTE_WIDTH + tileX;

            // draw the pixel
            scan[x] = colourSqueeze(CLUT[colorIndex]);
        }
    }

    // TODO: a double thick holow inverted box to show its selected.
    // Draw double-thick hollow inverted box around selected tile
    for (int dy = 0; dy < PALETTE_BOX_VSIZE; dy++) {
        for (int dx = 0; dx < PALETTE_BOX_HSIZE; dx++) {
            bool onBorder =
                dx < 2 || dx >= PALETTE_BOX_HSIZE - 2 ||    // left/right border 2px
                dy < 2 || dy >= PALETTE_BOX_VSIZE - 2;      // top/bottom border 2px

            if (onBorder) {
                int px = GridX + dx;
                int py = GridY + dy;

                QRgb *scan = reinterpret_cast<QRgb*>(paletteImg.scanLine(py));
                scan[px] = ~scan[px];   // invert the pixel
            }
        }
    }
    palettePixmap->setPixmap(QPixmap::fromImage(paletteImg));
}
