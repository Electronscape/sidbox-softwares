#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QIcon>
#include <QMouseEvent>
#include <QPushButton>
#include <QTimer>
#include <functional>
#include <cstring>
#include <string.h>

#define PALETTE_BOX_HSIZE   16
#define PALETTE_BOX_VSIZE   22
#define PALETTE_WIDTH       PALETTE_BOX_HSIZE
#define PALETTE_HEIGHT      PALETTE_BOX_VSIZE

#define PALETTE_VRAM_SIZE   (PALETTE_WIDTH * PALETTE_BOX_HSIZE * PALETTE_HEIGHT * PALETTE_BOX_VSIZE)
int SelectedX = 0;    // this will be clickable later
int SelectedY = 0;
uint8_t     numSelectedPaletteID = 0, numPrevSelectedPaletteID = 0;
uint8_t     pltColourPreset[3] = {0,0,0};

QPalette pal;

int hoverPixelX = -1;
int hoverPixelY = -1;


#define gridEnabled      true
#define gridRed          255
#define gridGreen        255
#define gridBlue         255

uint16_t icon_zoom       = 16;
uint16_t icon_width      = 32;
uint16_t icon_height     = 32;

int editorViewPortWidth  = 8;   // editor width grid
int editorViewPortHeight = 8;

//uint8_t icon_area[8][8] = {0};  // all to paletteID 0
std::vector<std::vector<uint8_t>> icon_area;


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


// this keeps the original colour so can revert the selected palette,
// commiting the change, simply just click on another colour item
uint32_t BACKUP_CLUT[256];

uint8_t palleteCanvas[PALETTE_VRAM_SIZE];

QTimer *updateTimer;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);




    scene = new QGraphicsScene(this);
    ui->gfxPalleteSelect->setScene(scene);

    // Your framebuffer image (480×320)
    paletteImg = QImage(PALETTE_WIDTH * PALETTE_BOX_HSIZE, PALETTE_HEIGHT * PALETTE_BOX_VSIZE, QImage::Format_RGB32);

    ui->gfxPalleteSelect->setRenderHint(QPainter::SmoothPixmapTransform, true);
    ui->gfxPalleteSelect->setRenderHint(QPainter::Antialiasing, false); // don't need antialias for pixels
    ui->gfxPalleteSelect->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->gfxPalleteSelect->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->gfxPalleteSelect->setMouseTracking(true);

    ui->gfxPalleteSelect->viewport()->installEventFilter(this);

    // Add it to the scene
    palettePixmap = scene->addPixmap(QPixmap::fromImage(paletteImg));
    palettePixmap->setTransformationMode(Qt::SmoothTransformation);
    renderPaletteCanvas();


    ///---------------- EDITOR GRAPHICS PORT ----------------------------//
    // the editor Canvas
    editorScene = new QGraphicsScene(this);
    ui->gfxEditor->setScene(editorScene);

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

    ui->gfxEditor->setMouseTracking(true);
    ui->gfxEditor->viewport()->setMouseTracking(true); // important for QGraphicsView
    ui->gfxEditor->viewport()->installEventFilter(this);

    QTimer *allDoneTimer;
    allDoneTimer = new QTimer(this);
    allDoneTimer->setSingleShot(true);

    connect(allDoneTimer, &QTimer::timeout, this, &MainWindow::reSize);
    //icon_width
    ui->txtProjectImageWidth->setText(QString("%1").arg(icon_width));
    ui->txtProjectImageHeight->setText(QString("%1").arg(icon_height));

    allDoneTimer->start(100);


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
        for(int y = 0; y < icon_height; y++){
            for(int x = 0; x < icon_width / 2; x++){
                uint8_t tmp = icon_area[y][x];
                icon_area[y][x] = icon_area[y][icon_width - 1 - x];
                icon_area[y][icon_width - 1 - x] = tmp;
            }
        }
        renderEditorCanvas();
    });

    connect(ui->cmdFlipV, &QPushButton::clicked, this, [this](){
        for(int y = 0; y < icon_height / 2; y++){
            for(int x = 0; x < icon_width; x++){
                uint8_t tmp = icon_area[y][x];
                icon_area[y][x] = icon_area[icon_height - 1 - y][x];
                icon_area[icon_height - 1 - y][x] = tmp;
            }
        }
        renderEditorCanvas();
    });


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

