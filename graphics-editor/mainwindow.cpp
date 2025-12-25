#include "mainwindow.h"
#include "fonteditor.h"
#include "ui_mainwindow.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QRegularExpression>
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
#include <QByteArray>

// clipboard
#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

#include <stdio.h>

#include "projectheader.h"

#define PALETTE_BOX_HSIZE   16
#define PALETTE_BOX_VSIZE   17
#define PALETTE_WIDTH       PALETTE_BOX_HSIZE
#define PALETTE_HEIGHT      PALETTE_BOX_VSIZE


enum class KeyBinding : int {
    kscLoadProject      = Qt::Key_L,    // load project
    kscSaveProject      = Qt::Key_S,    // save project


    kscSwapScreen       = Qt::Key_J,    // switch between both the scratch pad and the main edit screen
    kscUndo             = Qt::Key_Z,    // the undo feature (its not as crap as it sounds!!)

    kscBrushRotateCC    = Qt::Key_Q,    // counter clock rotate Brush
    kscBrushRotateCW    = Qt::Key_E,    // clockwise rotate Brush
    kscBrushFlipX       = Qt::Key_X,    // flip brush Horizontally  (left to right)
    kscBrushFlipY       = Qt::Key_Y,    // flip brush Virtially     ( top to bottom)

    kscPenSize1         = Qt::Key_1,    // Set PenSize to x1    // nothing too fancy
    kscPenSize2         = Qt::Key_2,    // Set PenSize to x2
    kscPenSize3         = Qt::Key_3,    // Set PenSize to x4
    kscPenSize4         = Qt::Key_4,    // Set PenSize to x6
    kscPenSize5         = Qt::Key_5,    // Set PenSize to x8

    kscBlendDecrease    = Qt::Key_BracketLeft,    // decrease de blending!
    kscBlendIncrease    = Qt::Key_BracketRight,   // increase de blending!

    kscToolSelectPlot   = Qt::Key_F1,   // Plotter
    kscToolSelectLine   = Qt::Key_F2,   // Line draw
    kscToolSelectPen    = Qt::Key_F3,   // Pen draw
    kscToolSelectSpray  = Qt::Key_F4,   // Spray Can

    kscZoomeOut         = Qt::Key_Minus,    // zoom out screen
    kscZoomIn           = Qt::Key_Equal,     // zoom in screen

    kscPanZoom          = Qt::Key_Space,    // Pan Zoom
};


#define PALETTE_VRAM_SIZE   (PALETTE_WIDTH * PALETTE_BOX_HSIZE * PALETTE_HEIGHT * PALETTE_BOX_VSIZE)
int SelectedX = 1;    // this will be clickable later
int SelectedY = 0;
int colourCycleSpeed = 0;
uint8_t     numSelectedPaletteID = 1, numPrevSelectedPaletteID = 1;
uint8_t     numSelectedBackPaletteID = 0;
int         paletteDepth    = 256;
uint8_t     pltColourPreset[3] = {0,0,0};

uint8_t     capturedPaletteIndex;
bool        bReassignedPaletteIndex = false;
bool        bSwapColours    = false;
bool        bSpreadPalette  = false;

QPalette    pal;
QTimer      *tmrColourCycle;
QString     ProjectFilename = "untitled.icn";
QString     PaletteFilename = "untitled.pal";

int         currentCellID = 0;

int         iAnimationFPS;
QTimer      *tmrCellAnimator;

//chkCyclePaletteDraw#include "projectheader.h"
int         cyclePaletteID = 0; // used for when drawing the index + numSelectedPaletteID
int         cyclePaletteStepping = 0;   // used to control the division of steps before next cyclePaletteID increment.
int         cyclefrom = 80, cycleto = 87, cyclelength = 8;
bool        selectingCycle = false;   // true when CTRL is held
bool        waitingForEnd = false;    // true after choosing cycle_from
int         clickedIndex = 256; // way off grid

bool        selectingGradientRange = false;    // true when Left ALT is held
bool        waitingForEndGradient = false;
int         GradientRangeFrom = 160;
int         GradientRangeTo = 165;

bool        bMouseLeftRight    = true; // true if it is left
bool        bMouseButtonDown   = false;
bool        bShiftKey          = false;    // used for holding brush draw ;)

// for previewing new primatives, lines, circles, rectangles
bool        captureXYStart = false;
int         ctcapturedX = -1, ctcapturedY = -1;  // current location target
int         ltcapturedX = -1, ltcapturedY = -1;  // last location target
int         capturedX = -1, capturedY = -1;

bool paletteRestrictor = false;
int paletteRangerOffset, paletteRangerLength;

int hoverPixelX = -1;
int hoverPixelY = -1;


bool gridEnabled        = false;
#define gridRed          128
#define gridGreen        128
#define gridBlue         128

// default settings at startup
uint16_t    icon_zoom       = 3;
// cell divide
uint16_t    cell_width      = 32;
uint16_t    cell_height     = 32;

uint16_t    icon_width      = cell_width * 10; // Sidbox 4.3 Screen dimentions;
uint16_t    icon_height     = cell_height * 8;

uint16_t    icon_old_width  = icon_width;
uint16_t    icon_old_height = icon_height;

int editorViewPortWidth     = 8;   // editor width grid
int editorViewPortHeight    = 8;

bool        bPlayAnimations = false;

//uint8_t icon_area[8][8] = {0};  // all to paletteID 0
bool bEditorPage            = 0;
std::vector<std::vector<uint8_t>> icon_area_front;   // this is the current edit screen
std::vector<std::vector<uint8_t>> icon_area_back;   // this is the current edit screen
std::vector<std::vector<uint8_t>> icon_area_scratchpage;    // this is the scratch page

std::vector<std::vector<uint8_t>> *active_icon_area = &icon_area_front;
std::vector<std::vector<uint8_t>> *icon_area = active_icon_area;

// undo/redo
std::vector<std::vector<uint8_t>> icon_area_backup; // this is the one for if we ever "undo"
std::vector<std::vector<uint8_t>> icon_area_redo;   // basically this is the area to redo things



// only rastering 8x8 pixel font, nothing advanced
uint8_t fontedit_area[8][8];


// prototype calls
void loadDefaultFont();


enum ImageExportConfig {
    ExportRLE           = 1,  // chkExportRLE
    ExportSidBoxVRAM    = 2,  // chkExportSBVRAM
};
uint16_t    ExportBits  = 0;    // just basic bits


#define DrawUIMode_InitButton       0x01    // when the mouse  is initially clicked
#define DrawUIMode_LeftMouseButton  0x10    // used when the mouse is moving while button L is held down
#define DrawUIMode_RightMouseButton 0x20    // used when the mouse is moving while button R is held down



