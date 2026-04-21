#ifndef RC3D_H
#define RC3D_H

#include <stdint.h>

#define RC3D_SKYBOX_W          1024
#define RC3D_SKYBOX_H          240
#define RC3D_MAX_SPRITES       64
#define RC3D_INVALID_SPRITE    (-1)
#define RC3D_SPRITE_TEX_MAN    253
#define RC3D_SPRITE_TEX_GRICY  252

#define RC3D_SECTOR_STATE_NONE           0u
#define RC3D_SECTOR_STATE_RAISE_FLOOR    0x01u
#define RC3D_SECTOR_STATE_LOWER_FLOOR    0x02u
#define RC3D_SECTOR_STATE_LOWER_CEILING  0x04u
#define RC3D_SECTOR_STATE_RAISE_CEILING  0x08u

//#define OBJECT_TRIGGER_SIMPLESPRITE 0
//#define OBJECT_TRIGGER_SECTOR_DISTANCE     1
//#define OBJECT_TRIGGER_SECTOR_STEPPED_IN   2
//#define OBJECT_TRIGGER_SECTOR_STEPPED_OUT  3


typedef enum {
    RC3D_OBJTYPE_SPRITE                 = 0u,
    RC3D_OBJTYPE_SECTOR_TRIGGER_INOUT   = 1u,
    RC3D_OBJTYPE_SECTOR_TRIGGER_ENTER   = 2u,
    RC3D_OBJTYPE_SECTOR_TRIGGER_EXIT    = 3u,
    RC3D_OBJTYPE_ROUTE_PREVIEW          = 4u,
    RC3D_OBJTYPE_BAKED_ROUTE_NODE       = 5u,
    RC3D_OBJTYPE_OBJECT_TRIGGER_INOUT   = 6u,
    RC3D_OBJTYPE_OBJECT_TRIGGER_ENTER   = 7u,
    RC3D_OBJTYPE_OBJECT_TRIGGER_EXIT    = 8u,
    RC3D_OBJTYPE_OBJECT_TELEPORTER      = 9u
} RC3D_ObjectType;


#define OBJECT_TYPE_BASIC_SPRITE    0
#define OBJECT_TYPE_ENTEREXIT       1
#define OBJECT_TYPE_ENTERONLY       2
#define OBJECT_TYPE_EXITONLY        3
#define OBJECT_TYPE_NAVIGATION      4
#define OBJECT_TYPE_BAKEDNAV        5   // use this in the run time
#define OBJECT_TYPE_TELEPORTER      9

#define OBJECT_GENERAL_FLAGS_ENABLE 0x01    // bit 0, just a basic enable ;)


extern int g_viewport_top;
extern int g_viewport_left;
extern int g_viewport_width;
extern int g_viewport_height;
extern float g_draw_distance;



#define TEXSHIFT_LEFT       0x01
#define TEXSHIFT_RIGHT      0x02
#define TEXSHIFT_UP         0x04
#define TEXSHIFT_DOWN       0x08
#define TEXSHIFT_SINOUSSX   0x10
#define TEXSHIFT_SINOUSCX   0x20
#define TEXSHIFT_SINOUSSY   0x40
#define TEXSHIFT_SINOUSCY   0x80


#define RC3D_VIEWPORT_LEFT      0
#define RC3D_VIEWPORT_TOP       0
#define RC3D_VIEWPORT_WIDTH     SCREEN_W
#define RC3D_VIEWPORT_HEIGHT    SCREEN_H

#define RC3D_TEX_SIZE           64
#define RC3D_TEX_MASK           (RC3D_TEX_SIZE - 1)



#define RC3D_SECTOR_FLAGS_FLICKERING_LIGHTS     0x100
#define RC3D_SECTOR_FLAGS_PULSATING_LIGHT       0x200
#define RC3D_SECTOR_FLAGS_FULLBRIGHT            0x400
#define RC3D_SECTOR_FLAGS_EFFECTWALLS           0x800
#define RC3D_SECTOR_FLAGS_DAMAGEZONE            0x1000   /// YEAH, STAY out of these zones if you can ;)
#define RC3D_SECTOR_FLAGS_MINIMAP_DISCOVERED    0x2000   /* runtime minimap discovery */


typedef int32_t RC3D_Fixed;


typedef struct {
    RC3D_Fixed x;
    RC3D_Fixed y;
} RC3D_FixedVec2;

typedef struct {
    float x;
    float y;
    RC3D_Fixed xFixed;
    RC3D_Fixed yFixed;
    float z;
    float vz;
    float tHeadbob;
    float angle;
    int sector;
} RC3D_Player;