bool MainWindow::eventFilter(QObject *obj, QEvent *event){
    uint8_t r,g,b;
    if (obj == ui->gfxPalleteSelect->viewport() && event->type() == QEvent::MouseButtonPress){
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint viewPos = mouseEvent->pos(); // position in the QGraphicsView
        QPointF scenePos = ui->gfxPalleteSelect->mapToScene(viewPos); // map to scene coordinates

        int x = int(scenePos.x());
        int y = int(scenePos.y());

        SelectedX = x / PALETTE_BOX_HSIZE;
        SelectedY = y / PALETTE_BOX_VSIZE;

        renderPaletteCanvas();

        // clicked on another colour - ONLY if selected another colour
        if(numPrevSelectedPaletteID != numSelectedPaletteID){
            for(int i=0; i<256; i++){
                BACKUP_CLUT[i] = CLUT[i];
            }
            //printf("Commited new pallete\n");
        }

        numPrevSelectedPaletteID = numSelectedPaletteID;
        numSelectedPaletteID = (SelectedX + (SelectedY * PALETTE_WIDTH));

        SelectedPaletteID();


        return true; // mark event as handled
    }

    if (obj == ui->gfxEditor->viewport() && event->type() == QEvent::MouseMove){
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QPointF scenePos = ui->gfxEditor->mapToScene(mouseEvent->pos());

        hoverPixelX = int(scenePos.x()) / icon_zoom;
        hoverPixelY = int(scenePos.y()) / icon_zoom;

        if(hoverPixelX < 0) hoverPixelX = 0;
        if(hoverPixelY < 0) hoverPixelY = 0;
        if(hoverPixelX > editorViewPortWidth - 1)  hoverPixelX = editorViewPortWidth-1;
        if(hoverPixelY > editorViewPortHeight - 1) hoverPixelY = editorViewPortHeight-1;

        // Only draw if a button is pressed
        if (mouseEvent->buttons() & Qt::LeftButton) {
            icon_area[hoverPixelY][hoverPixelX] = numSelectedPaletteID;//currentPaletteID; // paint
        } else if (mouseEvent->buttons() & Qt::RightButton) {
            icon_area[hoverPixelY][hoverPixelX] = 0; // erase
        }

        updateTimer->start(1);
        return true; // event handled
    } else if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            //printf( "Left click at %lu\n",  mouseEvent->pos());
            icon_area[hoverPixelY][hoverPixelX] = numSelectedPaletteID;
            updateTimer->start(1);
        }
        else if (mouseEvent->button() == Qt::RightButton) {
            //printf( "Right click atlu\n",  mouseEvent->pos());
            icon_area[hoverPixelY][hoverPixelX] = 0;
            updateTimer->start(1);

        }
        else if (mouseEvent->button() == Qt::MiddleButton){
            numSelectedPaletteID = icon_area[hoverPixelY][hoverPixelX];

            SelectedX = numSelectedPaletteID % PALETTE_WIDTH;

            SelectedY = (numSelectedPaletteID / PALETTE_WIDTH) % PALETTE_HEIGHT;

            SelectedPaletteID();
            renderPaletteCanvas();
        }

        return true; // event handled
    }


    return QMainWindow::eventFilter(obj, event);
}



//// ----------------------------------------------------------------------------------------////

void MainWindow::reSize(){
    int WinXW, WinXH;
    int PWinXW, PWinXH;

    QWidget *wincontainer = ui->verticalLayoutWidget->parentWidget();
    QWidget *container = ui->verticalLayoutWidget;

    WinXW = wincontainer->width() - 16;
    WinXH = wincontainer->height() - 28;

    if(container)
        container->resize(WinXW, WinXH);  // or any size you want

    PWinXW = ui->gfxEditor->width()  - 4;
    PWinXH = ui->gfxEditor->height() - 4;

    editorViewPortWidth = PWinXW / icon_zoom;
    editorViewPortHeight = PWinXH / icon_zoom;

    if(editorViewPortWidth > icon_width) editorViewPortWidth = icon_width;
    if(editorViewPortHeight > icon_height) editorViewPortHeight = icon_height;

    //printf("EditorCanv: W:%lu, H:%lu\n", editorViewPortWidth, editorViewPortHeight);
    renderEditorCanvas();
}

void MainWindow::resizeEvent(QResizeEvent *event){
    QMainWindow::resizeEvent(event);
    reSize();
}

void MainWindow::renderEditorCanvas(){
    int visibleWidth  = editorViewPortWidth * icon_zoom;   // in pixels
    int visibleHeight = editorViewPortHeight * icon_zoom;

    editorImg = QImage(visibleWidth, visibleHeight, QImage::Format_RGB32);

    QRgb gridColor = gridEnabled ? QColor(gridRed, gridGreen, gridBlue).rgb() : 0; // choose color
    bool drawGrid = gridEnabled;

    for(int y = 0; y < visibleHeight; y++) {
        QRgb *scan = reinterpret_cast<QRgb*>(editorImg.scanLine(y));
        int imgY = y / icon_zoom;
        for(int x = 0; x < visibleWidth; x++) {
            int imgX = x / icon_zoom;

            QRgb base = colourSqueeze(CLUT[icon_area[imgY][imgX]]);
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
    // ---------------- DRAW HOVER BOX OVER THE TOP ---------------- //
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