uint32_t CLUTF[256] = {
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

uint32_t CLUTB[256] = { // back panel palette (NOW we can have TWO sets!!!
    0x000A0E08, 0xFF0C120A, 0xFF141209, 0xFF11180C, 0xFF141914, 0xFF1D1208, 0xFF1D1D15, 0xFF1E160C,
    0xFF141E13, 0xFF1F1C0B, 0xFF142214, 0xFF14220B, 0xFF1D2414, 0xFF1F271D, 0xFF1D2A14, 0xFF142A1C,
    0xFF2C1A0C, 0xFF2D2116, 0xFF2E1506, 0xFF2E291E, 0xFF2F210B, 0xFF222F1C, 0xFF302812, 0xFF243025,
    0xFF312F14, 0xFF2D3124, 0xFF373723, 0xFF24372C, 0xFF243724, 0xFF2C392D, 0xFF2C3924, 0xFF35392D,
    0xFF3C1909, 0xFF3C200B, 0xFF3D2911, 0xFF353E2F, 0xFF3E2115, 0xFF383E24, 0xFF3E291D, 0xFF243E2F,
    0xFF3F3120, 0xFF403012, 0xFF2C432E, 0xFF374324, 0xFF443E23, 0xFF384433, 0xFF444324, 0xFF443821,
    0xFF443C2D, 0xFF224433, 0xFF444433, 0xFF2C4A34, 0xFF344A35, 0xFF3E4B34, 0xFF2B4C3D, 0xFF414C3D,
    0xFF4D3926, 0xFF4D3117, 0xFF4D3918, 0xFF4E2812, 0xFF4E493C, 0xFF4E3025, 0xFF4E412D, 0xFF504F32,
    0xFF50291C, 0xFF502110, 0xFF504830, 0xFF503F22, 0xFF4D513F, 0xFF3F5233, 0xFF523F14, 0xFF295241,
    0xFF345243, 0xFF43523F, 0xFF534621, 0xFF44573A, 0xFF515733, 0xFF435745, 0xFF4D5843, 0xFF345847,
    0xFF285945, 0xFF4F594C, 0xFF5C492F, 0xFF5C412D, 0xFF5C5843, 0xFF5C5733, 0xFF5C4220, 0xFF5C4821,
    0xFF5C513D, 0xFF5C4F2D, 0xFF5E291B, 0xFF515E32, 0xFF445E40, 0xFF5E2F1B, 0xFF4F5F44, 0xFF5C5F43,
    0xFF40604D, 0xFF52604D, 0xFF603125, 0xFF61382C, 0xFF613721, 0xFF2C614B, 0xFF34644F, 0xFF64491A,
    0xFF5C6452, 0xFF654A28, 0xFF65513D, 0xFF655023, 0xFF65512F, 0xFF2C664D, 0xFF664936, 0xFF426654,
    0xFF546651, 0xFF546642, 0xFF674134, 0xFF676652, 0xFF5C6742, 0xFF684024, 0xFF69604D, 0xFF69593F,
    0xFF6A5724, 0xFF6A6042, 0xFF666B53, 0xFF6B582F, 0xFF526B56, 0xFF696B43, 0xFF5C6B57, 0xFF346B54,
    0xFF6C5E31, 0xFF656D5C, 0xFF3D6D58, 0xFF6E3526, 0xFF6F3A28, 0xFF5C725C, 0xFF68735C, 0xFF3C735B,
    0xFF745A3F, 0xFF744A44, 0xFF756032, 0xFF6B7564, 0xFF44755E, 0xFF75472E, 0xFF4E7562, 0xFF755930,
    0xFF766D5C, 0xFF77614C, 0xFF775144, 0xFF78775B, 0xFF786242, 0xFF797258, 0xFF79502E, 0xFF79664C,
    0xFF796C4F, 0xFF757965, 0xFF7A3F27, 0xFF7A7241, 0xFF427A60, 0xFF7A6634, 0xFF6C7B65, 0xFF7B6B40,
    0xFF7B774A, 0xFF7C3A24, 0xFF7C6B34, 0xFF5C7D6C, 0xFF4E7D67, 0xFF757E6A, 0xFF7C7E55, 0xFF6C826C,
    0xFF4C826C, 0xFF79836C, 0xFF54846D, 0xFF83845A, 0xFF84664D, 0xFF846635, 0xFF846E4D, 0xFF846C3E,
    0xFF84613A, 0xFF847241, 0xFF5E8572, 0xFF855431, 0xFF7B8674, 0xFF86846A, 0xFF867E65, 0xFF858674,
    0xFF867965, 0xFF885A35, 0xFF5F8A75, 0xFF7B8A6C, 0xFF6C8A7A, 0xFF848A69, 0xFF8B7856, 0xFF548B73,
    0xFF7C8B74, 0xFF8B7847, 0xFF8C6E4C, 0xFF8C724D, 0xFF8C6B2F, 0xFF8C7E49, 0xFF8C723B, 0xFF8C8356,
    0xFF8C662F, 0xFF848D77, 0xFF8D5F3C, 0xFF8D8D77, 0xFF62907A, 0xFF6C917E, 0xFF928B69, 0xFF759280,
    0xFF8D927C, 0xFF947A44, 0xFF947D5C, 0xFF947E4B, 0xFF948A57, 0xFF947953, 0xFF8D967C, 0xFF649780,
    0xFF6C9882, 0xFF988450, 0xFF849886, 0xFF789987, 0xFF99866C, 0xFF8F9984, 0xFF9A8562, 0xFF9C6644,
    0xFF9C6A44, 0xFF9D9984, 0xFF949E84, 0xFF9E8C55, 0xFF6B9F8A, 0xFF7A9F8A, 0xFF94A08C, 0xFF84A08F,
    0xFFA18E69, 0xFF9CA28C, 0xFFA29F7C, 0xFFA29174, 0xFFA39675, 0xFFA4965C, 0xFFA49259, 0xFFA4966C,
    0xFFA48664, 0xFF82A692, 0xFF9DA691, 0xFFA3AA94, 0xFF91AA96, 0xFFA0AA8B, 0xFFAC966B, 0xFFAC926B,
    0xFFACAB8E, 0xFFAC9674, 0xFFAC9274, 0xFFACA285, 0xFFADA68C, 0xFFB0A06B, 0xFFB0A278, 0xFFB4A674
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
uint32_t BACKUP_CLUTF[256];
uint32_t BACKUP_CLUTB[256];
uint32_t *CBACKUP_CLUT = BACKUP_CLUTF;
uint32_t *CCLUT = CLUTF;

uint8_t  CLUTRGB[3][256];     // ready split colours

uint8_t palleteCanvas[PALETTE_VRAM_SIZE];

QTimer *updateTimer;
QTimer *scrollUpdateTimer;

extern unsigned char SYSFONT[256][8];

// spray can worker
QTimer *tmrSprayCanTimer;           // the spray can deposite rate
int     iSpraySX, iSpraySY;         // the current mouse location + scroll offset
bool    bSprayingTheCan = false;    // this is going to be a trigger needed for spray draw routine
int     iSprayRate      = 90;       // default rate
bool    bSprayDraw      = 1;        // 1 is drawing, 0 is clearing

// draw text worker
int     iTextWidth      = 1;        // width of the text
int     iTextHeight     = 1;        // height of the text

// PEN system is the host for Spray Can and simple primative
bool    bPenShapeCircle = true;
int     iPenShapeSize   = 1;        // default pen size for spray and pen actions
bool    bFillToolIn     = false;    // weather the cirlce/box tool filles in (MINCE PIE'D!)


int blendopaque = 100;  // 0..100

// Copying and draw brushing
std::vector<std::vector<uint8_t>> icon_copy_area;   // this is the buffer that holes the copied area
                                                    // we'll use this to draw from this to the icon_area ;)
bool    bCapturingCopyArea  = false;
bool    bGrabbedCopyStart = false;
int     iCapturedCopyX, iCapturedCopyY;
int     iTargetCopyX, iTargetCopyY;
char    cBrushHandleMode = cHandleMM;
int     iCopyWidth      = 0;        // nothing in the buffer, so REALLY make sure this is checked
int     iCopyHeight     = 0;

// flood fill options
bool    bDithered = false;
bool    bNoisyDither = false; // toggle noisy dithering

bool    gradientDragging = false;
bool    bMirroredGradient = false;
bool    bReversedGradient = false;

int     gradStartX  = 0;
int     gradStartY  = 0;
int     gradLength  = 0;
float   gradAngle   = 0.0f;

#define PBankID_Front   0
#define PBankID_Back    1

int     paletteBankID   = PBankID_Front;    //


enum DrawMode {
    noDrawing,
    Plot,
    Line,
    Pen,
    SprayCan,
    Rect,
    Circle,
    FloodFill,
    FloodFillGradient,
    DrawText,
    CopyBrush,  // Copy meaning we're capturing
    PasteBrush, // THEN it will turn to this when we let go of the mouse button
} currentDrawMode = Plot;
DrawMode lastDrawMode = currentDrawMode; // store enum type directly

enum ResizeMode {
    Resample,       // averaging
    NearestNeighbor // simple pixel copy
};

enum FloodFillType {
    FillLinear,
    FillDiamond,
    FillCircles,
    FillFromBrush
};

int iFillType = FillLinear;

enum BrushChange {
    brushflipX,
    brushflipY,
    brushrotateR,
    brushrotateL
};

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
    fontEditor = new FontEditor(ui->gfxFontSelector, ui->gfxFontEditbox, ui->lblSelectedFont, this);  // the object of FontEditor
    fontEditor->RenderFontSelect();


    reSizeEditorArray(icon_width, icon_height);

    //editorImg = QImage(32, 32, QImage::Format_RGB32);
    editorPixmap = editorScene->addPixmap(QPixmap::fromImage(editorImg));
    renderEditorCanvas();

    for(int i=0; i<256; i++){
        BACKUP_CLUTF[i] = CLUTF[i];
        BACKUP_CLUTB[i] = CLUTB[i];
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
            uint32_t ca = CCLUT[a];
            uint32_t cb = CCLUT[b];

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
            newCLUT[i] = CCLUT[indices[i]];
        }
        for (int i = 0; i < 256; ++i) {
            CCLUT[i] = newCLUT[i];
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
        CCLUT[numSelectedPaletteID] = lngColour;

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

    int r, g, b;

    r = (CCLUT[numSelectedPaletteID] >> 16) & 0xff;
    g = (CCLUT[numSelectedPaletteID] >> 8) & 0xff;
    b = (CCLUT[numSelectedPaletteID]) & 0xff;

    pal = ui->lblPaletteColour->palette();
    pal.setColor(QPalette::Window, QColor(r, g, b)); // RGB
    ui->lblPaletteColour->setAutoFillBackground(true);   // must enable for background
    ui->lblPaletteColour->setPalette(pal);

    ui->lblPaletteForeSelect->setAutoFillBackground(true);   // must enable for background
    ui->lblPaletteForeSelect->setPalette(pal);

    r = (CCLUT[numSelectedBackPaletteID] >> 16) & 0xff;
    g = (CCLUT[numSelectedBackPaletteID] >> 8) & 0xff;
    b = (CCLUT[numSelectedBackPaletteID]) & 0xff;

    pal = ui->lblPaletteBackSelect->palette();
    pal.setColor(QPalette::Window, QColor(r, g, b)); // RGB
    ui->lblPaletteBackSelect->setAutoFillBackground(true);   // must enable for background
    ui->lblPaletteBackSelect->setPalette(pal);


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
        r = CBACKUP_CLUT[numSelectedPaletteID] >> 16;
        g = CBACKUP_CLUT[numSelectedPaletteID] >> 8;
        b = CBACKUP_CLUT[numSelectedPaletteID] & 0xff;

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

    ui->scrBrushBlend->setValue(blendopaque);

    connect(ui->scrBrushBlend, &QScrollBar::valueChanged, this, [this](){
        blendopaque = ui->scrBrushBlend->value();
        ui->lblBlendValue->setText(QString("%1%").arg(blendopaque));
    });

    connect(allDoneTimer, &QTimer::timeout, this, &MainWindow::reSize);
    //icon_width
    ui->txtProjectImageWidth->setText(QString("%1").arg(icon_width));
    ui->txtProjectImageHeight->setText(QString("%1").arg(icon_height));

    // CELL SYSTEM STARTUP
    animationScene = new QGraphicsScene(this);
    ui->gvAnimFrameView->setScene(animationScene);
    animationPixmap = animationScene->addPixmap(QPixmap::fromImage(animationImg));

    ui->lblCellSizeWarning->hide();
    ui->txtCellWidth->setText(QString("%1").arg(cell_width));
    ui->txtCellHeight->setText(QString("%1").arg(cell_height));
    connect(ui->txtCellWidth, &QLineEdit::textChanged, this, [this](){
        cell_width = ui->txtCellWidth->text().toInt();
        if (cell_width <= 0) return;
        renderEditorCanvas();
    });
    connect(ui->txtCellHeight, &QLineEdit::textChanged, this, [this](){
        cell_height = ui->txtCellHeight->text().toInt();
        if (cell_height <= 0) return;
        renderEditorCanvas();
    });

    // Cell Animator Setup
    ui->frmCellAnimator->hide();    // normally hiddeneded
    connect(ui->cmdOpenCellAnimator, &QPushButton::clicked, this, [this](){
        ui->frmCellAnimator->show();    // normally hiddeneded
        ui->frmCellAnimator->raise();
    });

    connect(ui->cmdAnimatorClose, &QPushButton::clicked, this, [this](){
        ui->frmCellAnimator->hide();    // normally hiddeneded
    });

    connect(ui->cmdPlayAnimations, &QPushButton::clicked, this, [this](){
        renderAnimatorCanvas();
    });


    //currentCellID =
    ui->scrShowCellID->setMaximum(0);
    connect(ui->scrShowCellID, &QScrollBar::valueChanged, this, [this](){
        currentCellID = ui->scrShowCellID->value();
        renderAnimatorCanvas();
    });

    //QTimer *allDoneTimer;
    //allDoneTimer = new QTimer(this);

    tmrCellAnimator = new QTimer(this);
    tmrCellAnimator->stop();


    connect(ui->cmdPlayAnimations, &QPushButton::clicked, this, [this](){
        bPlayAnimations = true;
        tmrCellAnimator->start(iAnimationFPS);
    });

    connect(ui->cmdStopAnimations, &QPushButton::clicked, this, [this](){
        bPlayAnimations = false;
        tmrCellAnimator->stop();
    });


    iAnimationFPS = 1000 / 5;
    connect(ui->scrAnimFPS, &QScrollBar::valueChanged, this, [this](){
        int fps = ui->scrAnimFPS->value();
        if (fps <= 0) fps = 1;

        iAnimationFPS = 1000 / fps;
        ui->lblAnimFPS->setText(QString("%1 fps").arg(fps));

        if (bPlayAnimations) {
            tmrCellAnimator->start(iAnimationFPS); // restart with new interval
        }

    });

    renderAnimatorCanvas();

    connect(tmrCellAnimator, &QTimer::timeout, this, [this](){
        currentCellID++;

        if (currentCellID > ui->scrShowCellID->maximum())
            currentCellID = 0;

        ui->scrShowCellID->setValue(currentCellID); // keeps UI in sync
        renderAnimatorCanvas();

    });



    allDoneTimer->start(100);

    connect(ui->chkShowGrid, &QCheckBox::clicked, this, [this](){
        gridEnabled = ui->chkShowGrid->isChecked();
        renderEditorCanvas();
    });

    ui->lblEditorZoomLevel->setText(QString("%1").arg(icon_zoom));  // load the default value
    ui->scrEditorZoomVal->setValue(icon_zoom);
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
        bool bCellMode = ui->chkCellDivider->isChecked();

        uint8_t topRow[icon_width];


        if(!bCellMode){
            for(int x = 0; x < icon_width; x++)
                topRow[x] = (*icon_area)[0][x];

            for(int y = 0; y < icon_height - 1; y++)
                for(int x = 0; x < icon_width; x++)
                    (*icon_area)[y][x] = (*icon_area)[y + 1][x];

            for(int x = 0; x < icon_width; x++)
                (*icon_area)[icon_height - 1][x] = topRow[x];
        } else {

            // we'll have to do each of these as cells!
            int cellsPerRow    = icon_width  / cell_width;
            int cellsPerColumn = icon_height / cell_height;

            // Temporary buffer for one row of a cell
            std::vector<uint8_t> topRow(cell_width);

            for (int cy = 0; cy < cellsPerColumn; cy++) {
                for (int cx = 0; cx < cellsPerRow; cx++) {

                    int cellStartX = cx * cell_width;
                    int cellStartY = cy * cell_height;

                    // Save top row of this cell
                    for (int x = 0; x < cell_width; x++) {
                        topRow[x] = (*icon_area)[cellStartY][cellStartX + x];
                    }

                    // Shift cell pixels up
                    for (int y = 0; y < cell_height - 1; y++) {
                        for (int x = 0; x < cell_width; x++) {
                            (*icon_area)[cellStartY + y][cellStartX + x] =
                                (*icon_area)[cellStartY + y + 1][cellStartX + x];
                        }
                    }

                    // Wrap top row to bottom of this cell
                    for (int x = 0; x < cell_width; x++) {
                        (*icon_area)[cellStartY + cell_height - 1][cellStartX + x] =
                            topRow[x];
                    }
                }
            }
        }


        renderEditorCanvas();
    });

    connect(ui->cmdPushImageDown, &QPushButton::clicked, this, [this](){
        bool bCellMode = ui->chkCellDivider->isChecked();

        if(!bCellMode){
            uint8_t bottomRow[icon_width];
            for(int x = 0; x < icon_width; x++)
                bottomRow[x] = (*icon_area)[icon_height - 1][x];

            for(int y = icon_height - 1; y > 0; y--)
                for(int x = 0; x < icon_width; x++)
                    (*icon_area)[y][x] = (*icon_area)[y - 1][x];

            for(int x = 0; x < icon_width; x++)
                (*icon_area)[0][x] = bottomRow[x];
        } else {
            int cellsPerRow    = icon_width  / cell_width;
            int cellsPerColumn = icon_height / cell_height;
            std::vector<uint8_t> bottomRow(cell_width);

            for (int cy = 0; cy < cellsPerColumn; cy++) {
                for (int cx = 0; cx < cellsPerRow; cx++) {
                    int cellStartX = cx * cell_width;
                    int cellStartY = cy * cell_height;

                    for (int x = 0; x < cell_width; x++)
                        bottomRow[x] = (*icon_area)[cellStartY + cell_height - 1][cellStartX + x];

                    for (int y = cell_height - 1; y > 0; y--)
                        for (int x = 0; x < cell_width; x++)
                            (*icon_area)[cellStartY + y][cellStartX + x] =
                                (*icon_area)[cellStartY + y - 1][cellStartX + x];

                    for (int x = 0; x < cell_width; x++)
                        (*icon_area)[cellStartY][cellStartX + x] = bottomRow[x];
                }
            }
        }

        renderEditorCanvas();
    });


    connect(ui->cmdPushImageLeft, &QPushButton::clicked, this, [this](){
        bool bCellMode = ui->chkCellDivider->isChecked();

        if(!bCellMode){
            for(int y = 0; y < icon_height; y++){
                uint8_t leftPixel = (*icon_area)[y][0];
                for(int x = 0; x < icon_width - 1; x++)
                    (*icon_area)[y][x] = (*icon_area)[y][x + 1];
                (*icon_area)[y][icon_width - 1] = leftPixel;
            }
        } else {
            int cellsPerRow    = icon_width  / cell_width;
            int cellsPerColumn = icon_height / cell_height;

            for (int cy = 0; cy < cellsPerColumn; cy++) {
                for (int cx = 0; cx < cellsPerRow; cx++) {
                    int cellStartX = cx * cell_width;
                    int cellStartY = cy * cell_height;

                    for (int y = 0; y < cell_height; y++){
                        uint8_t leftPixel = (*icon_area)[cellStartY + y][cellStartX];
                        for (int x = 0; x < cell_width - 1; x++)
                            (*icon_area)[cellStartY + y][cellStartX + x] =
                                (*icon_area)[cellStartY + y][cellStartX + x + 1];
                        (*icon_area)[cellStartY + y][cellStartX + cell_width - 1] = leftPixel;
                    }
                }
            }
        }

        renderEditorCanvas();


    });

    connect(ui->cmdPushImageRight, &QPushButton::clicked, this, [this](){
        bool bCellMode = ui->chkCellDivider->isChecked();

        if(!bCellMode){
            for(int y = 0; y < icon_height; y++){
                uint8_t rightPixel = (*icon_area)[y][icon_width - 1];
                for(int x = icon_width - 1; x > 0; x--)
                    (*icon_area)[y][x] = (*icon_area)[y][x - 1];
                (*icon_area)[y][0] = rightPixel;
            }
        } else {
            int cellsPerRow    = icon_width  / cell_width;
            int cellsPerColumn = icon_height / cell_height;

            for (int cy = 0; cy < cellsPerColumn; cy++) {
                for (int cx = 0; cx < cellsPerRow; cx++) {
                    int cellStartX = cx * cell_width;
                    int cellStartY = cy * cell_height;

                    for (int y = 0; y < cell_height; y++){
                        uint8_t rightPixel = (*icon_area)[cellStartY + y][cellStartX + cell_width - 1];
                        for (int x = cell_width - 1; x > 0; x--)
                            (*icon_area)[cellStartY + y][cellStartX + x] =
                                (*icon_area)[cellStartY + y][cellStartX + x - 1];
                        (*icon_area)[cellStartY + y][cellStartX] = rightPixel;
                    }
                }
            }
        }

        renderEditorCanvas();

    });

    connect(ui->cmdFlipH, &QPushButton::clicked, this, [this](){
        bool bCellMode = ui->chkCellDivider->isChecked();

        if(!bCellMode){
            for(int y = 0; y < icon_height / 2; y++){
                for(int x = 0; x < icon_width; x++){
                    uint8_t tmp = (*icon_area)[y][x];
                    (*icon_area)[y][x] = (*icon_area)[icon_height - 1 - y][x];
                    (*icon_area)[icon_height - 1 - y][x] = tmp;
                }
            }
        } else {
            int cellsPerRow    = icon_width  / cell_width;
            int cellsPerColumn = icon_height / cell_height;

            for (int cy = 0; cy < cellsPerColumn; cy++) {
                for (int cx = 0; cx < cellsPerRow; cx++) {
                    int cellStartX = cx * cell_width;
                    int cellStartY = cy * cell_height;

                    for(int y = 0; y < cell_height / 2; y++){
                        for(int x = 0; x < cell_width; x++){
                            uint8_t tmp = (*icon_area)[cellStartY + y][cellStartX + x];
                            (*icon_area)[cellStartY + y][cellStartX + x] =
                                (*icon_area)[cellStartY + cell_height - 1 - y][cellStartX + x];
                            (*icon_area)[cellStartY + cell_height - 1 - y][cellStartX + x] = tmp;
                        }
                    }
                }
            }
        }

        renderEditorCanvas();
    });

    connect(ui->cmdFlipV, &QPushButton::clicked, this, [this](){
        bool bCellMode = ui->chkCellDivider->isChecked();

        if(!bCellMode){
            for(int y = 0; y < icon_height; y++){
                for(int x = 0; x < icon_width / 2; x++){
                    uint8_t tmp = (*icon_area)[y][x];
                    (*icon_area)[y][x] = (*icon_area)[y][icon_width - 1 - x];
                    (*icon_area)[y][icon_width - 1 - x] = tmp;
                }
            }
        } else {
            int cellsPerRow    = icon_width  / cell_width;
            int cellsPerColumn = icon_height / cell_height;

            for (int cy = 0; cy < cellsPerColumn; cy++) {
                for (int cx = 0; cx < cellsPerRow; cx++) {
                    int cellStartX = cx * cell_width;
                    int cellStartY = cy * cell_height;

                    for(int y = 0; y < cell_height; y++){
                        for(int x = 0; x < cell_width / 2; x++){
                            uint8_t tmp = (*icon_area)[cellStartY + y][cellStartX + x];
                            (*icon_area)[cellStartY + y][cellStartX + x] =
                                (*icon_area)[cellStartY + y][cellStartX + cell_width - 1 - x];
                            (*icon_area)[cellStartY + y][cellStartX + cell_width - 1 - x] = tmp;
                        }
                    }
                }
            }
        }

        renderEditorCanvas();
    });

    connect(ui->cmdClsIconImage, &QPushButton::clicked, this, [this](){
        auto reply = QMessageBox::question(
            this,
            "Clear Icon",
            "Are you sure you want to clear the icon?\nNOTE: this will clear the icon, AND the scratch pad",
            QMessageBox::Yes | QMessageBox::No
            );

        if(reply == QMessageBox::Yes) {
            // Clear icon_area
            for(int y = 0; y < icon_height; y++)
                for(int x = 0; x < icon_width; x++) {
                    icon_area_front[y][x] = numSelectedBackPaletteID;
                    icon_area_back[y][x] = numSelectedBackPaletteID;
                    icon_area_scratchpage[y][x] = numSelectedBackPaletteID;
                }

            renderEditorCanvas(); // redraw empty icon
        }
    });


    connect(ui->cmdSaveIconProject, &QPushButton::clicked, this, [this](){
        static QString lastDir;
        QSettings settings(SettingsCompanyName, SettingsProjectName);
        lastDir = settings.value("lastProjectDir", QDir::homePath()).toString();

        QString filename = QFileDialog::getSaveFileName(this, "Save Icon", lastDir + "/" + ProjectFilename, "Icon Files (*.icn)");
        // saveIcon(filename);
        if(!filename.isEmpty()){
            QFileInfo info(filename);
            ProjectFilename = info.fileName();
            QSettings settings(SettingsCompanyName, SettingsProjectName);
            settings.setValue(SettingsLastFileDirProject, info.absolutePath());
            saveProjectIcon(filename.toUtf8().constData());
        }

    });

    connect(ui->cmdLoadIconProject, &QPushButton::clicked, this, [this](){
        static QString lastDir;
        QSettings settings(SettingsCompanyName, SettingsProjectName);
        lastDir = settings.value(SettingsLastFileDirProject, QDir::homePath()).toString();
        QString filename = QFileDialog::getOpenFileName(this, "Load Icon", lastDir + "/" + ProjectFilename, "Icon Files (*.icn)");//ProfileFilename
        if(!filename.isEmpty()) {
            QFileInfo info(filename);
            QSettings settings(SettingsCompanyName, SettingsProjectName);
            settings.setValue(SettingsLastFileDirProject, info.absolutePath());
            loadProjectIcon(filename.toUtf8().constData());
            ProjectFilename = info.fileName();
            // ---- Palette side-load ----
            QString baseName = info.completeBaseName();
            QString paletteFile = info.absolutePath() + "/" + baseName + ".pal";
            if (QFile::exists(paletteFile)){
                LoadPaletteData(paletteFile.toUtf8().constData());
            }
        }
    });



    connect(ui->cmdSavePalette, &QPushButton::clicked, this, [this](){
        // save de palette!!
        static QString lastDir;
        QSettings settings(SettingsCompanyName, SettingsProjectName);
        lastDir = settings.value(SettingsLastFileDirPalette, QDir::homePath()).toString();
        QString filename = QFileDialog::getSaveFileName(this, "Save Palette", lastDir + "/" + PaletteFilename, "Paltette Files (*.pal)");
        // saveIcon(filename);
        if(!filename.isEmpty()){
            QFileInfo info(filename);
            PaletteFilename = info.fileName();
            QSettings settings(SettingsCompanyName, SettingsProjectName);
            settings.setValue(SettingsLastFileDirPalette, info.absolutePath());
            SavePaletteData(filename.toUtf8().constData());
        }
    });

    connect(ui->cmdLoadPalette, &QPushButton::clicked, this, [this](){
        // save de palette!!
        static QString lastDir;
        QSettings settings(SettingsCompanyName, "SidBox-GraphicsEditV3");
        lastDir = settings.value(SettingsLastFileDirPalette, QDir::homePath()).toString();
        QString filename = QFileDialog::getOpenFileName(this, "Open Palette", lastDir + "/" + PaletteFilename, "Paltette Files (*.pal)");
        // saveIcon(filename);
        if(!filename.isEmpty()){
            QFileInfo info(filename);
            PaletteFilename = info.fileName();
            QSettings settings(SettingsCompanyName, "SidBox-GraphicsEditV3");
            settings.setValue(SettingsLastFileDirPalette, info.absolutePath());
            LoadPaletteData(filename.toUtf8().constData());
        }
    });


    connect(ui->cmdSetIconAreaSize, &QPushButton::clicked, this, [=](){
        icon_width = ui->txtProjectImageWidth->text().toInt();
        icon_height = ui->txtProjectImageHeight->text().toInt();

        // this image was SET a new size, no resizing
        icon_old_width = icon_width;
        icon_old_height = icon_height;

        if(icon_width >  2048) icon_width = 2048;
        if(icon_height > 2048) icon_height = 2048;

        if(icon_width  < 8)    icon_width = 8;
        if(icon_height < 8)    icon_height = 8;

        reSizeEditorArray(icon_width, icon_height);

        reSize();
        renderEditorCanvas();
    });

    connect(ui->cmdSetIconAreaResize, &QPushButton::clicked, this, [this](){
        icon_old_width = icon_width;
        icon_old_height = icon_height;


        icon_width = ui->txtProjectImageWidth->text().toInt();
        icon_height = ui->txtProjectImageHeight->text().toInt();

        if(icon_width >  2048) icon_width = 2048;
        if(icon_height > 2048) icon_height = 2048;

        if(icon_width  < 8)    icon_width = 8;
        if(icon_height < 8)    icon_height = 8;

        ui->txtProjectImageWidth->setText(QString("%1").arg(icon_width));
        ui->txtProjectImageHeight->setText(QString("%1").arg(icon_height));
        ResizeIconArea(icon_width, icon_height, icon_old_width, icon_old_height);

        reSize();
        renderEditorCanvas();
    });

    scrollUpdateTimer = new QTimer(this);
    scrollUpdateTimer->setSingleShot(true);

    connect(scrollUpdateTimer, &QTimer::timeout, this, &MainWindow::renderEditorCanvas);

    connect(ui->scrEditorH, &QScrollBar::valueChanged, this, &MainWindow::onEditorScrollChanged);
    connect(ui->scrEditorV, &QScrollBar::valueChanged, this, &MainWindow::onEditorScrollChanged);

    connect(ui->cmdRotateCC90, &QPushButton::clicked, this, [this](){
        if(ui->chkCellDivider->isChecked()){
            if(icon_width % cell_width != 0 || icon_height % cell_height != 0 || cell_width != cell_height){
                QMessageBox::warning(this, "Cannot Rotate",
                                     "Cells cannot be rotated!\n"
                                     "Make sure the icon is square and cells divide evenly.");
                return;
            }
        }
        rotateIcon(1);
    });

    connect(ui->cmdRotateC90, &QPushButton::clicked, this, [this](){
        if(ui->chkCellDivider->isChecked()){
            if(icon_width % cell_width != 0 || icon_height % cell_height != 0 || cell_width != cell_height){
                QMessageBox::warning(this, "Cannot Rotate",
                                     "Cells cannot be rotated!\n"
                                     "Make sure the icon is square and cells divide evenly.");
                return;
            }
        }
        rotateIcon(0);
    });


    ui->chkFloodFillMirrored->setChecked(bMirroredGradient);
    ui->chkFloodFillReversed->setChecked(bReversedGradient);


    // DRAWING MODE SELECTOR ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // draw tool bar
    //ui->toolButtonPlot->installEventFilter(this);
    connect(ui->toolButtonPlot, &QPushButton::clicked, this, [this](){
        clearToolButtons();
        ui->toolButtonPlot->setChecked(true);   // keep this high lighted!
        currentDrawMode = Plot;
        renderEditorCanvas();
    });

    connect(ui->toolButtonLine, &QPushButton::clicked, this, [this](){
        clearToolButtons();
        ui->toolButtonLine->setChecked(true);   // keep this high lighted!
        currentDrawMode = Line;
        renderEditorCanvas();
    });

    connect(ui->toolButtonPen, &QPushButton::clicked, this, [this](){
        clearToolButtons();
        ui->toolButtonPen->setChecked(true);   // keep this high lighted!
        currentDrawMode = Pen;
        renderEditorCanvas();
    });

    connect(ui->toolButtonSprayCan, &QPushButton::clicked, this, [this](){
        clearToolButtons();
        ui->toolButtonSprayCan->setChecked(true);   // keep this high lighted!
        currentDrawMode = SprayCan;
        renderEditorCanvas();
    });



    connect(ui->toolButtonText, &QPushButton::clicked, this, [this](){
        clearToolButtons();
        ui->toolButtonText->setChecked(true);   // keep this high lighted!
        currentDrawMode = DrawText;
        renderEditorCanvas();
    });

    ui->toolButtonRect->installEventFilter(this);
    connect(ui->toolButtonRect, &QPushButton::clicked, this, [this](){
        clearToolButtons();
        ui->toolButtonRect->setChecked(true);   // keep this high lighted!
        currentDrawMode = Rect;
        renderEditorCanvas();
    });

    ui->toolButtonCircle->installEventFilter(this);
    connect(ui->toolButtonCircle, &QPushButton::clicked, this, [this](){
        clearToolButtons();
        ui->toolButtonCircle->setChecked(true);   // keep this high lighted!
        currentDrawMode = Circle;
        renderEditorCanvas();
    });

    connect(ui->radFillTypeLinear, &QRadioButton::clicked, this, [this](){
        iFillType = FillLinear;
    });

    connect(ui->radFillTypeDiamond, &QRadioButton::clicked, this, [this](){
        iFillType = FillDiamond;
    });

    connect(ui->radFillTypeCircles, &QRadioButton::clicked, this, [this](){
        iFillType = FillCircles;
    });

    //connect(ui->radFillTypeBrush, &QRadioButton::clicked, this, [this](){
        //iFillType = FillFromBrush;
    //});
    //chkFloodFillBrush


    ui->toolButtonFloodFill->installEventFilter(this);
    connect(ui->toolButtonFloodFill, &QPushButton::clicked, this, [this](){
        clearToolButtons();
        ui->toolButtonFloodFill->setChecked(true);   // keep this high lighted!
        renderEditorCanvas();
    });

    //ui->toolButtonCopyArea->installEventFilter(this);
    connect(ui->toolButtonCopyArea, &QPushButton::clicked, this, [this](){

        QPoint cursorPos = ui->toolButtonCopyArea->mapFromGlobal(QCursor::pos());

        if(cursorPos.x() < ui->toolButtonCopyArea->width() / 2){
            currentDrawMode = CopyBrush;
            bCapturingCopyArea = true;
            clearToolButtons();
            ui->toolButtonCopyArea->setChecked(true);
        } else {
            lastDrawMode = currentDrawMode;

            ui->toolButtonCopyArea->blockSignals(true);
            if (!clipboardHasImage() && icon_copy_area.empty()) {
                QMessageBox::information(this, "Paste Brush", "No brush available and clipboard does not contain an image.");
                ui->toolButtonCopyArea->setChecked(false);
                currentDrawMode = lastDrawMode;
            } else {

                clearToolButtons();
                ui->toolButtonCopyArea->setChecked(true);

                if (clipboardHasImage()) {
                    auto reply = QMessageBox::question(this,
                        "Paste Brush",
                        "Clipboard contains an image.\n\nUse it as the brush?",
                        QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::Yes
                        );
                    if (reply == QMessageBox::Yes)
                        pasteClipboardAsBrush();
                }

                currentDrawMode = PasteBrush;
                bCapturingCopyArea = false;
                bGrabbedCopyStart = false;
            }
            ui->toolButtonCopyArea->blockSignals(false);
        }
    });

    // pen draw modes ----------------------
    ui->chkPenDrawShape->setChecked(bPenShapeCircle);   // initial size
    connect(ui->chkPenDrawShape,      &QCheckBox::clicked,   this, [this](){
        bPenShapeCircle = ui->chkPenDrawShape->isChecked();
    });

    ui->scrPenDrawSize->setValue(iPenShapeSize);        // initial size
    ui->lblPenSize->setText(QString("%1").arg(iPenShapeSize));
    connect(ui->scrPenDrawSize,       &QScrollBar::valueChanged, this, [this](){
        iPenShapeSize = ui->scrPenDrawSize->value();
        ui->lblPenSize->setText(QString("%1").arg(iPenShapeSize));
    });

    // spray can ---------------------------
    tmrSprayCanTimer = new QTimer(this);
    tmrSprayCanTimer->setInterval(iSprayRate);
    connect(tmrSprayCanTimer, &QTimer::timeout, this, &MainWindow::onSprayCanTick);  // your slot

    ui->scrSprayRate->setValue(iSprayRate);
    ui->lblSprayRate->setText(QString("%1").arg(iSprayRate));
    connect(ui->scrSprayRate,         &QScrollBar::valueChanged, this, [this](){
        iSprayRate = ui->scrSprayRate->value();
        ui->lblSprayRate->setText(QString("%1").arg(iSprayRate));
    });

    // text draw ---------------------------
    ui->scrTextWidth->setValue(iTextWidth);
    ui->lblTextWidth->setText(QString("%1").arg(iTextWidth));

    ui->scrTextHeight->setValue(iTextHeight);
    ui->lblTextHeight->setText(QString("%1").arg(iTextHeight));
    connect(ui->scrTextWidth, &QScrollBar::valueChanged, this, [this](){
        iTextWidth = ui->scrTextWidth->value();
        ui->lblTextWidth->setText(QString("%1").arg(iTextWidth));
    });
    connect(ui->scrTextHeight, &QScrollBar::valueChanged, this, [this](){
        iTextHeight = ui->scrTextHeight->value();
        ui->lblTextHeight->setText(QString("%1").arg(iTextHeight));
    });


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    connect(ui->cmdImportImage, &QPushButton::clicked, this, [this](){
        static QString lastDir;
        QSettings settings(SettingsCompanyName, SettingsProjectName);
        lastDir = settings.value(SettingsLastImportDir, QDir::homePath()).toString();


        QString filename = QFileDialog::getOpenFileName(this, "Import Image...", lastDir,
                "Acceptable Images [*.bmp, *.png, *.gif, *.jpg, *.iff, *.ilbm](*.bmp *.png *.gif *.iff *.ilbm *.jpg);;"
                "bitmap [*.bmp](*.bmp)");
        if(!filename.isEmpty()) {

            QFileInfo info(filename);
            QSettings settings(SettingsCompanyName, SettingsProjectName);
            settings.setValue(SettingsLastImportDir, info.absolutePath());
            importGif(filename);
            ui->outputTextView->hide();
            ui->frmFontWorkbench->hide();
            ui->frmOptions->hide();
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
                CCLUT[i] = DEFAULT_CLUT[i];
                CBACKUP_CLUT[i] = DEFAULT_CLUT[i];
            }
            renderPaletteCanvas();
            renderEditorCanvas(); // redraw empty icon
        }
    });

    connect(ui->chkPaletteBankFront, &QPushButton::clicked, this, [this](){
        CCLUT = CLUTF;
        active_icon_area = &icon_area_front;
        if(bEditorPage){
            icon_area = &icon_area_scratchpage;
        } else {
            icon_area = active_icon_area;
        }

        renderPaletteCanvas();
    });

    connect(ui->chkPaletteBankBack, &QPushButton::clicked, this, [this](){
        CCLUT = CLUTB;
        active_icon_area = &icon_area_back;
        if(bEditorPage){
            icon_area = &icon_area_scratchpage;
        } else {
            icon_area = active_icon_area;
        }
        renderPaletteCanvas();
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

                uint32_t rgb = CCLUT[i];
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
        doHighlighter();
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
                    rgb = CCLUT[i] & 0xFFFFFF; // no alpha on this palette entry
                else
                    rgb = CCLUT[i] | 0xFF000000; // force alpha 255

                output += "0x" + QString::number(rgb, 16).rightJustified(8, '0').toUpper();
                if (i < 255) output += ",";
                if(x != 7) output += " ";
            }
            output += "\n";
        }
        output += "};\n";

        // Show in the text view
        ui->txtOutputText->setPlainText(output);
        doHighlighter();
        ui->outputTextView->show();
        ui->outputTextView->raise();
    });

    connect(ui->cmdCloseOutputText, &QPushButton::clicked, this, [this](){
        ui->outputTextView->hide();
    });

    connect(ui->cmdExportH, &QPushButton::clicked, this, [this](){
        uint16_t bits;   // collect the config bits
        bits = ui->chkExportRLE->isChecked() * ExportRLE;
        bits += ui->chkExportSBVRAM->isChecked() * ExportSidBoxVRAM;
        //printf("Bits checked: %x\n", bits);
        ExportImageToH("", bits);
        ui->outputTextView->raise();
    });


    connect(ui->scrCycleStepper, &QScrollBar::valueChanged, this, [this](){
        ui->lblCycleStepping->setText(QString("%1").arg(ui->scrCycleStepper->value()));
    });

    ui->outputTextView->hide();
    ui->frmFontWorkbench->hide();
    ui->frmOptions->hide();

    connect(ui->cmdOpenFontWorkbench, &QPushButton::clicked, this, [this](){
        ui->frmFontWorkbench->show();
        //ui->frmFontWorkbench->topLevelWidget();
        ui->frmFontWorkbench->raise();
    });

    connect(ui->cmdCloseFontWorkbench, &QPushButton::clicked, this, [this](){
        ui->frmFontWorkbench->hide();
    });

    connect(ui->cmdOpenOptions, &QPushButton::clicked, this, [this](){
        ui->frmOptions->show();
        ui->frmOptions->raise();
    });

    connect(ui->cmdOptionsClose, &QPushButton::clicked, this, [this](){
        ui->frmOptions->hide();
    });

    connect(ui->chkFloodFillDitherNoise, &QCheckBox::clicked, this, [this](){
        bNoisyDither = ui->chkFloodFillDitherNoise->isChecked();
    });

    connect(ui->chkFloodFillDither, &QCheckBox::clicked, this, [this](){
        bDithered = ui->chkFloodFillDither->isChecked();
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
                //CCLUT[i] = CBACKUP_CLUT[i];
                CLUTF[i] = BACKUP_CLUTF[i];
                CLUTB[i] = BACKUP_CLUTB[i];
                renderPaletteCanvas();
                renderEditorCanvas(); // redraw empty icon
            }
        }
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
    connect(ui->cmdFlipFontVALL,    &QPushButton::clicked, this, [this](){ fontEditor->MoveFont(font_flip_vall);});
    connect(ui->cmdFlipFontHALL,    &QPushButton::clicked, this, [this](){ fontEditor->MoveFont(font_flip_hall);});


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
        doHighlighter();
        //
        ui->outputTextView->show();
        //ui->frmFontWorkbench->hide();
        ui->outputTextView->raise();
    });

    connect(ui->cmdExportAmigaILBM, &QPushButton::clicked, this, [this](){
        QString filename = QFileDialog::getSaveFileName(this, "Save Amiga ILBM (256 AGA)", "", "Amiga ILBM (*.iff *.ilbm)");
        // saveIcon(filename);
        if(!filename.isEmpty())
            ExportToILBM(filename.toUtf8().constData());
    });

    connect(ui->cmdExportPixelPictureBitmap, &QPushButton::clicked, this, [this](){
        uint16_t bits;   // collect the config bits
        bits = ui->chkExportRLE->isChecked() * ExportRLE;
        bits += ui->chkExportSBVRAM->isChecked() * ExportSidBoxVRAM;
        //printf("Bits checked: %x\n", bits);
        QString filename = QFileDialog::getSaveFileName(this, "Save Pixel Picture Bitmap", "", "Pixel Picture Bitmap (*.ppb)");
        if(!filename.isEmpty())
            ExportToPPB(filename.toUtf8().constData(), bits);
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


    /// image handler buttons
    connect(ui->cmdHandleTL, &QPushButton::clicked, this, [this](){ clearHandlerButtons(cHandleTL); });
    connect(ui->cmdHandleTM, &QPushButton::clicked, this, [this](){ clearHandlerButtons(cHandleTM); });
    connect(ui->cmdHandleTR, &QPushButton::clicked, this, [this](){ clearHandlerButtons(cHandleTR); });
    connect(ui->cmdHandleML, &QPushButton::clicked, this, [this](){ clearHandlerButtons(cHandleML); });
    connect(ui->cmdHandleMM, &QPushButton::clicked, this, [this](){ clearHandlerButtons(cHandleMM); });
    connect(ui->cmdHandleMR, &QPushButton::clicked, this, [this](){ clearHandlerButtons(cHandleMR); });
    connect(ui->cmdHandleBL, &QPushButton::clicked, this, [this](){ clearHandlerButtons(cHandleBL); });
    connect(ui->cmdHandleBM, &QPushButton::clicked, this, [this](){ clearHandlerButtons(cHandleBM); });
    connect(ui->cmdHandleBR, &QPushButton::clicked, this, [this](){ clearHandlerButtons(cHandleBR); });

    connect(ui->cmdToolBrushSize1, &QPushButton::clicked, this, [this](){ clearBrushSizeButtons(0); });
    connect(ui->cmdToolBrushSize2, &QPushButton::clicked, this, [this](){ clearBrushSizeButtons(1); });
    connect(ui->cmdToolBrushSize3, &QPushButton::clicked, this, [this](){ clearBrushSizeButtons(2); });
    connect(ui->cmdToolBrushSize4, &QPushButton::clicked, this, [this](){ clearBrushSizeButtons(3); });
    connect(ui->cmdToolBrushSize5, &QPushButton::clicked, this, [this](){ clearBrushSizeButtons(4); });

    paletteRangerOffset = 128;
    paletteRangerLength = 16;

    clearBrushSizeButtons(0);

    ui->lblPaletteOffset->setText( QString("%1").arg(paletteRangerOffset));
    ui->lblPaletteSizer->setText( QString("%1").arg(paletteRangerLength));

    ui->sldPaletteOffset->setValue(paletteRangerOffset);
    ui->sldPaletteSize->setValue(paletteRangerLength);




    loadDefaultFont();
    icon_area = active_icon_area;

    connect(tmrColourCycle, &QTimer::timeout, this, &MainWindow::onColourCycleTick);  // your slot
    tmrColourCycle->start();

}


void MainWindow::clearHandlerButtons(char handleMode){

    cBrushHandleMode = handleMode;
    if(handleMode == cHandleTL) ui->cmdHandleTL->setChecked(true); else ui->cmdHandleTL->setChecked(false);
    if(handleMode == cHandleTM) ui->cmdHandleTM->setChecked(true); else ui->cmdHandleTM->setChecked(false);
    if(handleMode == cHandleTR) ui->cmdHandleTR->setChecked(true); else ui->cmdHandleTR->setChecked(false);

    if(handleMode == cHandleML) ui->cmdHandleML->setChecked(true); else ui->cmdHandleML->setChecked(false);
    if(handleMode == cHandleMM) ui->cmdHandleMM->setChecked(true); else ui->cmdHandleMM->setChecked(false);
    if(handleMode == cHandleMR) ui->cmdHandleMR->setChecked(true); else ui->cmdHandleMR->setChecked(false);

    if(handleMode == cHandleBL) ui->cmdHandleBL->setChecked(true); else ui->cmdHandleBL->setChecked(false);
    if(handleMode == cHandleBM) ui->cmdHandleBM->setChecked(true); else ui->cmdHandleBM->setChecked(false);
    if(handleMode == cHandleBR) ui->cmdHandleBR->setChecked(true); else ui->cmdHandleBR->setChecked(false);
}

void MainWindow::clearBrushSizeButtons(char brushSize){
    ui->cmdToolBrushSize1->setChecked(false);
    ui->cmdToolBrushSize2->setChecked(false);
    ui->cmdToolBrushSize3->setChecked(false);
    ui->cmdToolBrushSize4->setChecked(false);
    ui->cmdToolBrushSize5->setChecked(false);

    if(brushSize == 0) ui->cmdToolBrushSize1->setChecked(true);
    if(brushSize == 1) ui->cmdToolBrushSize2->setChecked(true);
    if(brushSize == 2) ui->cmdToolBrushSize3->setChecked(true);
    if(brushSize == 3) ui->cmdToolBrushSize4->setChecked(true);
    if(brushSize == 4) ui->cmdToolBrushSize5->setChecked(true);

    iPenShapeSize = (brushSize * 2) + 1;

    ui->scrPenDrawSize->setValue(iPenShapeSize);
    ui->lblPenSize->setText(QString("%1").arg(iPenShapeSize));

}

// used for copying the icon_area to the backup
void CommitIconArea(){
    for(int x = 0; x < icon_width; x++){
        for(int y = 0; y < icon_height; y++){
            icon_area_backup[y][x] = (*icon_area)[y][x];
        }
    }

}

void UndoIconArea(){

    // copy whats in the icon_area to the redo buffer
    for(int x = 0; x < icon_width; x++){
        for(int y = 0; y < icon_height; y++){
            icon_area_redo[y][x] = (*icon_area)[y][x];
        }
    }

    // copy back from the backup to the icon area
    for(int x = 0; x < icon_width; x++){
        for(int y = 0; y < icon_height; y++){
            (*icon_area)[y][x] = icon_area_backup[y][x];
        }
    }

    // make a back up of the last known icon_area
    for(int x = 0; x < icon_width; x++){
        for(int y = 0; y < icon_height; y++){
            icon_area_backup[y][x] = icon_area_redo[y][x];
        }
    }
}



void MainWindow::ResizeIconArea(int newWidth, int newHeight, int oldWidth, int oldHeight){
    if (newWidth <= 0 || newHeight <= 0) return;
    int mode = ui->chkResampler->isChecked() ? Resample:NearestNeighbor;

    std::vector<std::vector<uint8_t>> icon_area_tmp = (*icon_area);

    reSizeEditorArray(newWidth, newHeight);

    double scaleX = (double)oldWidth / newWidth;
    double scaleY = (double)oldHeight / newHeight;

    for (int y = 0; y < newHeight; y++) {
        int srcY = (mode == NearestNeighbor) ? (int)(y * scaleY) : 0;
        for (int x = 0; x < newWidth; x++) {
            int srcX = (mode == NearestNeighbor) ? (int)(x * scaleX) : 0;

            if (mode == NearestNeighbor) {
                // same as before
                if (srcY >= oldHeight) srcY = oldHeight - 1;
                if (srcX >= oldWidth)  srcX = oldWidth - 1;
                (*icon_area)[y][x] = icon_area_tmp[srcY][srcX];
            } else {
                // Color-aware resample
                double x0 = x * scaleX;
                double x1 = (x + 1) * scaleX;
                double y0 = y * scaleY;
                double y1 = (y + 1) * scaleY;

                int ix0 = (int)floor(x0), ix1 = (int)ceil(x1);
                int iy0 = (int)floor(y0), iy1 = (int)ceil(y1);

                if (ix1 > oldWidth)  ix1 = oldWidth;
                if (iy1 > oldHeight) iy1 = oldHeight;

                int rSum = 0, gSum = 0, bSum = 0;
                int count = 0;

                for (int sy = iy0; sy < iy1; sy++) {
                    for (int sx = ix0; sx < ix1; sx++) {
                        uint8_t idx = icon_area_tmp[sy][sx];
                        uint32_t c = CCLUT[idx];
                        rSum += (c >> 16) & 0xFF;
                        gSum += (c >> 8) & 0xFF;
                        bSum += c & 0xFF;
                        count++;
                    }
                }

                if (count == 0) count = 1;
                int rAvg = rSum / count;
                int gAvg = gSum / count;
                int bAvg = bSum / count;

                // Find nearest palette index
                int bestIdx = 0;
                int bestDist = 256*256*3;
                for (int i = 0; i < 256; i++) {
                    uint32_t c = CCLUT[i];
                    int dr = ((c >> 16) & 0xFF) - rAvg;
                    int dg = ((c >> 8) & 0xFF) - gAvg;
                    int db = (c & 0xFF) - bAvg;
                    int dist = dr*dr + dg*dg + db*db;
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestIdx = i;
                    }
                }
                (*icon_area)[y][x] = (uint8_t)bestIdx;
            }
        }
    }

    icon_width  = newWidth;
    icon_height = newHeight;


    renderEditorCanvas();
}