typedef struct {
    float t;
    int wallIndex;
    int hit;
} RC3D_WallHit;

typedef struct {
    int hit;
    int wallIndex;
    float normalX;
    float normalY;
    float penetration;
} RC3D_BlockingContact;

typedef struct {
    //uint8_t pix[RC3D_TEX_SIZE * RC3D_TEX_SIZE];
    uint8_t *pix;
} RC3D_Texture;

typedef struct {
    float x;
    float y;
    RC3D_Fixed xFixed;
    RC3D_Fixed yFixed;
    float baseZ;
    float width;
    float height;
    int16_t sector;
    uint8_t texId;
    uint8_t inUse;
    uint8_t active;
    uint8_t reserved;
} RC3D_Sprite;

typedef struct {
    int16_t y0;
    int16_t y1;
    uint16_t depth;
} RC3D_WallDepthSpan;

typedef struct {
    int16_t sector;
    int16_t clipTop;
    int16_t clipBottom;
    uint16_t depthLimit;
} RC3D_VisibleTraceSegment;

typedef struct {
    const RC3D_Sprite *sprite;
    const uint8_t *texels;
    float camDepth;
    float scale;
    int leftX;
    int rightX;
    int topY;
    int bottomY;
    int unclampedWidth;
    int unclampedHeight;
    int spriteClipTop;
    int spriteClipBottom;
    uint8_t spriteGlow;
    uint16_t depthCode;
} RC3D_VisibleSprite;

typedef struct {
    RC3D_VisibleTraceSegment *visibleTrace;
    uint8_t *visibleTraceCount;
    RC3D_WallDepthSpan *wallSpans;
    uint8_t *wallSpanCount;
} RC3D_ColumnContext;









void rc3dSetViewport(int left, int top, int width, int height);
void rc3dResetViewport(void);
void rc3dSetDrawDistance(float distance);
void rc3dInit(void);
void rc3dPreparePalette();
void rc3dSetLightRange(float brightRange, float midRange, float darkRange);
void rc3dLightRange(float brightRange, float midRange, float darkRange);
void rc3dClearSprites(void);
int rc3dSpriteCreate(float x, float y, float z, float width, float height, uint8_t texId);
void rc3dSpriteDestroy(int spriteId);
void rc3dSpriteSetActive(int spriteId, int active);
void rc3dSpriteSetPosition(int spriteId, float x, float y);
void rc3dSpriteSetPositionFixed(int spriteId, int32_t xFixed, int32_t yFixed);
void rc3dSpriteSetSize(int spriteId, float width, float height);
void rc3dSpriteSetTexture(int spriteId, uint8_t texId);
void rc3dSpriteSetBaseZ(int spriteId, float baseZ);


// Sector API
int rc3dGetSectorByTag(int32_t tagId);  // get the first sector with this tagid
int rc3dSetSectorStateByTag(int32_t tagId, uint32_t stateFlags);
void rc3dSetSectorLightLevel(int32_t sectorId, uint8_t level);

// Sector API - Minimapping
int rc3dMinimapRevealSector(int32_t sectorId);
int rc3dMinimapRevealAll(void);
void rc3dMinimapResetDiscovery(void);

// Textures API
void shiftTexture(uint8_t texIndex, int8_t dir);
void shiftTextureFX(
    uint8_t texIndex,
    uint8_t flags,
    float speedscalex,
    float speedscaley,
    float timescalex,
    float timescaley,
    float frametime);

int rc3dTextureAlloc(RC3D_Texture *texture);
void rc3dTextureFree(RC3D_Texture *texture);
int rc3dTextureSetSlotPixels(uint8_t textureindex, uint8_t *pixels);
uint8_t *rc3d_GetTexturePtr(uint8_t textureindex);
void copyTextureToTexture(uint8_t *from, uint8_t *to, int sizex, int sizey);

void rc3dUpdate(float dt, const uint8_t *keys, int mouseDx);
void rc3dRender(void);

// objects API
int rc3dSetObjectStateByTag(int32_t tagId, uint32_t stateFlags);
void enableObject(int tagId, uint8_t en);
int getObjectState(int pTagId);
void processObjects(int pTagId, int pType);

// maploading API
int rc3dLoadMapBinary(const char *path);
void rc3dUnloadMapBinary(void);




void randSeed(uint32_t seed);
uint32_t rand32(void);
uint32_t randRange(uint32_t min, uint32_t max);
float randFloat(void);


#endif