void MainWindow::clearToolButtons(){
    gradientDragging = false;    // restart this
    ui->toolButtonPlot->setChecked(false);
    ui->toolButtonLine->setChecked(false);
    ui->toolButtonPen->setChecked(false);
    ui->toolButtonSprayCan->setChecked(false);
    ui->toolButtonFloodFill->setChecked(false);
    ui->toolButtonText->setChecked(false);
    ui->toolButtonRect->setChecked(false);
    ui->toolButtonCircle->setChecked(false);
    ui->toolButtonCopyArea->setChecked(false);

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
        tmp = CLUTF[cyclefrom];
        tmpold = clut_cycle_index[cyclefrom];
        for (i = cyclefrom; i < cycleto; i++) {
            CLUTF[i] = CLUTF[i + 1];
            clut_cycle_index[i] = clut_cycle_index[i + 1];
        }
        CLUTF[i] = tmp;
        clut_cycle_index[i] = tmpold;

    } else {
        tmp = CLUTF[cycleto];
        tmpold = clut_cycle_index[cycleto];
        for (i = cycleto; i > cyclefrom; i--) {
            CLUTF[i] = CLUTF[i - 1];
            clut_cycle_index[i] = clut_cycle_index[i - 1];
        }
        CLUTF[i] = tmp;
        clut_cycle_index[i] = tmpold;
    }

    if (bCycleDirection == 0) {
        tmp = CLUTB[cyclefrom];
        tmpold = clut_cycle_index[cyclefrom];
        for (i = cyclefrom; i < cycleto; i++) {
            CLUTB[i] = CLUTB[i + 1];
            clut_cycle_index[i] = clut_cycle_index[i + 1];
        }
        CLUTB[i] = tmp;
        clut_cycle_index[i] = tmpold;

    } else {
        tmp = CLUTB[cycleto];
        tmpold = clut_cycle_index[cycleto];
        for (i = cycleto; i > cyclefrom; i--) {
            CLUTB[i] = CLUTB[i - 1];
            clut_cycle_index[i] = clut_cycle_index[i - 1];
        }
        CLUTB[i] = tmp;
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
                        output += ",\n    ";
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

QByteArray generateRLEBytes(
    const std::vector<std::vector<uint8_t>>& icon_area,
    uint16_t width,
    uint16_t height,
    int /*colWidthMax*/,   // unused in binary version
    int& outRLESize        // returns compressed byte count
){
    QByteArray output;
    output.reserve(width * height); // worst case

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
                    // emit RLE pair: (count, value)
                    output.append(char(runLength));
                    output.append(char(lastPixel));
                    outRLESize += 2;
                }

                lastPixel = pixel;
                runLength = 1;
            }
        }
    }

    // flush final run
    if (runLength > 0) {
        output.append(char(runLength));
        output.append(char(lastPixel));
        outRLESize += 2;
    }

    return output;
}


#include <stdio.h>
#include <stdint.h>

static inline uint32_t swap32(uint32_t v)
{
    return ((v >> 24) & 0x000000FF) |
           ((v >>  8) & 0x0000FF00) |
           ((v <<  8) & 0x00FF0000) |
           ((v << 24) & 0xFF000000);
}


int MainWindow::ExportToPPB(const char *filename, const uint16_t modes)
{
    if (!filename || !icon_area) return 0;

    const uint16_t imgW = (uint16_t)icon_width;
    const uint16_t imgH = (uint16_t)icon_height;

    // Flags
    bool useRLE  = !!(modes & ExportRLE);
    bool isCells = ui->chkCellDivider->isChecked();
    if (useRLE) isCells = false;

    // configbits: bit4=RLE, bit5=cells, low nibble = paletteDepth
    uint8_t configbits = 0;
    configbits =
        (uint8_t)((useRLE  ? 1 : 0) << 4) |
        (uint8_t)((isCells ? 1 : 0) << 5) |
        (uint8_t)(paletteDepth & 0x0F);

    // Build payload (unchanged)
    QByteArray payload;
    payload.reserve((int)imgW * (int)imgH);

    if (!useRLE) {
        for (uint16_t y = 0; y < imgH; y++) {
            for (uint16_t x = 0; x < imgW; x++) {
                payload.append(char((*icon_area)[y][x]));
            }
        }
    } else {
        int rleSize = 0;
        QByteArray rleBytes = generateRLEBytes((*icon_area), imgW, imgH, /*maxRun*/16, rleSize);
        payload = rleBytes;
    }

    const uint32_t imgLen = (uint32_t)payload.size();

    // Open file (C stdio)
    FILE *f = fopen(filename, "wb");
    if (!f) return 0;

    // ---- Write header (16 bytes) ----
    // Layout:
    // [0] configbits
    // [1..2] width  (big-endian)
    // [3..4] height (big-endian)
    // [5..8] payload length (big-endian)
    // [9..15] reserved = 0
    fputc(configbits, f);

    fputc((imgW >> 8) & 0xFF, f);
    fputc((imgW >> 0) & 0xFF, f);

    fputc((imgH >> 8) & 0xFF, f);
    fputc((imgH >> 0) & 0xFF, f);

    fputc((imgLen >> 24) & 0xFF, f);
    fputc((imgLen >> 16) & 0xFF, f);
    fputc((imgLen >>  8) & 0xFF, f);
    fputc((imgLen >>  0) & 0xFF, f);

    for (int i = 0; i < 7; i++)
        fputc(0x00, f);

    // Optional: detect header write failure early
    if (ferror(f)) { fclose(f); return 0; }

    // ---- Write palette (256 * 4 bytes) ----
    // IMPORTANT: This writes CLUT as raw uint32_t words in native endianness (little-endian on PC).
    uint32_t tCLUT[256];
    for(int i = 0; i < 256; i++){
        uint32_t v = CCLUT[i];

        // Fix alpha
        if (i == 0)
            v &= 0x00FFFFFF;   // transparent
        else
            v |= 0xFF000000;   // opaque

        tCLUT[i] = v;//swap32(v);

    }
    if (fwrite(tCLUT, 256u * sizeof(uint32_t), 1, f) != 1) {
        fclose(f);
        return 0;
    }

    // ---- Write payload ----
    if (imgLen > 0) {
        if (fwrite(payload.constData(), imgLen, 1, f) != 1) {
            fclose(f);
            return 0;
        }
    }

    fflush(f);
    fclose(f);
    return 1;
}


void MainWindow::ExportToILBM(const char *filename){
    bool RLE = true;   // <-- toggle compression here

    RLE = ui->chkExportRLE->isChecked();

    FILE *f = fopen(filename, "wb");
    if (!f) return;

    const int width  = icon_width;
    const int height = icon_height;
    const int planes = 8;

    const int bytesPerRow = ((width + 15) / 16) * 2;

    // # Build BODY (compressed or not)

    uint8_t *bodyBuf = (uint8_t*)malloc(height * planes * bytesPerRow * 2);
    int bodySize = 0;

    uint8_t *rowBuf = (uint8_t*)calloc(planes * bytesPerRow, 1);

    for (int y = 0; y < height; y++){
        memset(rowBuf, 0, planes * bytesPerRow);
        for (int x = 0; x < width; x++){
            uint8_t pix = (*icon_area)[y][x];
            int byte = x >> 3;
            int bit  = 7 - (x & 7);
            for (int p = 0; p < planes; p++){
                if (pix & (1 << p))
                    rowBuf[p * bytesPerRow + byte] |= (1 << bit);
            }
        }

        // RLE or raw
        if (!RLE){  // RAW output
            memcpy(bodyBuf + bodySize, rowBuf, planes * bytesPerRow);
            bodySize += planes * bytesPerRow;
        } else {    // RLE Compressed
            for (int p = 0; p < planes; p++){
                uint8_t *src = rowBuf + p * bytesPerRow;
                int i = 0;
                while (i < bytesPerRow){
                    int run = 1;
                    while (i + run < bytesPerRow && run < 128 && src[i] == src[i + run])
                        run++;

                    if (run >= 2){
                        bodyBuf[bodySize++] = (uint8_t)(1 - run);
                        bodyBuf[bodySize++] = src[i];
                        i += run;
                    } else {
                        int start = i++;
                        while (i < bytesPerRow) {
                            int look = 1;
                            while (i + look < bytesPerRow && look < 128 && src[i] == src[i + look])
                                look++;
                            if (look >= 2 || (i - start) >= 127)
                                break;
                            i++;
                        }

                        int count = i - start;
                        bodyBuf[bodySize++] = (uint8_t)(count - 1);
                        for (int j = 0; j < count; j++)
                            bodyBuf[bodySize++] = src[start + j];
                    }
                }
            }
        }
    }

    free(rowBuf);

    // IFF requires even-sized chunks
    int bodyPad = bodySize & 1;

    // Calculate FORM size
    uint32_t bmhdSize = 20;
    uint32_t cmapSize = 256 * 3;

    uint32_t formSize =
        4 +
        8 + bmhdSize +
        8 + cmapSize +
        8 + bodySize +
        bodyPad;
    // FORM header
    fputs("FORM", f);
    fputc((formSize >> 24) & 0xFF, f);
    fputc((formSize >> 16) & 0xFF, f);
    fputc((formSize >> 8)  & 0xFF, f);
    fputc(formSize & 0xFF, f);
    fputs("ILBM", f);

    // BMHD
    fputs("BMHD", f);
    fputc(0, f); fputc(0, f); fputc(0, f); fputc(20, f);

    fputc(width >> 8, f);  fputc(width & 0xFF, f);
    fputc(height >> 8, f); fputc(height & 0xFF, f);

    fputc(0, f); fputc(0, f);   // x
    fputc(0, f); fputc(0, f);   // y

    fputc(planes, f);
    fputc(0, f);                // masking
    fputc(RLE ? 1 : 0, f);      // compression
    fputc(0, f);                // pad

    fputc(0, f); fputc(0, f);   // transparent
    fputc(10, f); fputc(10, f); // aspect

    fputc(width >> 8, f);  fputc(width & 0xFF, f);
    fputc(height >> 8, f); fputc(height & 0xFF, f);

    // CMAP
    fputs("CMAP", f);
    fputc(0, f); fputc(0, f);
    fputc(3, f); fputc(0, f);

    for (int i = 0; i < 256; i++){
        uint32_t c = CCLUT[i];
        fputc((c >> 16) & 0xFF, f);
        fputc((c >> 8)  & 0xFF, f);
        fputc(c & 0xFF, f);
    }

    // BODY
    fputs("BODY", f);
    fputc((bodySize >> 24) & 0xFF, f);
    fputc((bodySize >> 16) & 0xFF, f);
    fputc((bodySize >> 8)  & 0xFF, f);
    fputc(bodySize & 0xFF, f);

    fwrite(bodyBuf, bodySize, 1, f);
    if (bodyPad) fputc(0, f);

    free(bodyBuf);
    fclose(f);
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
    imgW = icon_width;
    imgH = icon_height;

    uint8_t il_v0 = (imgLen >> 24) & 0xff;
    uint8_t il_v1 = (imgLen >> 16) & 0xff;
    uint8_t il_v2 = (imgLen >> 8) & 0xff;
    uint8_t il_v3 = (imgLen >> 0) & 0xff;


    QString output;
    QString rleData;
    uint8_t isCells = ui->chkCellDivider->isChecked();

    output += "#include <stdint.h>\n\n";
    output += "// Image Params //\n";
    if(!(modes & ExportRLE))
        output += "// non compressed \n";
    else {

        output += "// RLE compressed bytes are now (how-many), (pixel colour index), ...\n";
        if(isCells) {
            output += "// !!! NOTE: Cells are enabled but will be ignored because RLE is active\n";
            isCells = false;
        }
    }

    if(isCells){
        output += "// image is arranged as cells \n";
        output += "//     Width: " + QString("%1").arg(cell_width) + "px\n";
        output += "//    Height: " + QString("%1").arg(cell_height) + "px ";
        output += "\n";
    }

    output += "\n";
    output += "uint8_t image[] = {\n";

    uint8_t configbits = 0;

    configbits = (!!(modes & ExportRLE) << 4) |
                 ((isCells << 5));

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
        rleData = generateRLE((*icon_area), icon_width, icon_height, 16, rleSize);

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
        /// Uncompressed, unchanged or cell-divided
        output += "    ";
        int columnstep = 0;
        bool firstElement = true;

        if(!isCells){
            // Normal row-by-row
            for (uint16_t y = 0; y < icon_height; y++) {
                for (uint16_t x = 0; x < icon_width; x++) {
                    uint8_t colDat = (*icon_area)[y][x];

                    if (!(x == 0 && y == 0)) output += ", ";
                    if(columnstep >= colWidthMax){
                        output += "\n    ";
                        columnstep = 0;
                    }
                    output += QString("%1").arg(hex8(colDat));
                    columnstep++;
                }
            }
        } else {
            // Cell-divided output
            int cellsX = icon_width / cell_width;
            int cellsY = icon_height / cell_height;

            for(int cy = 0; cy < cellsY; cy++){
                for(int cx = 0; cx < cellsX; cx++){
                    int baseX = cx * cell_width;
                    int baseY = cy * cell_height;

                    for(int y = 0; y < cell_height; y++){
                        for(int x = 0; x < cell_width; x++){
                            uint8_t colDat = (*icon_area)[baseY + y][baseX + x];

                            if(!firstElement) output += ", ";
                            firstElement = false;

                            if(columnstep >= colWidthMax){
                                output += "\n    ";
                                columnstep = 0;
                            }
                            output += QString("%1").arg(hex8(colDat));
                            columnstep++;
                        }
                    }
                }
            }
        }
    } else {
        output += "    ";
        output += rleData;
    }

    output += "\n};\n";

    if(isCells){
        output += "\n\n\n";
        output += "// Cell to image pointer list\n";
        output += "uint8_t *images[] = {\n";

        int cellsX = icon_width / cell_width;
        int cellsY = icon_height / cell_height;
        int cellSize = cell_width * cell_height;
        int cellIndex = 0;

        for(int cy=0; cy<cellsY; cy++){
            for(int cx=0; cx<cellsX; cx++){
                int offset = (cy * cellsX + cx) * cellSize;
                //output += QString("    image + %1, //    %2\n").arg(offset).arg(cellIndex);
                output += QString("    image + %1, //    %2\n")
                              .arg(offset, 6, 10, QLatin1Char(' '))   // offset, width=6, base=10, pad with space
                              .arg(cellIndex, 3, 10, QLatin1Char(' ')); // cellIndex, width=3


                cellIndex ++;
            }
        }

        output += "};\n";
    }



    // Show in the text view
    ui->txtOutputText->setPlainText(output);
    //QString html = output.toHtmlEscaped();
    //html.replace("0x00", "<span style=\"color:#005500;\">0x00</span>");
    //ui->txtOutputText->setHtml("<pre>" + html + "</pre>");

    doHighlighter();

    ui->outputTextView->show();
}

void MainWindow::doHighlighter()
{
    QString html = ui->txtOutputText->toPlainText().toHtmlEscaped();

    // Capture the entire zero hex literal
    QRegularExpression re(
        R"(\b(0x0+)\b)",
        QRegularExpression::CaseInsensitiveOption
    );

    html.replace(re, "<span style=\"color:#008800;\">\\1</span>");

    ui->txtOutputText->setHtml(
        "<pre style=\"font-family:monospace; font-size:10pt; white-space:pre;\">"
        + html +
        "</pre>"
    );
}



void MainWindow::SavePaletteData(const char *filename){
        FILE *f = fopen(filename, "wb");
        if(!f) {
            QMessageBox::warning(this, "Save Palette Fail", "Cannot open file for writing!");
            return;
        }
        fwrite(CLUTF, sizeof(uint32_t), 256, f);
        fwrite(CLUTB, sizeof(uint32_t), 256, f);
        fclose(f);
}

void MainWindow::LoadPaletteData(const char *filename){
    FILE *f = fopen(filename, "rb");
    if(!f) {
        QMessageBox::warning(this, "Load Palette Fail", "Cannot open file for read!");
        return;
    }
    fread(CLUTF, sizeof(uint32_t), 256, f);
    fread(CLUTB, sizeof(uint32_t), 256, f);
    fclose(f);

    renderPaletteCanvas();
    renderEditorCanvas(); // redraw empty icon
}

void MainWindow::doSpreadPalette(uint8_t targetID){
    uint8_t r, g, b,
            r1, g1, b1,
            r2, g2, b2;

    int spreadLength;
    uint32_t fromColour = CCLUT[capturedPaletteIndex];
    uint32_t toColour   = CCLUT[targetID];

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
        CCLUT[start + i] = (r << 16) | (g << 8) | b;
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

                // Get full palette
                for (int i = 0; i < totalCols; i++) {
                    int base = pos + 8 + i * 3;
                    int r = p[base + 0];
                    int g = p[base + 1];
                    int b = p[base + 2];

                    // perceptual luminance
                    int lum = r * 30 + g * 59 + b * 11;

                    cols.push_back({ r, g, b, lum });
                }

                // Sort by luminance (groups similar shades)
                std::sort(cols.begin(), cols.end(),
                          [](const Col &a, const Col &b) {
                              return a.lum < b.lum;
                          });

                int outCount = qMin(paletteRangerLength, totalCols);
                float step = float(totalCols) / float(outCount);

                // Average groups
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



void MainWindow::reSizeEditorArray(int newWidth, int newHeight){
    // Resize icon_area

    //icon_area_main.assign(newHeight, std::vector<uint8_t>(newWidth, 0));
    //icon_area_scratchpage.assign(newHeight, std::vector<uint8_t>(newWidth, 0));

    icon_area_back.resize(newHeight);
    icon_area_front.resize(newHeight);
    icon_area_scratchpage.resize(newHeight);
    icon_area_backup.resize(newHeight);
    icon_area_redo.resize(newHeight);


    icon_width  = newWidth;
    icon_height = newHeight;
    ui->txtProjectImageWidth->setText(QString("%1").arg(icon_width));
    ui->txtProjectImageHeight->setText(QString("%1").arg(icon_height));

    for (auto &row : icon_area_front)       row.resize(icon_width, 0);  // new cells initialized to 0, existing cells preserved
    for (auto &row : icon_area_back)        row.resize(icon_width, 0);  // new cells initialized to 0, existing cells preserved
    for (auto &row : icon_area_scratchpage) row.resize(icon_width, 0);  // new cells initialized to 0, existing cells preserved
    for (auto &row : icon_area_backup)      row.resize(icon_width, 0);  // new cells initialized to 0, existing cells preserved
    for (auto &row : icon_area_redo)        row.resize(icon_width, 0);  // new cells initialized to 0, existing cells preserved
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

    bool isGood = false;
    if (ui->chkImportPalette->isChecked()) {
        isGIF = extractGifPalette(path, CCLUT);
        if(!isGIF) {
            printf("Not Gif\n");
            isPNG = extractPngPalette(path, CCLUT);
            if(!isPNG)
                printf("not png\n");
        }

        isGood = isGIF + isPNG;

        if(!isGood){    // no pallete data good enough, will have to faff for it our selves
            printf("Palette Faff\n");
            img = img.convertToFormat(QImage::Format_Indexed8);
            ct = img.colorTable();
            for (int i = 0; i < ct.size(); i++) {
                int r = qRed(ct[i + ictoffset]), g = qGreen(ct[i + ictoffset]), b = qBlue(ct[i + ictoffset]);
                CCLUT[i] = (r << 16) | (g << 8) | b;
                CBACKUP_CLUT[i] = CCLUT[i];
            }
        }

        renderPaletteCanvas();
    }

    printf("Source Image width: %d, Height: %d\n", w, h);
    printf("Image Pallete Go? %d\n", isGood);

    reSizeEditorArray(w, h);

    printf("Done resizing editor\n");

    if(paletteRestrictor){
        printf("in the restrictor mode..\n");
        int palStart = 0;
        int palEnd   = paletteDepth;

        if (paletteRestrictor) {
            palStart = paletteRangerOffset;
            palEnd   = paletteRangerOffset + paletteRangerLength;
            if (palEnd > 255) palEnd = 255;
        }

        printf("Preparing Range\n");
        printf("Range: from: %d - %d\n", palStart, palEnd);

        // ===== PIXELS =====
        for (int y = 0; y < h; y++) {
            const uchar* row = img.scanLine(y);
            for (int x = 0; x < w; x++) {

                uint8_t colourIndex = row[x];

                //ct = img.colorTable();

                //if (colourIndex >= ct.size())
                    //colourIndex = ct.size() - 1;


                //int rf = (ct[colourIndex] >> 16) & 0xFF;
                //int gf = (ct[colourIndex] >> 8)  & 0xFF;
                //int bf =  ct[colourIndex]        & 0xFF;
                //int rf = qRed(ct[colourIndex]);
                //int gf = qGreen(ct[colourIndex]);
                //int bf = qBlue(ct[colourIndex]);

                QRgb pix = img.pixel(x, y);
                rf = qRed(pix);
                gf = qGreen(pix);
                bf = qBlue(pix);



                int bestIndex = palStart;
                int bestDist  = INT_MAX;

                // SEARCH ONLY WITHIN RANGE
                for (int pi = palStart; pi < palEnd; pi++) {

                    int r2 = (CCLUT[pi] >> 16) & 0xFF;
                    int g2 = (CCLUT[pi] >> 8)  & 0xFF;
                    int b2 =  CCLUT[pi]        & 0xFF;

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
                (*icon_area)[y][x] = colourIndex;
            }
        }
    } else { // not using the restrictor
        printf("Did we end up here in the NON restrictor mode?\n");
        for (int i = 0; i < 256; i++)
            pal[i] = CCLUT[i];//qRgb(i, i, i);

        // Apply to palette
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

                        int r2 = (CCLUT[pi] >> 16) & 0xFF;
                        int g2 = (CCLUT[pi] >> 8)  & 0xFF;
                        int b2 =  CCLUT[pi]        & 0xFF;

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
                (*icon_area)[y][x] = colourIndex;
            }
        }
    }

    printf("Do we even get this far??\n");
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

    // [ header ]
    fwrite("SBCN", 4, 1, f);// the header type file.

    // [width height] image dimentions
    fwrite(&icon_width, sizeof(uint16_t), 1, f);
    fwrite(&icon_height, sizeof(uint16_t), 1, f);
    fwrite(&cell_width, 1, 1, f);   // cell spacing information
    fwrite(&cell_height, 1, 1, f);

    // [ palette data ]
    fwrite(&CLUTF, 4, 256, f);   // 4 byte @ 256 elements
    fwrite(&CLUTB, 4, 256, f);   // 4 byte @ 256 elements

    // [ palette cycle infos ]
    fwrite(&cyclefrom, 1, 1, f);
    fwrite(&cycleto, 1, 1, f);
    fwrite(&cyclelength, 1, 1, f);
    fwrite(&GradientRangeFrom, 1, 1, f);
    fwrite(&GradientRangeTo, 1, 1, f);

    // [ image body Front]
    for(int y = 0; y < icon_height; y++) {
        fwrite(icon_area_front[y].data(), sizeof(uint8_t), icon_width, f);
    }
    // [ image body Back]
    for(int y = 0; y < icon_height; y++) {
        fwrite(icon_area_back[y].data(), sizeof(uint8_t), icon_width, f);
    }

    fclose(f);
}

void MainWindow::loadProjectIcon(const char *filename){
    FILE *f = fopen(filename, "rb");
    if(!f) { QMessageBox::warning(this, "Load Icon", "Cannot open file!"); return; }

    // read width and height
    uint16_t w, h;
    uint32_t magic_header;

    fread(&magic_header, 4, 1, f);
    //if(magic_header == 0x5342434E){
    if(magic_header == 0x4E434253){


        fread(&w, sizeof(uint16_t), 1, f);
        fread(&h, sizeof(uint16_t), 1, f);
        fread(&cell_width, 1, 1, f);   // cell spacing information
        fread(&cell_height, 1, 1, f);

        ui->txtCellWidth->setText(QString("%1").arg(cell_width));
        ui->txtCellHeight->setText(QString("%1").arg(cell_height));
        // resize rows first
        reSizeEditorArray(w, h);
        reSize();

        // [ palette data front ]
        if(fread(CLUTF, sizeof(uint32_t), 256, f) != 256){
            QMessageBox::warning(this,"Load Icon", "Palette read fail!");
            fclose(f);
            return;
        };

        // [ palette data back ]
        if(fread(CLUTB, sizeof(uint32_t), 256, f) != 256){
            QMessageBox::warning(this,"Load Icon", "Palette read fail!");
            fclose(f);
            return;
        };

        for(int c = 0; c < 256; c++) {
            BACKUP_CLUTF[c] = CLUTF[c];
            BACKUP_CLUTB[c] = CLUTB[c];
        }

        fread(&cyclefrom, 1, 1, f);
        fread(&cycleto, 1, 1, f);
        fread(&cyclelength, 1, 1, f);
        fread(&GradientRangeFrom, 1, 1, f);
        fread(&GradientRangeTo, 1, 1, f);

        // [ Image Body front ]
        for(int y = 0; y < h; y++) {
            if(fread(icon_area_front[y].data(), sizeof(uint8_t), w, f) != w){
                QMessageBox::warning(this, "Load Icon", "Image data corrupted!");
                fclose(f);
                return;
            }
        }

        // [ Image Body back ]
        for(int y = 0; y < h; y++) {
            if(fread(icon_area_back[y].data(), sizeof(uint8_t), w, f) != w){
                QMessageBox::warning(this, "Load Icon", "Image data corrupted!");
                fclose(f);
                return;
            }
        }

        fclose(f);
        CommitIconArea();
        renderEditorCanvas();
        renderPaletteCanvas();
        ui->outputTextView->hide();
        ui->frmFontWorkbench->hide();
        ui->frmOptions->hide();

    } else {
        fclose(f);
        QMessageBox::warning(this, "Load Icon - Error", "Not a Sidbox Icon Project file!"); return;

    }
}

// 0 clockwise, 1 = counter-clockwise
void MainWindow::rotateIcon(int direction){
    if(icon_width == 0 || icon_height == 0) return;

    bool bCellMode = ui->chkCellDivider->isChecked();

    if(!bCellMode){
        // --- Full image mode (same as before) ---
        std::vector<std::vector<uint8_t>> buffer_mnf = icon_area_front;
        std::vector<std::vector<uint8_t>> buffer_mnb = icon_area_back;
        std::vector<std::vector<uint8_t>> buffer_sp  = icon_area_scratchpage;
        std::vector<std::vector<uint8_t>> buffer_bk  = icon_area_backup;
        std::vector<std::vector<uint8_t>> buffer_rd  = icon_area_redo;

        int oldW = icon_width;
        int oldH = icon_height;

        int newW = oldH;
        int newH = oldW;

        auto rotateCW = [](auto &dst, auto &src, int newH, int newW, int oldH, int oldW){
            for(int y = 0; y < newH; ++y)
                for(int x = 0; x < newW; ++x)
                    dst[y][x] = src[oldH - 1 - x][y];
        };

        auto rotateCCW = [](auto &dst, auto &src, int newH, int newW, int oldH, int oldW){
            for(int y = 0; y < newH; ++y)
                for(int x = 0; x < newW; ++x)
                    dst[y][x] = src[x][oldW - 1 - y];
        };

        icon_area_backup.assign(newH, std::vector<uint8_t>(newW, 0));
        icon_area_scratchpage.assign(newH, std::vector<uint8_t>(newW, 0));
        icon_area_redo.assign(newH, std::vector<uint8_t>(newW, 0));
        icon_area_front.assign(newH, std::vector<uint8_t>(newW, 0));
        icon_area_back.assign(newH, std::vector<uint8_t>(newW, 0));

        if(direction == 0){ // clockwise
            rotateCW(icon_area_backup, buffer_bk, newH, newW, oldH, oldW);
            rotateCW(icon_area_scratchpage, buffer_sp, newH, newW, oldH, oldW);
            rotateCW(icon_area_redo, buffer_rd, newH, newW, oldH, oldW);
            rotateCW(icon_area_front, buffer_mnf, newH, newW, oldH, oldW);
            rotateCW(icon_area_back, buffer_mnb, newH, newW, oldH, oldW);
        } else { // counter-clockwise
            rotateCCW(icon_area_backup, buffer_bk, newH, newW, oldH, oldW);
            rotateCCW(icon_area_scratchpage, buffer_sp, newH, newW, oldH, oldW);
            rotateCCW(icon_area_redo, buffer_rd, newH, newW, oldH, oldW);
            rotateCCW(icon_area_front, buffer_mnf, newH, newW, oldH, oldW);
            rotateCCW(icon_area_back, buffer_mnb, newH, newW, oldH, oldW);
        }

        icon_width  = newW;
        icon_height = newH;
    }
    else {
        // --- Cell mode ---
        int cellsPerRow    = icon_width  / cell_width;
        int cellsPerColumn = icon_height / cell_height;

        for(int cy = 0; cy < cellsPerColumn; cy++){
            for(int cx = 0; cx < cellsPerRow; cx++){
                int cellStartX = cx * cell_width;
                int cellStartY = cy * cell_height;

                // Create temporary cell buffers
                std::vector<std::vector<uint8_t>> cell_front(cell_height, std::vector<uint8_t>(cell_width));
                std::vector<std::vector<uint8_t>> cell_back(cell_height, std::vector<uint8_t>(cell_width));
                std::vector<std::vector<uint8_t>> cell_sp(cell_height, std::vector<uint8_t>(cell_width));
                std::vector<std::vector<uint8_t>> cell_bk(cell_height, std::vector<uint8_t>(cell_width));
                std::vector<std::vector<uint8_t>> cell_rd(cell_height, std::vector<uint8_t>(cell_width));

                for(int y = 0; y < cell_height; y++)
                    for(int x = 0; x < cell_width; x++){
                        cell_front[y][x] = icon_area_front[cellStartY + y][cellStartX + x];
                        cell_back[y][x] = icon_area_back[cellStartY + y][cellStartX + x];
                        cell_sp[y][x]   = icon_area_scratchpage[cellStartY + y][cellStartX + x];
                        cell_bk[y][x]   = icon_area_backup[cellStartY + y][cellStartX + x];
                        cell_rd[y][x]   = icon_area_redo[cellStartY + y][cellStartX + x];
                    }

                auto rotateCellCW = [](auto &dst, auto &src, int W, int H){
                    std::vector<std::vector<uint8_t>> tmp(H, std::vector<uint8_t>(W));
                    for(int y = 0; y < H; y++)
                        for(int x = 0; x < W; x++)
                            tmp[y][x] = src[H - 1 - x][y];
                    dst = tmp;
                };

                auto rotateCellCCW = [](auto &dst, auto &src, int W, int H){
                    std::vector<std::vector<uint8_t>> tmp(H, std::vector<uint8_t>(W));
                    for(int y = 0; y < H; y++)
                        for(int x = 0; x < W; x++)
                            tmp[y][x] = src[x][W - 1 - y];
                    dst = tmp;
                };

                if(direction == 0){ // clockwise
                    rotateCellCW(cell_front, cell_front, cell_width, cell_height);
                    rotateCellCW(cell_back, cell_back, cell_width, cell_height);
                    rotateCellCW(cell_sp, cell_sp, cell_width, cell_height);
                    rotateCellCW(cell_bk, cell_bk, cell_width, cell_height);
                    rotateCellCW(cell_rd, cell_rd, cell_width, cell_height);
                } else { // counter-clockwise
                    rotateCellCCW(cell_back, cell_back, cell_width, cell_height);
                    rotateCellCCW(cell_sp, cell_sp, cell_width, cell_height);
                    rotateCellCCW(cell_bk, cell_bk, cell_width, cell_height);
                    rotateCellCCW(cell_rd, cell_rd, cell_width, cell_height);
                }

                // Copy back rotated cells
                for(int y = 0; y < cell_height; y++)
                    for(int x = 0; x < cell_width; x++){
                        icon_area_front[cellStartY + y][cellStartX + x] = cell_front[y][x];
                        icon_area_back[cellStartY + y][cellStartX + x] = cell_back[y][x];
                        icon_area_scratchpage[cellStartY + y][cellStartX + x] = cell_sp[y][x];
                        icon_area_backup[cellStartY + y][cellStartX + x] = cell_bk[y][x];
                        icon_area_redo[cellStartY + y][cellStartX + x] = cell_rd[y][x];
                    }
            }
        }
    }

    ui->txtProjectImageWidth->setText(QString("%1").arg(icon_width));
    ui->txtProjectImageHeight->setText(QString("%1").arg(icon_height));

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

    ui->lblPaletteForeSelect->setPalette(pal);
    CCLUT[numSelectedPaletteID] = (pltColourPreset[0] << 16) | (pltColourPreset[1] << 8) | (pltColourPreset[2]);

    renderPaletteCanvas();

    bit32col = CCLUT[numSelectedPaletteID] | (255 << 24);    // making sure that the alpha channel is full solid

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

    r = CCLUT[numSelectedPaletteID] >> 16;
    g = CCLUT[numSelectedPaletteID] >> 8;
    b = CCLUT[numSelectedPaletteID] & 0xff;

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
            paletteId = (*icon_area)[iy][ix];
            if(capturedPaletteIndex == paletteId){
                (*icon_area)[iy][ix] = targetPalID;
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

    backClut = CCLUT[capturedPaletteIndex];

    CCLUT[capturedPaletteIndex] = CCLUT[targetPalID];

    CCLUT[targetPalID] = backClut;

    ui->cmdSwapColours->setEnabled(true);
    renderPaletteCanvas();
    renderEditorCanvas();
}

void setIconArea(int x, int y){
    if(x < 0 || x >= icon_width) return;
    if(y < 0 || y >= icon_height) return;

    uint8_t &dst = (*icon_area)[y][x];
    uint8_t src = numSelectedPaletteID;

    if(blendopaque <= 0) return;
    if(blendopaque >= 100){
        dst = src;
        return;
    }

    int a = blendopaque; // 0..100

    int sr = CLUTRGB[0][src];
    int sg = CLUTRGB[1][src];
    int sb = CLUTRGB[2][src];

    int dr = CLUTRGB[0][dst];
    int dg = CLUTRGB[1][dst];
    int db = CLUTRGB[2][dst];

    int r = dr + ((sr - dr) * a) / 100;
    int g = dg + ((sg - dg) * a) / 100;
    int b = db + ((sb - db) * a) / 100;

    // nearest palette
    int best = 0;
    int bestDist = INT_MAX;
    for(int i = 0; i < 256; i++){
        int dr2 = r - CLUTRGB[0][i];
        int dg2 = g - CLUTRGB[1][i];
        int db2 = b - CLUTRGB[2][i];
        int dist = dr2*dr2 + dg2*dg2 + db2*db2;
        if(dist < bestDist){
            bestDist = dist;
            best = i;
        }
    }

    dst = uint8_t(best);

}

void clrIconArea(int x, int y){
    if(x<0) return;
    if(y<0) return;
    if(x > icon_width-1) return;
    if(y > icon_height-1) return;
    (*icon_area)[y][x] = numSelectedBackPaletteID;
}

void setIconAreaPen(int sx, int sy, int size, bool set){
    //if(set) setIconArea(sx, sy);
    //else    clrIconArea(sx, sy);
    void (*IconDrawM)(int, int);

    if(set) IconDrawM = setIconArea;
    else    IconDrawM = clrIconArea;

    int dx = sx;
    int dy = sy;

    int x = size/2;
    int y = 0;

    if(bPenShapeCircle){
        int err = 0;
        while(x >= y){
            IconDrawM(dx + x, dy + y);
            IconDrawM(dx + y, dy + x);
            IconDrawM(dx - y, dy + x);
            IconDrawM(dx - x, dy + y);
            IconDrawM(dx - x, dy - y);
            IconDrawM(dx - y, dy - x);
            IconDrawM(dx + y, dy - x);
            IconDrawM(dx + x, dy - y);

            // Fill: draw horizontal spans inside the circle
            for(int fx = dx - x + 1; fx < dx + x; fx++){
                IconDrawM(fx, dy + y);
                IconDrawM(fx, dy - y);
            }
            for(int fx = dx - y + 1; fx < dx + y; fx++){
                IconDrawM(fx, dy + x);
                IconDrawM(fx, dy - x);
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
    } else { // just a square box
        x = size / 2;
        y = size / 2;
        for(int rdy = (dy - y); rdy <= (dy + y); rdy++){
            for(int rdx = (dx - x); rdx <= (dx + x); rdx++){
                IconDrawM(rdx, rdy);
            }
        }
    }
}

void drawHoverBox(int sx, int sy, QImage *edImg){
    int scaleY = icon_zoom;
    int scaleX = icon_zoom;
    int px = sx * icon_zoom;
    int py = sy * icon_zoom;
    for (int dy = 0; dy < scaleY; dy++){
        //if (py + dy >= edImg->height()) break;

        int pycell = py + dy;
        if (pycell < 0 || pycell >= edImg->height()) continue;


        QRgb* scanLine = reinterpret_cast<QRgb*>(edImg->scanLine(py + dy));
        for (int dx = 0; dx < scaleX; dx++){
            //if (px + dx >= edImg->width()) break;
            int pxcell = px + dx;
            if (pxcell < 0 || pxcell >= edImg->width()) continue;
            scanLine[px + dx] = CCLUT[bMouseLeftRight?numSelectedPaletteID:numSelectedBackPaletteID];
        }
    }
}

void drawHoverSelectionBox(int sx, int sy, int sw, int sh, QImage *edImg){
    int z = icon_zoom;

    sw += 1;
    sh += 1;

    int px0 = sx * z;
    int py0 = sy * z;
    int pw  = sw * z;
    int ph  = sh * z;

    /* ---------- Top & Bottom ---------- */
    for (int t = 0; t < z; t++) {
        int yTop = py0 + t;
        int yBot = py0 + ph - z + t;

        if (yTop >= 0 && yTop < edImg->height()) {
            QRgb *line = reinterpret_cast<QRgb*>(edImg->scanLine(yTop));
            for (int x = px0; x < px0 + pw; x++) {
                if (x >= 0 && x < edImg->width())
                    line[x] ^= 0x00FFFFFF;
            }
        }

        if (yBot >= 0 && yBot < edImg->height()) {
            QRgb *line = reinterpret_cast<QRgb*>(edImg->scanLine(yBot));
            for (int x = px0; x < px0 + pw; x++) {
                if (x >= 0 && x < edImg->width())
                    line[x] ^= 0x00FFFFFF;
            }
        }
    }

    /* ---------- Left & Right ---------- */
    for (int t = 0; t < z; t++) {
        int xLeft  = px0 + t;
        int xRight = px0 + pw - z + t;

        for (int y = py0 + z; y < py0 + ph - z; y++) {
            if (y < 0 || y >= edImg->height()) continue;

            QRgb *line = reinterpret_cast<QRgb*>(edImg->scanLine(y));

            if (xLeft >= 0 && xLeft < edImg->width())
                line[xLeft] ^= 0x00FFFFFF;

            if (xRight >= 0 && xRight < edImg->width())
                line[xRight] ^= 0x00FFFFFF;
        }
    }
}


void MainWindow::drawCopyBrushHover(int sx, int sy, QImage *edImg){
    int brush_width  = iCopyWidth;
    int brush_height = iCopyHeight;

    if(icon_copy_area.empty() || brush_width <= 0 || brush_height <= 0)
        return;

    int a = blendopaque;       // 0..100
    if(a <= 0) return;
    int ia = 100 - a;
    bool fullyOpaque = (a >= 100);

    int z = icon_zoom;

    for(int y = 0; y < iCopyHeight; y++){
        for(int x = 0; x < iCopyWidth; x++){
            uint8_t src = icon_copy_area[y][x];
            if(src == 0) continue;

            int sr = CLUTRGB[0][src];
            int sg = CLUTRGB[1][src];
            int sb = CLUTRGB[2][src];

            int px0 = (sx + x) * z;
            int py0 = (sy + y) * z;

            for(int dy = 0; dy < z; dy++){
                int py = py0 + dy;
                if(py < 0 || py >= edImg->height()) continue;

                QRgb *scan = reinterpret_cast<QRgb*>(edImg->scanLine(py));

                for(int dx = 0; dx < z; dx++){
                    int px = px0 + dx;
                    if(px < 0 || px >= edImg->width()) continue;

                    if(fullyOpaque){
                        scan[px] = CCLUT[src]; // fast path
                        continue;
                    }

                    QRgb dstCol = scan[px];
                    int dr = qRed(dstCol);
                    int dg = qGreen(dstCol);
                    int db = qBlue(dstCol);

                    // --- integer blend ---
                    int r = (dr * ia + sr * a) / 100;
                    int g = (dg * ia + sg * a) / 100;
                    int b = (db * ia + sb * a) / 100;

                    // --- nearest palette ---
                    int best = 0;
                    int bestDist = INT_MAX;
                    for(int i = 0; i < 256; i++){
                        int dr2 = r - CLUTRGB[0][i];
                        int dg2 = g - CLUTRGB[1][i];
                        int db2 = b - CLUTRGB[2][i];
                        int dist = dr2*dr2 + dg2*dg2 + db2*db2;
                        if(dist < bestDist){
                            bestDist = dist;
                            best = i;
                        }
                    }

                    scan[px] = CCLUT[best];
                }
            }
        }

    }
}



/*
for(int i = 0; i < 255; i++){
    CLUTRGB[0][i] = (CLUT[i] >>16) & 0xff;
    CLUTRGB[1][i] = (CLUT[i] >>8) & 0xff;
    CLUTRGB[2][i] = (CLUT[i] ) & 0xff;
}
*/
void MainWindow::PlaceBrush(int gridX, int gridY){
    if(icon_copy_area.empty() || iCopyWidth <= 0 || iCopyHeight <= 0)
        return;


    if(blendopaque <= 0) return;

    // Map 0..100% to integer alpha math
    int a = blendopaque;
    int ia = 100 - a;

    bool fullyOpaque = (blendopaque >= 100);

    for(int y = 0; y < iCopyHeight; y++){
        int ty = gridY + y;
        if(ty < 0 || ty >= icon_height) continue;

        for(int x = 0; x < iCopyWidth; x++){
            int tx = gridX + x;
            if(tx < 0 || tx >= icon_width) continue;

            uint8_t src = icon_copy_area[y][x];
            if(src == 0) continue;

            uint8_t &dst = (*icon_area)[ty][tx];

            if(fullyOpaque){
                dst = src;  // fast path
                continue;
            }

            // --- Get precomputed R/G/B ---
            int sr = CLUTRGB[0][src];
            int sg = CLUTRGB[1][src];
            int sb = CLUTRGB[2][src];

            int dr = CLUTRGB[0][dst];
            int dg = CLUTRGB[1][dst];
            int db = CLUTRGB[2][dst];

            // --- Integer blend ---
            int r = (dr * ia + sr * a) / 100;
            int g = (dg * ia + sg * a) / 100;
            int b = (db * ia + sb * a) / 100;

            // --- Find nearest palette index (integer only) ---
            int best = 0;
            int bestDist = INT_MAX;

            for(int i = 0; i < 256; i++){
                int dr2 = r - CLUTRGB[0][i];
                int dg2 = g - CLUTRGB[1][i];
                int db2 = b - CLUTRGB[2][i];
                int dist = dr2*dr2 + dg2*dg2 + db2*db2;

                if(dist < bestDist){
                    bestDist = dist;
                    best = i;
                }
            }

            dst = uint8_t(best);
        }
    }
}



void MainWindow::drawIconAreaPenHover(int sx, int sy, int size, QImage *edImg, bool filled){
    //if(set) setIconArea(sx, sy);
    //else    clrIconArea(sx, sy);

    int dx = sx;
    int dy = sy;

    int x = size/2;
    int y = 0;

    if(bPenShapeCircle){
        int err = 0;
        while(x >= y){
            drawHoverBox(dx + x, dy + y, edImg);
            drawHoverBox(dx + y, dy + x, edImg);
            drawHoverBox(dx - y, dy + x, edImg);
            drawHoverBox(dx - x, dy + y, edImg);
            drawHoverBox(dx - x, dy - y, edImg);
            drawHoverBox(dx - y, dy - x, edImg);
            drawHoverBox(dx + y, dy - x, edImg);
            drawHoverBox(dx + x, dy - y, edImg);

            // Fill: draw horizontal spans inside the circle
            if(filled){
                for(int fx = dx - x + 1; fx < dx + x; fx++){
                    drawHoverBox(fx, dy + y, edImg);
                    drawHoverBox(fx, dy - y, edImg);
                }
                for(int fx = dx - y + 1; fx < dx + y; fx++){
                    drawHoverBox(fx, dy + x, edImg);
                    drawHoverBox(fx, dy - x, edImg);
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
    } else { // just a square box
        x = size / 2;
        y = size / 2;
        for(int rdy = (dy - y); rdy <= (dy + y); rdy++){
            for(int rdx = (dx - x); rdx <= (dx + x); rdx++){
                drawHoverBox(rdx, rdy, edImg);
            }
        }
    }
}


#define DENSITY_SCALE 64
void MainWindow::onSprayCanTick(){
    // all the params should already be known by the time this is triggereded

    int osx, osy, dsx, dsy;
    int rsx, rsy;   // results

    // source location
    dsx = iSpraySX;
    dsy = iSpraySY;

    int radius = iPenShapeSize;
    int progSteps ;


    // this is to make it seem constant spread over larger areas, so BIGGER brushes will want MORE itterations, at the scale of our iSprayRate
    int r = iPenShapeSize;   // radius
    if( r<1) r=1;
    progSteps = 1 + (1 * r * r) / (100 - iSprayRate);

    if(progSteps < 1) progSteps = 1;


    // osx, osy - are used for the offsets, but they'll need to be calculated in a shape of a circle, cos, sin stuff (yey maths time)

    for(int steps = 0; steps < progSteps; steps++){
        if(bPenShapeCircle){    // calculate in a circle area
            do {
                osx = (rand() % (radius * 2 + 1)) - radius;
                osy = (rand() % (radius * 2 + 1)) - radius;
            } while (osx*osx + osy*osy > radius*radius);
        } else {                // otherwise just a simple boxey area
            osx = (rand() % (radius * 2 + 1)) - radius;
            osy = (rand() % (radius * 2 + 1)) - radius;

        }

        rsx = dsx + osx;
        rsy = dsy + osy;

        if(bSprayingTheCan){    // only works when the conditions are set, BUT i've put this here just incase the timer triggers AFTER the update to Spraycan
            if(bSprayDraw)
                setIconArea(rsx, rsy);
            else
                clrIconArea(rsx, rsy);
        }
    }

    renderEditorCanvas();
}


// in your MainWindow class
bool grabbing = false;
QPoint grabStartMouse;      // screen pos when SPACE pressed
int grabStartScrollH, grabStartScrollV;

bool MainWindow::eventFilter(QObject *obj, QEvent *event){
    uint8_t r,g,b;


    if(event->type() == QEvent::MouseButtonPress) {
        // will need these for the tool buttons that need the left and right bitsies
        if(obj == ui->toolButtonRect){
            auto *me = static_cast<QMouseEvent*>(event);
            QPoint p = me->pos(); // position INSIDE button
            if (p.x() < ui->toolButtonRect->width() / 2) {
                bFillToolIn = false;
            } else {
                bFillToolIn = true;
            }
        }

        if(obj == ui->toolButtonCircle){
            auto *me = static_cast<QMouseEvent*>(event);
            QPoint p = me->pos(); // position INSIDE button
            if (p.x() < ui->toolButtonCircle->width() / 2) {
                bFillToolIn = false;
            } else {
                bFillToolIn = true;
            }
        }

        if(obj == ui->toolButtonFloodFill){
            auto *me = static_cast<QMouseEvent*>(event);
            QPoint p = me->pos(); // position INSIDE button
            if (p.x() < ui->toolButtonCircle->width() / 2) {
                currentDrawMode = FloodFill;
            } else {
                gradientDragging = true;    // initially the floodfill will do nothing right now
                currentDrawMode = FloodFillGradient;
            }
        }
    }

    // Palette Selector
    if(obj == ui->gfxPalleteSelect){
        if (event->type() == QEvent::KeyPress){ // capture current Colour Palette Index.
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            if (ke->isAutoRepeat()) return true; // ignore repeats
            //printf("Palette keypress event\n");
            if (ke->key() == Qt::Key_Control){
                clickedIndex = numSelectedPaletteID;
                selectingCycle = true;
                waitingForEnd = false;
                cyclefrom = clickedIndex;
                //printf("Cycle Setting Start: Start pos %lu\n", clickedIndex);
                return true;
            }
            if (ke->key() == Qt::Key_Alt){
                clickedIndex = numSelectedPaletteID;
                selectingGradientRange = true;
                waitingForEndGradient = false;
                GradientRangeFrom = clickedIndex;
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
            if (ke->key() == Qt::Key_Alt) {
                selectingGradientRange = false;
                waitingForEndGradient = false;
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


            if(mouseEvent->buttons() == Qt::RightButton){
                //printf("Right Clicked on the Palette  ");

                unsigned char r, g, b;
                int tSelectedX = x / PALETTE_BOX_HSIZE;
                int tSelectedY = y / PALETTE_BOX_VSIZE;

                numSelectedBackPaletteID = (tSelectedX + (tSelectedY * PALETTE_WIDTH));

                r = (CCLUT[numSelectedBackPaletteID] >> 16) & 0xff;
                g = (CCLUT[numSelectedBackPaletteID] >> 8) & 0xff;
                b = (CCLUT[numSelectedBackPaletteID]) & 0xff;

                pal.setColor(QPalette::Window, QColor(r, g, b)); // RGB
                ui->lblPaletteColour->setAutoFillBackground(true);   // must enable for background
                ui->lblPaletteColour->setPalette(pal);

                ui->lblPaletteBackSelect->setPalette(pal);

                return true;
            }


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
            if(selectingGradientRange == true){
                GradientRangeTo = numSelectedPaletteID;
                if(GradientRangeTo < GradientRangeFrom)
                    std::swap(GradientRangeTo, GradientRangeFrom);
            }

            // clicked on another colour - ONLY if selected another colour
            if(numPrevSelectedPaletteID != numSelectedPaletteID){
                for(int i=0; i<256; i++){
                    //CBACKUP_CLUT[i] = CCLUT[i];
                    BACKUP_CLUTF[i] = CLUTF[i];
                    BACKUP_CLUTB[i] = CLUTB[i];
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
            if(ke->key() == static_cast<int>(KeyBinding::kscPanZoom) && !grabbing && !ke->isAutoRepeat()){
                grabbing = true;
                gradientDragging = false;
                setCursor(Qt::ClosedHandCursor);
                return true;
            }
            if(ke->key() == Qt::Key_Shift && !ke->isAutoRepeat()){
                bShiftKey = true;
            }

            // SWAP BUFFERS (scratch pad or icon_area)
            if(ke->key() == static_cast<int>(KeyBinding::kscSwapScreen) && !ke->isAutoRepeat()){
                //ui->frmGraphicEdit->bac
                QPalette pal = ui->frmGraphicEdit->palette();
                bEditorPage = 1 - bEditorPage;  // simple dirty toggle
                if(bEditorPage){
                    pal.setColor(QPalette::Window, Qt::green);
                    ui->frmGraphicEdit->setAutoFillBackground(true);
                    ui->frmGraphicEdit->setPalette(pal);

                    icon_area = &icon_area_scratchpage;

                } else {
                    ui->frmGraphicEdit->setPalette(QPalette());
                    icon_area = active_icon_area;
                }
                renderEditorCanvas();
                return true;

            }
        }
        // keyshortcuts
        if(event->type() == QEvent::KeyRelease){
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            if(ke->key() == static_cast<int>(KeyBinding::kscPanZoom) && grabbing && !ke->isAutoRepeat()){
                grabbing = false;
                setCursor(Qt::ArrowCursor);
                return true;
            }
            // UNDO
            if (ke->key() == static_cast<int>(KeyBinding::kscUndo) && (ke->modifiers() & Qt::ControlModifier) && !ke->isAutoRepeat()){
                UndoIconArea();
                renderEditorCanvas();
                return true;
            }
            if(ke->key() == Qt::Key_Shift){
                bShiftKey = false;
            }
            if (ke->key() == static_cast<int>(KeyBinding::kscZoomeOut)){
                int newVal = ui->scrEditorZoomVal->value();
                //newVal += delta;

                newVal --;

                if(newVal>32) newVal = 32;
                if(newVal<1) newVal = 1;

                ui->scrEditorZoomVal->setValue(newVal);
                icon_zoom = ui->scrEditorZoomVal->value();
                ui->lblEditorZoomLevel->setText(QString("%1").arg(icon_zoom));
                reSize();
                renderEditorCanvas();
            }
            if (ke->key() == static_cast<int>(KeyBinding::kscZoomIn)){
                int newVal = ui->scrEditorZoomVal->value();
                //newVal += delta;

                newVal ++;

                if(newVal>32) newVal = 32;
                if(newVal<1) newVal = 1;

                ui->scrEditorZoomVal->setValue(newVal);
                icon_zoom = ui->scrEditorZoomVal->value();
                ui->lblEditorZoomLevel->setText(QString("%1").arg(icon_zoom));
                reSize();
                renderEditorCanvas();
            }

            if(!ke->isAutoRepeat()){
                if (ke->key() == static_cast<int>(KeyBinding::kscBrushFlipX)){
                    doAlterBrush(brushflipX);
                    renderEditorCanvas();
                }
                if (ke->key() == static_cast<int>(KeyBinding::kscBrushFlipY)){
                    doAlterBrush(brushflipY);
                    renderEditorCanvas();
                }
                if (ke->key() == static_cast<int>(KeyBinding::kscBrushRotateCC)){
                    doAlterBrush(brushrotateL);
                    renderEditorCanvas();
                }
                if (ke->key() == static_cast<int>(KeyBinding::kscBrushRotateCW)){
                    doAlterBrush(brushrotateR);
                    renderEditorCanvas();
                }

                if (ke->key() == static_cast<int>(KeyBinding::kscPenSize1)){
                    clearBrushSizeButtons(0);
                    renderEditorCanvas();
                }
                if (ke->key() == static_cast<int>(KeyBinding::kscPenSize2)){
                    clearBrushSizeButtons(1);
                    renderEditorCanvas();
                }
                if (ke->key() == static_cast<int>(KeyBinding::kscPenSize3)){
                    clearBrushSizeButtons(2);
                    renderEditorCanvas();
                }
                if (ke->key() == static_cast<int>(KeyBinding::kscPenSize4)){
                    clearBrushSizeButtons(3);
                    renderEditorCanvas();
                }
                if (ke->key() == static_cast<int>(KeyBinding::kscPenSize5)){
                    clearBrushSizeButtons(4);
                    renderEditorCanvas();
                }

                if (ke->key() == static_cast<int>(KeyBinding::kscBlendDecrease)){
                    blendopaque -= 10;// by 10
                    if(blendopaque<10) blendopaque = 10;
                    ui->scrBrushBlend->setValue(blendopaque);
                    ui->lblBlendValue->setText(QString("%1%").arg(blendopaque));
                }
                if (ke->key() == static_cast<int>(KeyBinding::kscBlendIncrease)){
                    blendopaque += 10;// by 10
                    if(blendopaque>100) blendopaque = 100;
                    ui->scrBrushBlend->setValue(blendopaque);
                    ui->lblBlendValue->setText(QString("%1%").arg(blendopaque));
                }


                if (ke->key() == static_cast<int>(KeyBinding::kscToolSelectPlot)){
                    clearToolButtons();
                    ui->toolButtonPlot->setChecked(true);   // keep this high lighted!
                    currentDrawMode = Plot;
                    renderEditorCanvas();
                };

                if (ke->key() == static_cast<int>(KeyBinding::kscToolSelectLine)){
                    clearToolButtons();
                    ui->toolButtonLine->setChecked(true);   // keep this high lighted!
                    currentDrawMode = Line;
                    renderEditorCanvas();
                };

                if (ke->key() == static_cast<int>(KeyBinding::kscToolSelectPen)){
                    clearToolButtons();
                    ui->toolButtonPen->setChecked(true);   // keep this high lighted!
                    currentDrawMode = Pen;
                    renderEditorCanvas();
                };

                if (ke->key() == static_cast<int>(KeyBinding::kscToolSelectSpray)){
                    clearToolButtons();
                    ui->toolButtonSprayCan->setChecked(true);   // keep this high lighted!
                    currentDrawMode = SprayCan;
                    renderEditorCanvas();
                };



            }
        }
        return QObject::eventFilter(obj, event);
    }

    if(obj == ui->gfxEditor->viewport()){

        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (event->type() == QEvent::MouseButtonPress){
            bMouseButtonDown = true;
            if(grabbing) {
                if(mouseEvent->buttons() & Qt::LeftButton) {
                    grabStartMouse = QCursor::pos();
                    grabStartScrollH = ui->scrEditorH->value();
                    grabStartScrollV = ui->scrEditorV->value();
                }
            } else {
                // annoying but more a glue logic:
                if(currentDrawMode == FloodFillGradient)
                    gradientDragging = true;
            }

            if(mouseEvent->buttons() & Qt::LeftButton)
                bMouseLeftRight = true;
            else
                bMouseLeftRight = false;
        }


        if (event->type() == QEvent::MouseMove){
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePos = ui->gfxEditor->mapToScene(mouseEvent->pos());
            ui->gfxEditor->setFocus();

            int coordX = int(scenePos.x()) / icon_zoom;
            int coordY = int(scenePos.y()) / icon_zoom;

            if(coordX < 0) coordX = 0;
            if(coordY < 0) coordY = 0;
            if(coordX > editorViewPortWidth - 1)  hoverPixelX = editorViewPortWidth-1;
            if(coordY > editorViewPortHeight - 1) coordY = editorViewPortHeight-1;

            int dx = coordX + xOffset;
            int dy = coordY + yOffset;


            int cWidth = cell_width;
            int cHeight = cell_height;
            QString text = "";
            if(ui->chkCellDivider->isChecked()) {
                if(cWidth < 1) cWidth = 1;
                if(cHeight < 1) cHeight = 1;
                int cellIndex = (dx / cWidth) + ((dy / cHeight) * (icon_width / cWidth));
                text = QString("Coords: X:%1, Y:%2 / CELL ID: %3")
                                   .arg(dx, 4, 10, QLatin1Char('0'))
                                   .arg(dy, 4, 10, QLatin1Char('0'))
                                   .arg(cellIndex);

            }
            else
                text = QString("Coords: X:%1, Y:%2")
                                   .arg(dx, 4, 10, QChar('0'))
                                   .arg(dy, 4, 10, QChar('0'));

            ui->lblCoords->setText(text);

            if(!bCapturingCopyArea){

                if(bMouseButtonDown && bShiftKey){
                    if((currentDrawMode == PasteBrush) || (currentDrawMode == DrawText))
                        ProcessClickPaint(hoverPixelX + xOffset, hoverPixelY + yOffset, DrawUIMode_InitButton | DrawUIMode_LeftMouseButton); // initial click
                }

                if(gradientDragging){
                    QPoint pos = mouseEvent->pos();
                    //int mx = pos.x() / icon_zoom + ui->scrEditorH->value();
                    //int my = pos.y() / icon_zoom + ui->scrEditorV->value();

                    hoverPixelX = int(scenePos.x()) / icon_zoom;
                    hoverPixelY = int(scenePos.y()) / icon_zoom;

                    float dx = hoverPixelX - gradStartX;
                    float dy = hoverPixelY - gradStartY;

                    gradAngle = atan2f(dy, dx) * (180.0f / M_PI);

                    // Length (distance)
                    float dist = std::sqrt(dx*dx + dy*dy);
                    gradLength = (int)(dist);


                    //printf("Angle TO Grad: %f, len:%d\n", gradAngle, gradLength);
                } else
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

                    int dx = hoverPixelX + xOffset;
                    int dy = hoverPixelY + yOffset;

                    if(mouseEvent->buttons() & Qt::LeftButton)  ProcessClickPaint(dx, dy, DrawUIMode_LeftMouseButton);  // moving draw process
                    if(mouseEvent->buttons() & Qt::RightButton) ProcessClickPaint(dx, dy, DrawUIMode_RightMouseButton); // moving draw process


                    if(captureXYStart==true){
                        ltcapturedX = ctcapturedX;
                        ltcapturedY = ctcapturedY;
                        readToolXY(&ctcapturedX, &ctcapturedY); // process where the capturey point is
                    }
                }
            } else {
                if(bGrabbedCopyStart){
                    QPointF scenePos = ui->gfxEditor->mapToScene(mouseEvent->pos());
                    ui->gfxEditor->setFocus();

                    hoverPixelX = int(scenePos.x()) / icon_zoom;
                    hoverPixelY = int(scenePos.y()) / icon_zoom;

                    hoverPixelX = std::clamp(hoverPixelX, 0, editorViewPortWidth  - 1);
                    hoverPixelY = std::clamp(hoverPixelY, 0, editorViewPortHeight - 1);

                    int dx = hoverPixelX + xOffset;
                    int dy = hoverPixelY + yOffset;

                    dx = std::clamp(dx, 0, icon_width  - 1);
                    dy = std::clamp(dy, 0, icon_height - 1);

                    int dxw = dx - iCapturedCopyX;
                    int dyh = dy - iCapturedCopyY;

                    iCopyWidth  = abs(dxw);
                    iCopyHeight = abs(dyh);

                    int copyX = (dxw < 0) ? dx : iCapturedCopyX;
                    int copyY = (dyh < 0) ? dy : iCapturedCopyY;

                    iTargetCopyX = copyX;
                    iTargetCopyY = copyY;
                    renderEditorCanvas();
                }
            }

            updateTimer->start(1);
            return true; // event handled
        } else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            bMouseButtonDown = false;
            if (mouseEvent->button() == Qt::LeftButton) {
                //printf( "Left click at %lu\n",  mouseEvent->pos());

                // check if we're in a tool that needs point to point interaction
                // if() {
                //todo: make the circle tool have long inverted cross hairs in the overlay draw

                if(bCapturingCopyArea){
                    bCapturingCopyArea = false;
                    bGrabbedCopyStart = false;
                    CopySelectionToBrush(); // now perform the bit where we copy
                    currentDrawMode = PasteBrush;   // now this is the bit where we should be able to see the brush and draw it later
                    renderEditorCanvas();
                }
                if(!grabbing){
                    tmrSprayCanTimer->stop();   // regardless, should stop the spray can just incase, stuck
                    bSprayingTheCan = false;

                    if(captureXYStart==true){
                        int xOffset = ui->scrEditorH->value();
                        int yOffset = ui->scrEditorV->value();
                        if(currentDrawMode == FloodFillGradient){
                            int x1 = ctcapturedX + xOffset;
                            int y1 = ctcapturedY + yOffset;
                            gradientDragging = true;    // restart this
                            floodFillGradient(x1, y1, GradientRangeFrom, GradientRangeTo, gradLength, gradAngle);
                        }
                        if(currentDrawMode == Line){
                            int x0 = capturedX + xOffset;
                            int y0 = capturedY + yOffset;
                            int x1 = ctcapturedX + xOffset;
                            int y1 = ctcapturedY + yOffset;

                            int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
                            int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
                            int err = dx + dy, e2;

                            int halfPen = iPenShapeSize / 2; // half thickness
                            int x = x0, y = y0;

                            auto setThickPixel = [&](int px, int py){
                                for(int yy = -halfPen; yy <= halfPen; yy++){
                                    int ny = py + yy;
                                    if(ny < 0 || ny >= icon_height) continue;
                                    for(int xx = -halfPen; xx <= halfPen; xx++){
                                        int nx = px + xx;
                                        if(nx < 0 || nx >= icon_width) continue;
                                        setIconArea(nx, ny); // commit color to icon_area
                                    }
                                }
                            };

                            int prevX = x;
                            int prevY = y;

                            auto setSoftPixel = [&](int px, int py){
                                if(px < 0 || px >= icon_width || py < 0 || py >= icon_height)
                                    return;
                                setIconArea(px, py);
                            };

                            while(true){
                                setThickPixel(x, y);

                                if(x == x1 && y == y1)
                                    break;

                                prevX = x;
                                prevY = y;

                                e2 = 2 * err;
                                if(e2 >= dy){ err += dy; x += sx; }
                                if(e2 <= dx){ err += dx; y += sy; }

                                // --- CHEAP AA (ONLY FOR SMALL PENS) ---
                                // --- CHEAP AA (pen-thickness aware) ---
                                if(ui->chkPenDrawAA->isChecked()){
                                    // for small pens, just do your old single-pixel AA
                                    if(iPenShapeSize <= 2){
                                        if(x != prevX && y != prevY){
                                            setAAPixel(prevX, y, numSelectedPaletteID);
                                            setAAPixel(x, prevY, numSelectedPaletteID);
                                        }
                                    } else {
                                        // for thicker pens, spread AA around the edges
                                        for(int yy = -halfPen; yy <= halfPen; yy++){
                                            for(int xx = -halfPen; xx <= halfPen; xx++){
                                                int nx1 = prevX + xx;
                                                int ny1 = y + yy;
                                                int nx2 = x + xx;
                                                int ny2 = prevY + yy;
                                                setAAPixel(nx1, ny1, numSelectedPaletteID);
                                                setAAPixel(nx2, ny2, numSelectedPaletteID);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if(currentDrawMode == Rect){
                            int x0 = capturedX + xOffset;
                            int y0 = capturedY + yOffset;
                            int x1 = hoverPixelX + xOffset;
                            int y1 = hoverPixelY + yOffset;

                            // Ensure top-left -> bottom-right
                            int left   = std::min(x0, x1);
                            int right  = std::max(x0, x1);
                            int top    = std::min(y0, y1);
                            int bottom = std::max(y0, y1);

                            int penW = std::min(iPenShapeSize, right - left + 1);
                            int penH = std::min(iPenShapeSize, bottom - top + 1);

                            for(int y = top; y <= bottom; y++){
                                for(int x = left; x <= right; x++){
                                    bool drawPixel = false;

                                    // Top edge
                                    if(y - top < penH) drawPixel = true;
                                    // Bottom edge
                                    else if(bottom - y < penH) drawPixel = true;
                                    // Left edge
                                    else if(x - left < penW) drawPixel = true;
                                    // Right edge
                                    else if(right - x < penW) drawPixel = true;
                                    // Fill inside
                                    else if(bFillToolIn) drawPixel = true;

                                    if(drawPixel) setIconArea(x, y);
                                }
                            }
                        }

                        if(currentDrawMode == Circle){
                            int cx = capturedX + xOffset;
                            int cy = capturedY + yOffset;
                            int radius = std::max(abs(hoverPixelX - capturedX), abs(hoverPixelY - capturedY));
                            int halfPen = iPenShapeSize / 2;

                            int outerRadius = radius + halfPen;
                            int innerRadius = std::max(0, radius - halfPen);

                            auto setOutlinePixel = [&](int x, int y){
                                if(x < 0 || x >= icon_width || y < 0 || y >= icon_height)
                                    return;
                                setIconArea(x, y);
                            };

                            for(int y = cy - outerRadius; y <= cy + outerRadius; y++){
                                for(int x = cx - outerRadius; x <= cx + outerRadius; x++){
                                    int dx = x - cx;
                                    int dy = y - cy;
                                    float dist = std::sqrt(dx*dx + dy*dy);

                                    float epsilon = 0.5f; // small tolerance
                                    if(dist >= innerRadius - epsilon && dist <= outerRadius + epsilon){
                                        // Outline
                                        setOutlinePixel(x, y);

                                        // --- CHEAP AA (ONLY FOR SMALL PENS) ---
                                        if(ui->chkPenDrawAA->isChecked()){// && iPenShapeSize <= 2){
                                            // touch the neighboring pixels diagonally
                                            setAAPixel(x+1, y, numSelectedPaletteID);
                                            setAAPixel(x-1, y, numSelectedPaletteID);
                                            setAAPixel(x, y+1, numSelectedPaletteID);
                                            setAAPixel(x, y-1, numSelectedPaletteID);
                                        }

                                    } else if(bFillToolIn && dist < innerRadius){
                                        // Fill inside
                                        setOutlinePixel(x, y);
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
                CommitIconArea();   // copy what we have right now to backup

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
                if(gradientDragging){

                    QPointF current = mouseEvent->globalPosition();
                    QPointF scenePos = ui->gfxEditor->mapToScene(mouseEvent->pos());
                    ui->gfxEditor->setFocus();
                    int xOffset = ui->scrEditorH->value();
                    int yOffset = ui->scrEditorV->value();

                    hoverPixelX = int(scenePos.x()) / icon_zoom;
                    hoverPixelY = int(scenePos.y()) / icon_zoom;

                    gradStartX = hoverPixelX;
                    gradStartY = hoverPixelY;
                    return true;
                }

                if(bCapturingCopyArea){
                    if(mouseEvent->buttons() & Qt::LeftButton) {
                        QPointF current = mouseEvent->globalPosition();
                        QPointF scenePos = ui->gfxEditor->mapToScene(mouseEvent->pos());
                        ui->gfxEditor->setFocus();
                        int xOffset = ui->scrEditorH->value();
                        int yOffset = ui->scrEditorV->value();

                        hoverPixelX = int(scenePos.x()) / icon_zoom;
                        hoverPixelY = int(scenePos.y()) / icon_zoom;

                        if(hoverPixelX < 0) hoverPixelX = 0;
                        if(hoverPixelY < 0) hoverPixelY = 0;
                        if(hoverPixelX > editorViewPortWidth - 1)  hoverPixelX = editorViewPortWidth-1;
                        if(hoverPixelY > editorViewPortHeight - 1) hoverPixelY = editorViewPortHeight-1;

                        int dx = hoverPixelX + xOffset;
                        int dy = hoverPixelY + yOffset;

                        iCapturedCopyX = dx;
                        iCapturedCopyY = dy;
                        iCopyWidth = 0;
                        iCopyHeight = 0;

                        bGrabbedCopyStart = true;
                    }
                } else {
                    if(!grabbing){
                        ProcessClickPaint(hoverPixelX + xOffset, hoverPixelY + yOffset, DrawUIMode_InitButton | DrawUIMode_LeftMouseButton); // initial click
                    }
                }
                updateTimer->start(1);
            }
            else if (mouseEvent->button() == Qt::RightButton) {
                CommitIconArea();   // copy what we have right now to backup
                //printf( "Right click atlu\n",  mouseEvent->pos());
                ProcessClickPaint(hoverPixelX + xOffset, hoverPixelY + yOffset, DrawUIMode_InitButton | DrawUIMode_RightMouseButton); // initial click
                updateTimer->start(1);

            }
            else if (mouseEvent->button() == Qt::MiddleButton){
                numSelectedPaletteID = (*icon_area)[hoverPixelY + yOffset][hoverPixelX + xOffset];
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

static void buildTextPtrFromQString(const QString &src, char *outBuf, int outBufSize){
    if (!outBuf || outBufSize <= 0) return;

    QByteArray ba;
    ba.reserve(outBufSize);

    for (int i = 0; i < src.length(); ){
        // Detect $0xNN
        if (src[i] == '$' && i + 4 < src.length() && src[i + 1] == '0' && (src[i + 2] == 'x' || src[i + 2] == 'X')) {
            bool ok = false;
            QString hex = src.mid(i + 3, 2);
            int value = hex.toInt(&ok, 16);

            if (ok && value >= 0 && value <= 255) {
                ba.append(static_cast<char>(value)); // exact byte
                i += 5;
                continue;
            }
        }

        // Normal character → force to raw 8-bit
        ushort uc = src[i].unicode();
        ba.append(static_cast<char>(uc & 0xFF));
        i++;
    }

    int out = 0;
    for (; out < ba.size() && out < outBufSize - 1; ++out)
        outBuf[out] = ba[out];
    outBuf[out] = '\0';
}


void MainWindow::drawTextHover(int sx, int sy, QImage *edImg){
    // in this section we're only drawing over the top of what is already rendered, but NOT commiting it to icon_area.
    char textptr[128];
    QString src = ui->txtTextDrawText->toPlainText();

    if (!edImg) return;

    buildTextPtrFromQString(src, textptr, sizeof(textptr));

    int x = sx;
    int y = sy;

    const QRgb colf = CCLUT[bMouseLeftRight?numSelectedPaletteID:numSelectedBackPaletteID]; // foreground color
    const int zoom = icon_zoom; // base editor zoom

    const int scaleX = zoom * iTextWidth;   // horizontal block size
    const int scaleY = zoom * iTextHeight;  // vertical block size

    for (int32_t i = 0; textptr[i] != '\0'; ++i){
        if (textptr[i] == '\n') {
            x = sx;
            y += 8 * scaleY; // next line
            continue;
        }

        //if ((uint32_t)x >= edImg->width() || (uint32_t)y >= edImg->height())            continue;


        const uint8_t* pixeldata = SYSFONT[(uint8_t)textptr[i]];

        for (int row = 0; row < 8; ++row){
            int py = y + row * scaleY;
            //if ((uint32_t)py >= edImg->height()) break;
            if ((py + scaleY <= 0) || (py >= edImg->height())) // fully off-screen vertically
                continue;

            for (int col = 0; col < 8; ++col){
                if (pixeldata[col] & (1 << row)){
                    int px = x + col * scaleX;

                    //if ((uint32_t)px >= edImg->width()) break;
                    if ((px + scaleX <= 0) || (px >= edImg->width())) // fully off-screen horizontally
                        continue;

                    // Fill block taking both zoom and text scaling into account
                    for (int dy = 0; dy < scaleY; dy++){
                        //if (py + dy >= edImg->height()) break;

                        int pycell = py + dy;
                        if (pycell < 0 || pycell >= edImg->height()) continue;


                        QRgb* scanLine = reinterpret_cast<QRgb*>(edImg->scanLine(py + dy));
                        for (int dx = 0; dx < scaleX; dx++){
                            //if (px + dx >= edImg->width()) break;
                            int pxcell = px + dx;
                            if (pxcell < 0 || pxcell >= edImg->width()) continue;


                            scanLine[px + dx] = colf;
                        }
                    }
                }
            }
        }
        x += 8 * scaleX; // advance for next character
    }

}

void MainWindow::drawText(int sx, int sy, bool setPixel){
    bool outlineDraw = ui->chkTextOutline->isChecked();
    char textptr[128];
    QString src = ui->txtTextDrawText->toPlainText();
    buildTextPtrFromQString(src, textptr, sizeof(textptr));

    int x = sx;
    int y = sy;

    const int zoom = icon_zoom;       // editor zoom
    const int scaleX = iTextWidth;    // horizontal block size
    const int scaleY = iTextHeight;   // vertical block size

    for (int32_t i = 0; textptr[i] != '\0'; ++i){
        if (textptr[i] == '\n') {
            x = sx;
            y += 8 * scaleY; // next line
            continue;
        }

        const uint8_t* pixeldata = SYSFONT[(uint8_t)textptr[i]];

        // --- First pass: stroke outline ---
        if(outlineDraw){
            for (int row = 0; row < 8; ++row){
                int py = y + row * scaleY;
                if (py + scaleY <= 0 || py >= icon_height) continue;

                for (int col = 0; col < 8; ++col){
                    if (!(pixeldata[col] & (1 << row))) continue;

                    int px = x + col * scaleX;
                    if (px + scaleX <= 0 || px >= icon_width) continue;

                    // Draw outline around the pixel
                    for(int dy = -1; dy <= 1; dy++){
                        for(int dx = -1; dx <= 1; dx++){
                            if(dx == 0 && dy == 0) continue; // skip center
                            int nx = px + dx * scaleX;
                            int ny = py + dy * scaleY;

                            for(int ddy = 0; ddy < scaleY; ddy++){
                                int pycell = ny + ddy;
                                if(pycell < 0 || pycell >= icon_height) continue;

                                for(int ddx = 0; ddx < scaleX; ddx++){
                                    int pxcell = nx + ddx;
                                    if(pxcell < 0 || pxcell >= icon_width) continue;

                                    clrIconArea(pxcell, pycell); // back color for outline
                                }
                            }
                        }
                    }
                }
            }
        }

        // --- Second pass: main text ---
        for (int row = 0; row < 8; ++row){
            int py = y + row * scaleY;
            if (py + scaleY <= 0 || py >= icon_height) continue;

            for (int col = 0; col < 8; ++col){
                if (!(pixeldata[col] & (1 << row))) continue;

                int px = x + col * scaleX;
                if (px + scaleX <= 0 || px >= icon_width) continue;

                for(int dy = 0; dy < scaleY; dy++){
                    int pycell = py + dy;
                    if(pycell < 0 || pycell >= icon_height) continue;

                    for(int dx = 0; dx < scaleX; dx++){
                        int pxcell = px + dx;
                        if(pxcell < 0 || pxcell >= icon_width) continue;

                        if(setPixel)
                            setIconArea(pxcell, pycell); // main text color
                        else
                            clrIconArea(pxcell, pycell);
                    }
                }
            }
        }

        x += 8 * scaleX; // advance to next character
    }

}


void MainWindow::getTextCenterHandle(int sx, int sy, int* outX, int* outY){
    QByteArray ba = ui->txtTextDrawText->toPlainText().toUtf8();
    const char* textptr = ba.constData();

    int lineWidth = 0;
    int maxWidth = 0;
    int totalHeight = 8 * iTextHeight; // first line

    for (int i = 0; textptr[i] != '\0'; i++) {
        if (textptr[i] == '\n') {
            if (lineWidth > maxWidth) maxWidth = lineWidth;
            lineWidth = 0;
            totalHeight += 8 * iTextHeight;
        } else {
            lineWidth += 8 * iTextWidth;
        }
    }
    if (lineWidth > maxWidth) maxWidth = lineWidth;

    int x = sx;
    int y = sy;

    switch (cBrushHandleMode) {
        case cHandleTL: break;

        case cHandleTM:
            x -= maxWidth / 2;
            break;

        case cHandleTR:
            x -= maxWidth;
            break;

        case cHandleML:
            y -= totalHeight / 2;
            break;

        case cHandleMM:
            x -= maxWidth / 2;
            y -= totalHeight / 2;
            break;

        case cHandleMR:
            x -= maxWidth;
            y -= totalHeight / 2;
            break;

        case cHandleBL:
            y -= totalHeight;
            break;

        case cHandleBM:
            x -= maxWidth / 2;
            y -= totalHeight;
            break;

        case cHandleBR:
            x -= maxWidth;
            y -= totalHeight;
            break;
    }

    *outX = x;
    *outY = y;
}


void MainWindow::getCenterHandle(int sx, int sy, int* outX, int* outY, int width, int height){
    //*outX = sx - width / 2;
    //*outY = sy - height / 2;

    int ox = sx;
    int oy = sy;

    switch(cBrushHandleMode){
        case cHandleTL:
            // no offset
            break;

        case cHandleTM:
            ox -= width / 2;
            break;

        case cHandleTR:
            ox -= width;
            break;

        case cHandleML:
            oy -= height / 2;
            break;

        case cHandleMM:
            ox -= width / 2;
            oy -= height / 2;
            break;

        case cHandleMR:
            ox -= width;
            oy -= height / 2;
            break;

        case cHandleBL:
            oy -= height;
            break;

        case cHandleBM:
            ox -= width / 2;
            oy -= height;
            break;

        case cHandleBR:
            ox -= width;
            oy -= height;
            break;
    }

    *outX = ox;
    *outY = oy;
}


int ooldx, ooldy;
void MainWindow::ProcessClickPaint(int sx, int sy, unsigned char flags){
    int nsx, nsy;
    nsx = sx;
    nsy = sy;

    uint8_t oldSelectedFPenColour = numSelectedPaletteID;
    uint8_t oldSelectedBPenColour = numSelectedBackPaletteID;

    //int cyclePaletteID = 0; // used for when drawing the index + numSelectedPaletteID
    if(ui->chkCyclePaletteDraw->isChecked()){
        if((ooldx != sx) || (ooldy != sy)){
            ooldx = sx;
            ooldy = sy;
            //chkCyclePaletteDrawStepping++;
            cyclePaletteStepping++;
            if(cyclePaletteStepping > ui->scrCycleStepper->value()){
                cyclePaletteStepping = 0;
            }
            if(cyclePaletteStepping == 1){
                cyclePaletteID ++;
                if(cyclePaletteID >= cyclelength)
                    cyclePaletteID = 0;
            }
        };
        numSelectedPaletteID = cyclefrom + cyclePaletteID;
    }

    switch(currentDrawMode){
        case Plot: {    // this will only work with Motion Drawing (plot)
            if(flags & DrawUIMode_LeftMouseButton) // only when left mouse is down
                setIconArea(sx, sy);
            else    // otherwise Right mouse button?
                clrIconArea(sx, sy);
        } break;

        case Pen: {    // this will only work with Motion Drawing (plot)
            if(flags & DrawUIMode_LeftMouseButton) // only when left mouse is down
                setIconAreaPen(sx, sy, iPenShapeSize, true);
            else    // otherwise Right mouse button?
                setIconAreaPen(sx, sy, iPenShapeSize, false);
        } break;

        case FloodFill: {
            if((flags & DrawUIMode_InitButton) && (flags & DrawUIMode_LeftMouseButton))   // only allow this to work on the Initial Mouse Hit
                floodFill(sx, sy, numSelectedPaletteID);
            if((flags & DrawUIMode_InitButton) && (flags & DrawUIMode_RightMouseButton))   // only allow this to work on the Initial Mouse Hit
                floodFill(sx, sy, numSelectedBackPaletteID);
                //floodFillGradient(sx, sy, 0, 255, 45);
        } break;

        // this will trigger on the click down! so commenting out
        /*
        case FloodFillGradient:
            if((flags & DrawUIMode_InitButton) && (flags & DrawUIMode_LeftMouseButton))   // only allow this to work on the Initial Mouse Hit
                //floodFill(sx, sy, numSelectedPaletteID);
                floodFillGradient(sx, sy, 0, 255, gradAngle);
            //if((flags & DrawUIMode_InitButton) && (flags & DrawUIMode_RightMouseButton))   // only allow this to work on the Initial Mouse Hit
                //floodFill(sx, sy, numSelectedBackPaletteID);
                //floodFillGradient(sx, sy, 0, 255, 45);
            break;
        */

        case DrawText: {
            getTextCenterHandle(sx, sy, &nsx, &nsy);    // source x, source y, return results x, return results y
            if(flags & DrawUIMode_InitButton){   // only allow this to work on the Initial Mouse Hit
                if(flags & DrawUIMode_LeftMouseButton)
                    drawText(nsx, nsy, 1);
                else
                    drawText(nsx, nsy, 0);
            }
        } break;

        case SprayCan:{ // some weird condition logic here
            if(flags & DrawUIMode_LeftMouseButton){
                iSpraySX = sx;
                iSpraySY = sy;
                bSprayDraw = 1; // is drawing, NOT erassing (assing!??)
                if(bSprayingTheCan == false){
                    bSprayingTheCan=true;
                    tmrSprayCanTimer->stop();   // just incase it didnt stop
                    tmrSprayCanTimer->setInterval(22); // for a roughly 60hz update
                    tmrSprayCanTimer->start();
                    //printf("new spray rate: %lu\n", 1000 - (iSprayRate * 10));
                }
            } // the handling of the stop spray can, is in the mouse release event in the Editor Window  
        } break;

        case PasteBrush:{
            getCenterHandle(sx, sy, &nsx, &nsy, iCopyWidth, iCopyHeight);    // source x, source y, return results x, return results y
            if(flags & DrawUIMode_InitButton){   // only allow this to work on the Initial Mouse Hit
                PlaceBrush(nsx, nsy);
                /*
                if(flags & DrawUIMode_LeftMouseButton)
                    drawText(nsx, nsy, 1);
                else
                    drawText(nsx, nsy, 0);
                */
            }
        } break;

        default:
            return;
    }


    numSelectedPaletteID = oldSelectedFPenColour;
    numSelectedBackPaletteID = oldSelectedBPenColour;

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


struct FillNode {
    int x, y;
    int dist;
};

void MainWindow::floodFillGradient(int startX, int startY, uint8_t colStart, uint8_t colEnd, int length, float angleDegrees){
    if(startX < 0 || startX >= icon_width || startY < 0 || startY >= icon_height)
        return;

    uint8_t targetColor = (*icon_area)[startY][startX];
    if (targetColor == colStart && colStart == colEnd)
        return;

    bMirroredGradient = ui->chkFloodFillMirrored->isChecked();
    bReversedGradient = ui->chkFloodFillReversed->isChecked();



    // Gradient direction
    float rad = angleDegrees * (M_PI / 180.0f);
    float dirX = cosf(rad);
    float dirY = sinf(rad);


    int tick = 0;

    std::vector<FillNode> open;
    std::vector<FillNode> filled;

    open.push_back({ startX, startY, 0 });
    size_t head = 0;

    const uint8_t VISITED = 0xFF;
    int maxDist = 0;


    while(head < open.size()){
        FillNode n = open[head++];
        int x = n.x;
        int y = n.y;

        if(x < 0 || x >= icon_width || y < 0 || y >= icon_height)
            continue;

        if((*icon_area)[y][x] != targetColor)
            continue;

        (*icon_area)[y][x] = VISITED;
        filled.push_back(n);

        maxDist = std::max(maxDist, n.dist);

        open.push_back({ x+1, y, n.dist+1 });
        open.push_back({ x-1, y, n.dist+1 });
        open.push_back({ x, y+1, n.dist+1 });
        open.push_back({ x, y-1, n.dist+1 });
    }

    if(filled.empty()){
        renderEditorCanvas();
        return;
    }

    // ---- FIND GRADIENT RANGE ----
    float range = float(gradLength);
    if (range <= 0.0f)
        range = 1.0f;

    // Optional Bayer matrix for dithering
    const int ditherMatrix[4][4] = {
        {0,  8,  2, 10},
        {12, 4, 14, 6},
        {3, 11, 1,  9},
        {15, 7, 13, 5}
    };

    // ---- APPLY GRADIENT ----
    //for(const QPoint &p : filled){
    // ---- APPLY GRADIENT ----
    for(const FillNode &n : filled){
        int x = n.x;
        int y = n.y;

        float proj = (x - startX) * dirX + (y - startY) * dirY;

        float dx = x - startX;
        float dy = y - startY;
        float dist = std::sqrt(dx*dx + dy*dy);

        float t;

        switch(iFillType){
        case FillLinear:
            t = proj / range;   // Linear
            break;
        case FillDiamond:
            t = float(n.dist) / range;  // Diamond
            break;
        case FillCircles:
            t = dist / range;   // Circles
            break;
        default:
            t = proj / range;   // Linear
        }

        t = std::clamp(t, 0.0f, 1.0f);

        // --- MIRRORED GRADIENT ---
        if(bMirroredGradient){
            if(t <= 0.5f)
                t = t * 2.0f;          // first half: 0 → 1
            else
                t = (1.0f - t) * 2.0f; // second half: 1 → 0
        }
        // --- REVERSED GRADIENT ---
        if(bReversedGradient){
            t = 1.0f - t;
        }

        float rawColor = colStart + t * (colEnd - colStart); // keep as float
        uint8_t finalCol = uint8_t(std::round(rawColor));

        // ... existing dithering code ...
        if(bDithered){
            if(bNoisyDither){
                float frac = rawColor - std::floor(rawColor);
                float noise = (rand() % 1000) / 1000.0f;
                finalCol = (frac > noise) ? uint8_t(std::ceil(rawColor)) : uint8_t(std::floor(rawColor));
            } else {
                int mx = x % 4;
                int my = y % 4;
                float threshold = (ditherMatrix[my][mx] + 0.5f) / 16.0f;
                float frac = rawColor - std::floor(rawColor);
                finalCol = (frac > threshold) ? uint8_t(std::ceil(rawColor)) : uint8_t(std::floor(rawColor));
            }
        }

        // --- redraw step ---
        if(!ui->chkInstaFill->isChecked()){
            tick++;
            if(tick > 250){
                renderEditorCanvas();
                QCoreApplication::processEvents();
                QThread::msleep(3);
                tick = 0;
            }
        }

        finalCol = std::clamp(finalCol, colStart, colEnd);

        (*icon_area)[y][x] = finalCol;
    }

    renderEditorCanvas();
}



void MainWindow::floodFill(int startX, int startY, uint8_t fillColor){
    int tick = 0;
    if(startX < 0 || startX >= icon_width || startY < 0 || startY >= icon_height)
        return;

    uint8_t targetColor = (*icon_area)[startY][startX];
    if (targetColor == fillColor) return;

    std::stack<QPoint> s;
    s.push(QPoint(startX, startY));

    // visited mask
    std::vector<std::vector<bool>> visited(icon_height, std::vector<bool>(icon_width, false));

    while(!s.empty()){
        QPoint p = s.top(); s.pop();
        int x = p.x();
        int y = p.y();

        if(x < 0 || x >= icon_width || y < 0 || y >= icon_height) continue;
        if(visited[y][x]) continue;
        if((*icon_area)[y][x] != targetColor) continue;

        visited[y][x] = true; // mark as visited

        // --- fill with brush or solid ---
        if(ui->chkFloodFillBrush->isChecked() && !icon_copy_area.empty()){
            int brushX = x % iCopyWidth;
            int brushY = y % iCopyHeight;
            (*icon_area)[y][x] = icon_copy_area[brushY][brushX];
        } else {
            (*icon_area)[y][x] = fillColor;
        }

        // --- redraw step ---
        if(!ui->chkInstaFill->isChecked()){
            tick++;
            if(tick > 250){
                renderEditorCanvas();
                QCoreApplication::processEvents();
                QThread::msleep(3);
                tick = 0;
            }
        }

        // push neighbors
        s.push(QPoint(x, y+1));
        s.push(QPoint(x, y-1));
        s.push(QPoint(x+1, y));
        s.push(QPoint(x-1, y));
    }
    renderEditorCanvas();
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

    QWidget *frmGFXedit = ui->frmGraphicEdit->parentWidget();
    QWidget *wincontainer = ui->verticalLayoutWidget;
    QWidget *animator = ui->frmCellAnimator;
    QWidget *container = ui->frmGraphicEdit;
    QWidget *vboxh = ui->vboxTextoutputv;
    QWidget *fonteditBox = ui->frmFontWorkbench;
    QWidget *sysoptions = ui->frmOptions;

    //WinXW = wincontainer->width() - 2;
    //WinXH = wincontainer->height() - 28;
    WinXW = frmGFXedit->width() + 6;
    WinXH = frmGFXedit->height() - 14;


    if(container){
        container->resize(WinXW - 8, WinXH - 8);  // or any size you want
        wincontainer->resize(WinXW - 18, WinXH - 18);
    }
    if(vboxh){
        ui->outputTextView->resize(WinXW, WinXH);
        vboxh->resize(WinXW-16, WinXH-8);
    }
    if(fonteditBox){
        //ui->frmFontWorkbench->resize((WinXW))
        fonteditBox->resize(WinXW, WinXH);
    }
    if(sysoptions){
        //ui->frmFontWorkbench->resize((WinXW))
        sysoptions->resize(WinXW, WinXH);
    }
    if(animator){
        animator->resize(WinXW, WinXH);
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

void MainWindow::renderAnimatorCanvas(){
    int CellID = currentCellID; // <-- whatever cell you're previewing

    int cellsPerRow = icon_width / cell_width;
    int cellsPerColumn = icon_height / cell_height;
    int maximumCells = cellsPerRow * cellsPerColumn;
    if (cellsPerRow <= 0) return;

    int cellX = CellID % cellsPerRow;
    int cellY = CellID / cellsPerRow;

    int srcX = cellX * cell_width;
    int srcY = cellY * cell_height;

    // Safety check
    if (srcX + cell_width > icon_width ||
        srcY + cell_height > icon_height)
        return;

    ui->scrShowCellID->setMaximum(maximumCells - 1);
    ui->lblShowCellID->setText(QString("%1").arg(CellID));

    ui->lblCellSize->setText(QString("%1 x %2")
                                .arg(cell_width)
                                .arg(cell_height));

    // Create image at cell size
    animationImg = QImage(cell_width, cell_height, QImage::Format_RGB32);

    for(int y = 0; y < cell_height; y++) {
        QRgb *scan = reinterpret_cast<QRgb*>(animationImg.scanLine(y));
        for(int x = 0; x < cell_width; x++) {
            uint8_t pal = (*icon_area)[srcY + y][srcX + x];
            scan[x] = colourSqueeze(CCLUT[pal]);
        }
    }

    // Remove old pixmap
    if(animationPixmap){
        animationScene->removeItem(animationPixmap);
        delete animationPixmap;
    }

    // Add pixmap
    animationPixmap = animationScene->addPixmap(QPixmap::fromImage(animationImg));

    // Fixed preview area
    animationScene->setSceneRect(0, 0, 320, 320);

    // Scale to fit preview
    double scale = qMin(320.0 / cell_width, 320.0 / cell_height);
    animationPixmap->setScale(scale);

    // Center it
    animationPixmap->setPos(
        (320 - cell_width  * scale) / 2,
        (320 - cell_height * scale) / 2
        );
}




void MainWindow::renderEditorCanvas(){
    int visibleWidth  = editorViewPortWidth * icon_zoom;   // in pixels
    int visibleHeight = editorViewPortHeight * icon_zoom;

    int xOffset = ui->scrEditorH->value();
    int yOffset = ui->scrEditorV->value();

    editorImg = QImage(visibleWidth, visibleHeight, QImage::Format_RGB32);

    QRgb gridColor = gridEnabled ? QColor(gridRed, gridGreen, gridBlue).rgb() : 0; // choose color
    bool drawGrid = gridEnabled;
    bool nonInteger = false;

    if((icon_width % cell_width)) nonInteger=true;
    if((icon_height % cell_height)) nonInteger=true;

    if(nonInteger)
        ui->lblCellSizeWarning->show();
    else
        ui->lblCellSizeWarning->hide();

    for(int y = 0; y < visibleHeight; y++) {
        QRgb *scan = reinterpret_cast<QRgb*>(editorImg.scanLine(y));
        int imgY = (y) / icon_zoom;
        for(int x = 0; x < visibleWidth; x++) {
            int imgX = (x) / icon_zoom;
            QRgb base;
            //QRgb base = colourSqueeze(CCLUT[(*icon_area)[imgY + yOffset][imgX + xOffset]]);

            if(icon_area == &icon_area_scratchpage){
                base = colourSqueeze(CCLUT[(*icon_area)[imgY + yOffset][imgX + xOffset]]);
                scan[x] = base; // simple scratch pad area
            } else {

                int cindexf = (icon_area_front)[imgY + yOffset][imgX + xOffset];
                int cindexb = (icon_area_back) [imgY + yOffset][imgX + xOffset];

                // rendering front, unless its transparent
                if(cindexf == 0)    // front is transparent pixel
                    base = colourSqueeze(CLUTB[cindexb]);
                else
                    base = colourSqueeze(CLUTF[cindexf]);

                scan[x] = base; // simple scratch pad area
            }

            // -------- GRID --------
            if(drawGrid) {
                if(icon_zoom>7){
                    if ((x % icon_zoom == 0 && x != 0) ||
                        (y % icon_zoom == 0 && y != 0)){
                        scan[x] = gridColor;
                    }
                }
            }
        }
    }

    if(!grabbing){
        // ---------------- DRAW HOVER BOX OVER THE TOP ---------------- //
        if((ui->scrEditorZoomVal->value() > 3) || (currentDrawMode == Circle) ){
            if (hoverPixelX >= 0 && hoverPixelY >= 0){
                int px = hoverPixelX * icon_zoom;
                int py = hoverPixelY * icon_zoom;

                int inner = 1;             // offset from gridline
                int thick = 2;             // border thickness

                int left   = px + inner;
                int right  = px + icon_zoom - inner ;
                int top    = py + inner;
                int bottom = py + icon_zoom - inner ;

                if((currentDrawMode == Circle) && !(bMouseButtonDown)){
                    // draw a cross hair (ruler like)
                    // Clamp to viewport (for safety)
                    int cx = px;
                    int cy = py;

                    int thickPx = icon_zoom - 1;

                    auto invert = [&](int xx, int yy){
                        QRgb *scan = reinterpret_cast<QRgb*>(editorImg.scanLine(yy));
                        scan[xx] = 0xFFFFFFFF - scan[xx];
                    };

                    // Vertical ruler (aligned to icon pixel column)
                    if (cx >= 0 && cx + thickPx < visibleWidth) {
                        for (int t = 0; t <= thickPx; t++) {
                            int xx = cx + t;
                            for (int y = 0; y < visibleHeight; y++) {
                                invert(xx, y);
                            }
                        }
                    }

                    // Horizontal ruler (aligned to icon pixel row)
                    if (cy >= 0 && cy + thickPx < visibleHeight) {
                        for (int t = 0; t <= thickPx; t++) {
                            int yy = cy + t;
                            for (int x = 0; x < visibleWidth; x++) {
                                invert(x, yy);
                            }
                        }
                    }
                } else {
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
        }

        if(captureXYStart==true){
            int halfPen = iPenShapeSize / 2;
            //ltcapturedX = ctcapturedX;
            //ltcapturedY = ctcapturedY;
            //readToolXY(&ctcapturedX, &ctcapturedY); // process where the capturey point is
            //printf("P2P Tool: Target point TX: %d, TY: %d\n", ctcapturedX, ctcapturedY);
            if(currentDrawMode == FloodFillGradient){

            }
            if(currentDrawMode == Line || currentDrawMode == FloodFillGradient){
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
                            if(currentDrawMode == Line)
                                scan[px] = CCLUT[bMouseLeftRight?numSelectedPaletteID:numSelectedBackPaletteID];
                            else
                                scan[px] = 0xFFFFFFFF - scan[px]; // invert
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

                    // Draw a block around (x, y) for pen thickness
                    if(currentDrawMode == FloodFillGradient){
                        invertCell(x, y);
                    } else {
                        for(int pxOff = -halfPen; pxOff <= halfPen; pxOff++){
                            for(int pyOff = -halfPen; pyOff <= halfPen; pyOff++){
                                invertCell(x + pxOff, y + pyOff);
                            }
                        }
                    }
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
                            scan[px] = CCLUT[bMouseLeftRight?numSelectedPaletteID:numSelectedBackPaletteID];
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
                int drawHeight = std::min(iPenShapeSize, bottom - top + 1);
                int drawWidth  = std::min(iPenShapeSize, right - left + 1);

                // Top and bottom edges
                for(int x = left; x <= right; x++){
                    for(int py = 0; py < drawHeight; py++){
                        invertCell(x, top + py);
                        invertCell(x, bottom - py);
                    }
                }

                // Left and right edges
                for(int y = top; y <= bottom; y++){
                    for(int px = 0; px < drawWidth; px++){
                        invertCell(left + px, y);
                        invertCell(right - px, y);
                    }
                }

                if(bFillToolIn){
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
                            scan[px] = CCLUT[bMouseLeftRight ? numSelectedPaletteID : numSelectedBackPaletteID];
                        }
                    }
                };

                int cx = capturedX;
                int cy = capturedY;
                int radius = std::max(abs(hoverPixelX - cx), abs(hoverPixelY - cy));
                int halfPen = iPenShapeSize / 2;

                int outerRadius = radius + halfPen;
                int innerRadius = std::max(0, radius - halfPen);

                for(int y = cy - outerRadius; y <= cy + outerRadius; y++){
                    for(int x = cx - outerRadius; x <= cx + outerRadius; x++){
                        int dx = x - cx;
                        int dy = y - cy;
                        float dist = std::sqrt(dx*dx + dy*dy);

                        float epsilon = 0.5f; // small tolerance
                        if(dist >= innerRadius - epsilon && dist <= outerRadius + epsilon){
                            // Outline
                            invertCell(x, y);
                        } else if(bFillToolIn && dist < innerRadius){
                            // Fill inside
                            invertCell(x, y);
                        }
                    }
                }
            }

        } else {
            if(currentDrawMode == DrawText){
                //int startX = cellX * icon_zoom;
                //int startY = cellY * icon_zoom;
                int x1cell = hoverPixelX;// * icon_zoom;
                int y1cell = hoverPixelY;// * icon_zoom;
                int nsx, nsy;

                getTextCenterHandle(x1cell, y1cell, &nsx, &nsy);

                nsx *= icon_zoom;
                nsy *= icon_zoom;

                //drawTextHover(x1cell, y1cell, &editorImg);
                drawTextHover(nsx, nsy, &editorImg);
            }
            if(currentDrawMode == Pen){
                int x1cell = hoverPixelX;// * icon_zoom;
                int y1cell = hoverPixelY;// * icon_zoom;
                drawIconAreaPenHover(x1cell, y1cell, iPenShapeSize, &editorImg, true);
            }
            if(currentDrawMode == SprayCan){
                int x1cell = hoverPixelX;// * icon_zoom;
                int y1cell = hoverPixelY;// * icon_zoom;
                drawIconAreaPenHover(x1cell, y1cell, iPenShapeSize * 2, &editorImg, false);
            }
            //if(currentDrawMode == CopyBrush)
        }
    }

    if(bGrabbedCopyStart){
        int xOffset = ui->scrEditorH->value();
        int yOffset = ui->scrEditorV->value();
        int x1cell = iTargetCopyX - xOffset;
        int y1cell = iTargetCopyY - yOffset;
        // this is just the area around the graphics box!
        drawHoverSelectionBox(x1cell, y1cell, iCopyWidth, iCopyHeight, &editorImg);
    }


    if(currentDrawMode == PasteBrush){
        //printf("COX... ");
        int xOffset = ui->scrEditorH->value();
        int yOffset = ui->scrEditorV->value();
        //int x1cell = iTargetCopyX - xOffset;
        //int y1cell = iTargetCopyY - yOffset;

        int x1cell = hoverPixelX;// * icon_zoom;
        int y1cell = hoverPixelY;// * icon_zoom;

        int nsx, nsy;


        getCenterHandle(x1cell, y1cell, &nsx, &nsy, iCopyWidth, iCopyHeight);    // source x, source y, return results x, return results y
        drawCopyBrushHover(nsx, nsy, &editorImg);
        //renderEditorCanvas(); // redraw empty icon
    }

    if(ui->chkCellDivider->isChecked()){    // if cell divide is on draw the grid at all times !
        int cellW = ui->txtCellWidth->text().toInt() * icon_zoom;
        int cellH = ui->txtCellHeight->text().toInt() * icon_zoom;

        int xOffset = ui->scrEditorH->value() * icon_zoom;
        int yOffset = ui->scrEditorV->value() * icon_zoom;
        int colourToggle1 = 0xffffff;
        int colourToggle2 = 0xffffff;

        for (int y = 0; y < visibleHeight; y++) {
            QRgb *scan = reinterpret_cast<QRgb*>(editorImg.scanLine(y));

            for (int x = 0; x < visibleWidth; x++) {

                bool onVGrid = ((x + xOffset) % cellW == 0 && x != 0);
                bool onHGrid = ((y + yOffset) % cellH == 0 && y != 0);

                if (onVGrid) {
                    // vertical line → dot by Y
                    if (((y + yOffset) & 3) == 0) {
                        //scan[x] = colourToggle1;//~scan[x];
                        //colourToggle1 = 0xffffff - colourToggle1;

                        int phase = ((y + yOffset) >> 2) & 1;
                        scan[x] = phase ? 0xFFFFFF : 0x000000;

                    }
                }
                else if (onHGrid) {
                    // horizontal line → dot by X
                    if (((x + xOffset) & 3) == 0) {
                        //scan[x] = ~scan[x];
                        //scan[x] = colourToggle2;//~scan[x];
                        //colourToggle2 = 0xffffff - colourToggle2;
                        int phase = ((x + xOffset) >> 2) & 1;
                        scan[x] = phase ? 0xFFFFFF : 0x000000;

                    }
                }
            }
        }

    }


    editorPixmap->setPixmap(QPixmap::fromImage(editorImg));
}

//int cyclefrom, cycleto, cyclelength = 8;
void MainWindow::renderPaletteCanvas(){
    const int totalWidth  = PALETTE_WIDTH  * PALETTE_BOX_HSIZE;  // 16 * 16 = 256
    const int totalHeight = PALETTE_HEIGHT * PALETTE_BOX_VSIZE;  // 16 * 16 = 256

    //const int SelectedX = 1;    // this will be clickable later
    //const int SelectedY = 2;

    const int GridX = SelectedX * PALETTE_BOX_HSIZE;
    const int GridY = SelectedY * PALETTE_BOX_VSIZE;

    for(int i = 0; i < 256; i++){
        CLUTRGB[0][i] = (CCLUT[i] >>16) & 0xff;
        CLUTRGB[1][i] = (CCLUT[i] >>8) & 0xff;
        CLUTRGB[2][i] = (CCLUT[i] ) & 0xff;
    }

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
            scan[x] = colourSqueeze(CCLUT[colorIndex]);
        }
    }

    const int thickness = 2;
    // expand loop to cover surrounding border
    for (int dy = -thickness; dy < PALETTE_BOX_VSIZE + thickness; dy++) {
        for (int dx = -thickness; dx < PALETTE_BOX_HSIZE + thickness; dx++) {
            bool onBorder =
                dx < 0 || dx >= PALETTE_BOX_HSIZE ||   // left/right outside
                dy < 0 || dy >= PALETTE_BOX_VSIZE;     // top/bottom outside

            if (onBorder) {
                int px = GridX + dx;
                int py = GridY + dy;

                // clamp to image bounds
                if (px < 0 || px >= totalWidth || py < 0 || py >= totalHeight)
                    continue;

                QRgb *scan = reinterpret_cast<QRgb*>(paletteImg.scanLine(py));
                scan[px] = ~scan[px];   // invert the pixel
            }
        }
    }

    // cycle palette selector outlining
    auto isSelected = [&](int index) {
        int a = cyclefrom;
        int b = cycleto;
        if (a > b) std::swap(a, b);
        return index >= a && index <= b;
    };

    for (int idx = cyclefrom; idx <= cycleto; idx++) {

        int tileX = idx % PALETTE_WIDTH;
        int tileY = idx / PALETTE_WIDTH;

        int px = tileX * PALETTE_BOX_HSIZE;
        int py = tileY * PALETTE_BOX_VSIZE;

        // Neighbour tests
        bool left   = (tileX > 0) && isSelected(idx - 1);
        bool right  = (tileX < PALETTE_WIDTH - 1) && isSelected(idx + 1);
        bool top    = (tileY > 0) && isSelected(idx - PALETTE_WIDTH);
        bool bottom = (tileY < PALETTE_HEIGHT - 1) && isSelected(idx + PALETTE_WIDTH);

        for (int t = 0; t < thickness; t++) {
            // LEFT edge
            if (!left) {
                for (int y = 0; y < PALETTE_BOX_VSIZE; y++) {
                    QRgb *scan = reinterpret_cast<QRgb*>(
                        paletteImg.scanLine(py + y));
                    scan[px + t] = qRgb(0,0,0);
                }
            }

            // RIGHT edge
            if (!right) {
                for (int y = 0; y < PALETTE_BOX_VSIZE; y++) {
                    QRgb *scan = reinterpret_cast<QRgb*>(
                        paletteImg.scanLine(py + y));
                    scan[px + PALETTE_BOX_HSIZE - 1 - t] = qRgb(0,0,0);
                }
            }

            // TOP edge
            if (!top) {
                QRgb *scan = reinterpret_cast<QRgb*>(
                    paletteImg.scanLine(py + t));
                for (int x = 0; x < PALETTE_BOX_HSIZE; x++)
                    scan[px + x] = qRgb(0,0,0);
            }

            // BOTTOM edge
            if (!bottom) {
                QRgb *scan = reinterpret_cast<QRgb*>(
                    paletteImg.scanLine(py + PALETTE_BOX_VSIZE - 1 - t));
                for (int x = 0; x < PALETTE_BOX_HSIZE; x++)
                    scan[px + x] = qRgb(0,0,0);
            }
        }
    }

    // for the gradient select
    //int  GradientRangeFrom = 80;
    //int  GradientRangeTo = 87;
    auto isGradientSelected = [&](int index) {
        int a = GradientRangeFrom;
        int b = GradientRangeTo;
        if (a > b) std::swap(a, b);
        return index >= a && index <= b;
    };

    for (int idx = GradientRangeFrom; idx <= GradientRangeTo; idx++) {

        int tileX = idx % PALETTE_WIDTH;
        int tileY = idx / PALETTE_WIDTH;

        int px = tileX * PALETTE_BOX_HSIZE;
        int py = tileY * PALETTE_BOX_VSIZE;

        bool left   = (tileX > 0) && isGradientSelected(idx - 1);
        bool right  = (tileX < PALETTE_WIDTH - 1) && isGradientSelected(idx + 1);
        bool top    = (tileY > 0) && isGradientSelected(idx - PALETTE_WIDTH);
        bool bottom = (tileY < PALETTE_HEIGHT - 1) && isGradientSelected(idx + PALETTE_WIDTH);

        for (int t = 0; t < thickness; t++) {
            // LEFT edge
            if (!left) {
                for (int y = 0; y < PALETTE_BOX_VSIZE; y++) {
                    QRgb *scan = reinterpret_cast<QRgb*>(paletteImg.scanLine(py + y));
                    scan[px + t] = qRgb(255,255,255);
                }
            }

            // RIGHT edge
            if (!right) {
                for (int y = 0; y < PALETTE_BOX_VSIZE; y++) {
                    QRgb *scan = reinterpret_cast<QRgb*>(paletteImg.scanLine(py + y));
                    scan[px + PALETTE_BOX_HSIZE - 1 - t] = qRgb(255,255,255);
                }
            }

            // TOP edge
            if (!top) {
                QRgb *scan = reinterpret_cast<QRgb*>(paletteImg.scanLine(py + t));
                for (int x = 0; x < PALETTE_BOX_HSIZE; x++)
                    scan[px + x] = qRgb(255,255,255);
            }

            // BOTTOM edge
            if (!bottom) {
                QRgb *scan = reinterpret_cast<QRgb*>(paletteImg.scanLine(py + PALETTE_BOX_VSIZE - 1 - t));
                for (int x = 0; x < PALETTE_BOX_HSIZE; x++)
                    scan[px + x] = qRgb(255,255,255);
            }
        }
    }

    palettePixmap->setPixmap(QPixmap::fromImage(paletteImg));
}



void MainWindow::CopySelectionToBrush(){
    // hit this with a one hit adjust
    iCopyWidth ++;
    iCopyHeight ++;
    if (iCopyWidth <= 0 || iCopyHeight <= 0)
        return;

    icon_copy_area.resize(iCopyHeight);
    for (auto &row : icon_copy_area)
        row.resize(iCopyWidth, 0);

    for (int y = 0; y < iCopyHeight; y++){
        int srcY = iTargetCopyY + y;
        if (srcY < 0 || srcY >= icon_height) continue;

        for (int x = 0; x < iCopyWidth; x++){
            int srcX = iTargetCopyX + x;
            if (srcX < 0 || srcX >= icon_width) continue;
            icon_copy_area[y][x] = (*icon_area)[srcY][srcX];
        }
    }
}

void MainWindow::doAlterBrush(int mode){
    switch(mode){
        case brushflipX: {
            // Flip horizontally
            for (int y = 0; y < iCopyHeight; y++) {
                for (int x = 0; x < iCopyWidth / 2; x++) {
                    std::swap(icon_copy_area[y][x], icon_copy_area[y][iCopyWidth - 1 - x]);
                }
            }
        } break;

        case brushflipY: {
            // Flip vertically
            for (int y = 0; y < iCopyHeight / 2; y++) {
                std::swap(icon_copy_area[y], icon_copy_area[iCopyHeight - 1 - y]);
            }
        } break;

        case brushrotateR: {
            // Rotate 90° clockwise
            std::vector<std::vector<uint8_t>> newArea(iCopyWidth, std::vector<uint8_t>(iCopyHeight));
            for (int y = 0; y < iCopyHeight; y++) {
                for (int x = 0; x < iCopyWidth; x++) {
                    newArea[x][iCopyHeight - 1 - y] = icon_copy_area[y][x];
                }
            }
            std::swap(iCopyWidth, iCopyHeight);
            icon_copy_area = std::move(newArea);
        } break;

        case brushrotateL: {
            // Rotate 90° counter-clockwise
            std::vector<std::vector<uint8_t>> newArea(iCopyWidth, std::vector<uint8_t>(iCopyHeight));
            for (int y = 0; y < iCopyHeight; y++) {
                for (int x = 0; x < iCopyWidth; x++) {
                    newArea[iCopyWidth - 1 - x][y] = icon_copy_area[y][x];
                }
            }
            std::swap(iCopyWidth, iCopyHeight);
            icon_copy_area = std::move(newArea);
        } break;
    }
}


/// clip board sys

//#include <QClipboard>
//#include <QGuiApplication>
//#include <QMimeData>

void MainWindow::pasteClipboardAsBrush(){
    QClipboard *clipboard = QGuiApplication::clipboard();
    const QMimeData *mime = clipboard->mimeData();

    if(!mime->hasImage()){
        QMessageBox::warning(this, "Paste Brush", "Clipboard does not contain an image.");
        return;
    }

    QImage img = qvariant_cast<QImage>(mime->imageData());
    if(img.isNull()){
        QMessageBox::warning(this, "Paste Brush", "Failed to read clipboard image.");
        return;
    }

    // Convert to a sane format
    img = img.convertToFormat(QImage::Format_ARGB32);

    // From here → convert to icon_copy_area
    convertImageToBrush(img);
}

void MainWindow::convertImageToBrush(const QImage &img){
    iCopyWidth  = img.width();
    iCopyHeight = img.height();

    icon_copy_area.assign(iCopyHeight, std::vector<uint8_t>(iCopyWidth, 0));

    for(int y = 0; y < iCopyHeight; y++){
        const QRgb *scan = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        for(int x = 0; x < iCopyWidth; x++){
            QRgb px = scan[x];

            if(qAlpha(px) < 32){
                icon_copy_area[y][x] = 0; // transparent
                continue;
            }

            icon_copy_area[y][x] = findNearestPaletteColor(px);
        }
    }
}

uint8_t MainWindow::findNearestPaletteColor(QRgb rgb){
    int r = qRed(rgb);
    int g = qGreen(rgb);
    int b = qBlue(rgb);

    int best = 0;
    int bestDist = INT_MAX;

    for(int i = 0; i < 256; i++){
        QRgb p = CCLUT[i];
        int dr = r - qRed(p);
        int dg = g - qGreen(p);
        int db = b - qBlue(p);
        int dist = dr*dr + dg*dg + db*db;

        if(dist < bestDist){
            bestDist = dist;
            best = i;
        }
    }

    return uint8_t(best);
}

inline void unpackRGB(uint32_t c, int &r, int &g, int &b){
    r = (c >> 16) & 0xFF;
    g = (c >> 8)  & 0xFF;
    b = (c)       & 0xFF;
}

uint8_t MainWindow::findNearestPaletteIndex(int r, int g, int b){
    int best = 0;
    int bestDist = INT_MAX;

    for(int i = 0; i < 256; i++){
        int pr, pg, pb;
        unpackRGB(CCLUT[i], pr, pg, pb);

        int dr = pr - r;
        int dg = pg - g;
        int db = pb - b;
        int dist = dr*dr + dg*dg + db*db;

        if(dist < bestDist){
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

void MainWindow::setAAPixel(int x, int y, uint8_t fg){
    if(x < 0 || x >= icon_width || y < 0 || y >= icon_height)
        return;

    uint8_t bg = (*icon_area)[y][x];
    if(bg == fg) return;

    int fr, fg_, fb;
    int br, bg_, bb;

    unpackRGB(CCLUT[fg], fr, fg_, fb);
    unpackRGB(CCLUT[bg], br, bg_, bb);

    // 50/50 blend (classic AA)
    int r = (fr + br) >> 1;
    int g = (fg_ + bg_) >> 1;
    int b = (fb + bb) >> 1;

    uint8_t aa = findNearestPaletteIndex(r, g, b);
    (*icon_area)[y][x] = aa;
}


bool MainWindow::clipboardHasImage() {
    const QClipboard* cb = QGuiApplication::clipboard();
    const QMimeData* md = cb->mimeData();
    return md->hasImage();
}

