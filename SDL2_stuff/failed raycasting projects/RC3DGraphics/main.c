#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "gfx.h"

/* ------------------------------------------------------------------------- */
/* view + movement                                                           */
/* ------------------------------------------------------------------------- */

#define EyeHeight   6.0f
#define DuckHeight  2.5f
#define HeadMargin  1.0f
#define KneeHeight  2.0f

#define hfov        (0.73f * SCREEN_H)
#define vfov        (0.20f * SCREEN_H)

#define TURN_SPEED  0.98f
#define LOOK_SPEED  1.30f
#define MOVE_SPEED  6.0f
#define GRAVITY     28.0f
#define JUMP_SPEED  8.5f

#define FPS         60
#define A_FPS_MS    (1000 / FPS)

#define SECTOR_EPS  0.001f
#define MAX_QUEUE   256

/* ------------------------------------------------------------------------- */
/* data                                                                      */
/* ------------------------------------------------------------------------- */

static struct sector
{
    float floor, ceil;
    struct xy { float x, y; } *vertex;
    signed char *neighbors;
    unsigned npoints;
} *sectors = NULL;

static unsigned NumSectors = 0;

static struct player
{
    struct xyz { float x, y, z; } where, velocity;
    float angle, anglesin, anglecos, yaw;
    unsigned sector;
} player;

static unsigned g_prevSector = 0;
static uint8_t g_lastGoodFrame[SCREEN_W * SCREEN_H];

/* ------------------------------------------------------------------------- */
/* macros                                                                    */
/* ------------------------------------------------------------------------- */

#define min(a,b)             (((a) < (b)) ? (a) : (b))
#define max(a,b)             (((a) > (b)) ? (a) : (b))
#define clamp(a,mi,ma)       min(max(a,mi),ma)
#define vxs(x0,y0,x1,y1)     ((x0)*(y1) - (x1)*(y0))
#define Overlap(a0,a1,b0,b1) (min(a0,a1) <= max(b0,b1) && min(b0,b1) <= max(a0,a1))
#define IntersectBox(x0,y0,x1,y1,x2,y2,x3,y3) (Overlap(x0,x1,x2,x3) && Overlap(y0,y1,y2,y3))
#define PointSide(px,py,x0,y0,x1,y1) vxs((x1)-(x0), (y1)-(y0), (px)-(x0), (py)-(y0))

#define Yaw(y,z) ((y) + (z) * player.yaw)

#define Intersect(x1,y1, x2,y2, x3,y3, x4,y4) ((struct xy) { \
    vxs(vxs(x1,y1, x2,y2), (x1)-(x2), vxs(x3,y3, x4,y4), (x3)-(x4)) / vxs((x1)-(x2), (y1)-(y2), (x3)-(x4), (y3)-(y4)), \
    vxs(vxs(x1,y1, x2,y2), (y1)-(y2), vxs(x3,y3, x4,y4), (y3)-(y4)) / vxs((x1)-(x2), (y1)-(y2), (x3)-(x4), (y3)-(y4)) })

/* ------------------------------------------------------------------------- */
/* colours                                                                   */
/* ------------------------------------------------------------------------- */

#define COL_CEIL_TOP      17
#define COL_CEIL_MID      18
#define COL_FLOOR_TOP     72
#define COL_FLOOR_MID     73
#define COL_WALL_NEAR     26
#define COL_WALL_FAR      255
#define COL_LOWER_NEAR    48
#define COL_LOWER_FAR     31

static inline uint8_t shadeRange(uint8_t nearCol, uint8_t farCol, int z, int zmax)
{
    if (z < 0) z = 0;
    if (z > zmax) z = zmax;
    return (uint8_t)(nearCol + ((farCol - nearCol) * z) / zmax);
}

/* ------------------------------------------------------------------------- */
/* load/unload                                                               */
/* ------------------------------------------------------------------------- */

static void LoadData(const char *filename)
{
    FILE *fp = fopen(filename, "rt");
    if (!fp) { perror(filename); exit(1); }

    char Buf[256], word[256], *ptr;
    struct xy *vert = NULL, v;
    int n, m, NumVertices = 0;

    while (fgets(Buf, sizeof Buf, fp))
    {
        switch (sscanf(ptr = Buf, "%32s%n", word, &n) == 1 ? word[0] : '\0')
        {
            case 'v':
                for (sscanf(ptr += n, "%f%n", &v.y, &n);
                     sscanf(ptr += n, "%f%n", &v.x, &n) == 1; )
                {
                    vert = realloc(vert, ++NumVertices * sizeof(*vert));
                    vert[NumVertices - 1] = v;
                }
                break;

            case 's':
            {
                sectors = realloc(sectors, ++NumSectors * sizeof(*sectors));
                struct sector *sect = &sectors[NumSectors - 1];
                int *num = NULL;

                sscanf(ptr += n, "%f%f%n", &sect->floor, &sect->ceil, &n);

                for (m = 0; sscanf(ptr += n, "%32s%n", word, &n) == 1 && word[0] != '#'; )
                {
                    num = realloc(num, ++m * sizeof(*num));
                    num[m - 1] = (word[0] == 'x') ? -1 : atoi(word);
                }

                sect->npoints   = m /= 2;
                sect->neighbors = malloc(m * sizeof(*sect->neighbors));
                sect->vertex    = malloc((m + 1) * sizeof(*sect->vertex));

                for (n = 0; n < m; ++n) sect->neighbors[n] = num[m + n];
                for (n = 0; n < m; ++n) sect->vertex[n + 1] = vert[num[n]];
                sect->vertex[0] = sect->vertex[m];

                free(num);
                break;
            }

            case 'p':
            {
                float angle;
                sscanf(ptr += n, "%f %f %f %d", &v.x, &v.y, &angle, &n);
                player = (struct player){ { v.x, v.y, 0 }, { 0,0,0 }, angle, 0,0,0, (unsigned)n };
                player.where.z = sectors[player.sector].floor + EyeHeight;
                break;
            }
        }
    }

    fclose(fp);
    free(vert);

    player.anglesin = sinf(player.angle);
    player.anglecos = cosf(player.angle);
    g_prevSector = player.sector;
}

static void UnloadData(void)
{
    for (unsigned a = 0; a < NumSectors; ++a) free(sectors[a].vertex);
    for (unsigned a = 0; a < NumSectors; ++a) free(sectors[a].neighbors);
    free(sectors);
    sectors = NULL;
    NumSectors = 0;
}

/* ------------------------------------------------------------------------- */
/* tiny draw                                                                 */
/* ------------------------------------------------------------------------- */

static void vline(int x, int y1, int y2, uint8_t top, uint8_t middle, uint8_t bottom)
{
    if ((unsigned)x >= SCREEN_W) return;

    y1 = clamp(y1, 0, SCREEN_H - 1);
    y2 = clamp(y2, 0, SCREEN_H - 1);

    if (y2 < y1) return;

    if (y1 == y2) {
        fb[(y1 * SCREEN_W) + x] = middle;
        return;
    }

    fb[(y1 * SCREEN_W) + x] = top;
    for (int y = y1 + 1; y < y2; ++y) {
        fb[(y * SCREEN_W) + x] = middle;
    }
    fb[(y2 * SCREEN_W) + x] = bottom;
}

static void DrawBackground(void)
{
    int half = SCREEN_H / 2;

    for (int y = 0; y < half; ++y) {
        uint8_t c = (y < (half / 2)) ? COL_CEIL_TOP : COL_CEIL_MID;
        memset(&fb[y * SCREEN_W], c, SCREEN_W);
    }

    for (int y = half; y < SCREEN_H; ++y) {
        uint8_t c = (y < (half + half / 2)) ? COL_FLOOR_TOP : COL_FLOOR_MID;
        memset(&fb[y * SCREEN_W], c, SCREEN_W);
    }
}

/* ------------------------------------------------------------------------- */
/* sector tests                                                              */
/* ------------------------------------------------------------------------- */

static int PointInSector(float px, float py, unsigned sectno)
{
    const struct sector *sect = &sectors[sectno];
    const struct xy *vert = sect->vertex;

    for (unsigned s = 0; s < sect->npoints; ++s) {
        float side = PointSide(px, py,
                               vert[s+0].x, vert[s+0].y,
                               vert[s+1].x, vert[s+1].y);
        if (side < -SECTOR_EPS) {
            return 0;
        }
    }

    return 1;
}

static int FindPlayerSector(float px, float py, unsigned startSector)
{
    if (startSector < NumSectors) {
        if (PointInSector(px, py, startSector)) {
            return (int)startSector;
        }

        {
            const struct sector *sect = &sectors[startSector];
            for (unsigned s = 0; s < sect->npoints; ++s) {
                int n = sect->neighbors[s];
                if (n >= 0 && (unsigned)n < NumSectors) {
                    if (PointInSector(px, py, (unsigned)n)) {
                        return n;
                    }
                }
            }
        }
    }

    for (unsigned i = 0; i < NumSectors; ++i) {
        if (PointInSector(px, py, i)) {
            return (int)i;
        }
    }

    return -1;
}

static void RepairPlayerSector(float dirx, float diry)
{
    float probeX = player.where.x;
    float probeY = player.where.y;
    float len2 = dirx * dirx + diry * diry;

    if (len2 > 0.0f) {
        float len = sqrtf(len2);
        float invLen = 1.0f / len;
        probeX += dirx * invLen * SECTOR_EPS;
        probeY += diry * invLen * SECTOR_EPS;
    }

    {
        int fixedSector = FindPlayerSector(probeX, probeY, player.sector);
        if (fixedSector < 0) {
            fixedSector = FindPlayerSector(player.where.x, player.where.y, player.sector);
        }
        if (fixedSector >= 0) {
            player.sector = (unsigned)fixedSector;
        }
    }
}

/* ------------------------------------------------------------------------- */
/* movement                                                                  */
/* ------------------------------------------------------------------------- */

static void MovePlayer(float dx, float dy)
{
    float px = player.where.x;
    float py = player.where.y;
    float nx = px + dx;
    float ny = py + dy;

    const struct sector *sect = &sectors[player.sector];
    const struct xy *vert = sect->vertex;

    for (unsigned s = 0; s < sect->npoints; ++s)
    {
        if (sect->neighbors[s] >= 0
        && IntersectBox(px, py, nx, ny,
                        vert[s+0].x, vert[s+0].y,
                        vert[s+1].x, vert[s+1].y)
        && PointSide(nx, ny,
                     vert[s+0].x, vert[s+0].y,
                     vert[s+1].x, vert[s+1].y) < 0)
        {
            player.sector = sect->neighbors[s];
            break;
        }
    }

    player.where.x = nx;
    player.where.y = ny;

    RepairPlayerSector(dx, dy);

    player.anglesin = sinf(player.angle);
    player.anglecos = cosf(player.angle);
}

/* ------------------------------------------------------------------------- */
/* robust portal render                                                      */
/* ------------------------------------------------------------------------- */

struct queue_item {
    int sectorno;
    int sx1, sx2;
};


static int DrawScreenFromSector(unsigned startSector)
{
    enum { MAX_SECTOR_VISITS = 16 };

    struct queue_item queue[MAX_QUEUE];
    int head = 0;
    int tail = 0;

    int ytop[SCREEN_W];
    int ybottom[SCREEN_W];
    uint8_t visitCount[NumSectors];
    int anyPortalDraw = 0;

    if (startSector >= NumSectors) {
        return 0;
    }

    DrawBackground();

    for (int x = 0; x < SCREEN_W; ++x) {
        ytop[x] = 0;
        ybottom[x] = SCREEN_H - 1;
    }

    memset(visitCount, 0, sizeof(visitCount));

    queue[head].sectorno = (int)startSector;
    queue[head].sx1 = 0;
    queue[head].sx2 = SCREEN_W - 1;
    head = (head + 1) % MAX_QUEUE;

    while (head != tail)
    {
        struct queue_item now = queue[tail];
        tail = (tail + 1) % MAX_QUEUE;

        if (now.sectorno < 0 || now.sectorno >= (int)NumSectors) {
            continue;
        }

        if (visitCount[now.sectorno] >= MAX_SECTOR_VISITS) {
            continue;
        }
        visitCount[now.sectorno]++;

        const struct sector *sect = &sectors[now.sectorno];

        for (unsigned s = 0; s < sect->npoints; ++s)
        {
            float vx1 = sect->vertex[s+0].x - player.where.x;
            float vy1 = sect->vertex[s+0].y - player.where.y;
            float vx2 = sect->vertex[s+1].x - player.where.x;
            float vy2 = sect->vertex[s+1].y - player.where.y;

            float pcos = player.anglecos;
            float psin = player.anglesin;

            float tx1 = vx1 * psin - vy1 * pcos;
            float tz1 = vx1 * pcos + vy1 * psin;
            float tx2 = vx2 * psin - vy2 * pcos;
            float tz2 = vx2 * pcos + vy2 * psin;

            if (tz1 <= 0.0f && tz2 <= 0.0f) continue;

            if (tz1 <= 0.0f || tz2 <= 0.0f)
            {
                float nearz = 1e-4f, farz = 5.0f, nearside = 1e-5f, farside = 20.0f;
                struct xy i1 = Intersect(tx1,tz1,tx2,tz2, -nearside,nearz, -farside,farz);
                struct xy i2 = Intersect(tx1,tz1,tx2,tz2,  nearside,nearz,  farside,farz);

                if (tz1 < nearz) {
                    if (i1.y > 0.0f) { tx1 = i1.x; tz1 = i1.y; }
                    else             { tx1 = i2.x; tz1 = i2.y; }
                }

                if (tz2 < nearz) {
                    if (i1.y > 0.0f) { tx2 = i1.x; tz2 = i1.y; }
                    else             { tx2 = i2.x; tz2 = i2.y; }
                }
            }

            if (tz1 <= 0.0f || tz2 <= 0.0f) continue;

            float xscale1 = hfov / tz1;
            float xscale2 = hfov / tz2;
            float yscale1 = vfov / tz1;
            float yscale2 = vfov / tz2;

            int x1 = SCREEN_W / 2 - (int)(tx1 * xscale1);
            int x2 = SCREEN_W / 2 - (int)(tx2 * xscale2);

            if (x1 == x2) continue;

            if (x1 > x2) {
                int ti = x1; x1 = x2; x2 = ti;

                float tf;
                tf = tx1; tx1 = tx2; tx2 = tf;
                tf = tz1; tz1 = tz2; tz2 = tf;
                tf = yscale1; yscale1 = yscale2; yscale2 = tf;
            }

            if (x2 < now.sx1 || x1 > now.sx2) continue;

            int beginx = max(x1, now.sx1);
            int endx   = min(x2, now.sx2);
            if (beginx > endx) continue;

            float yceil  = sect->ceil  - player.where.z;
            float yfloor = sect->floor - player.where.z;

            int neighbor = sect->neighbors[s];
            float nyceil = 0.0f;
            float nyfloor = 0.0f;

            if (neighbor >= 0) {
                nyceil  = sectors[neighbor].ceil  - player.where.z;
                nyfloor = sectors[neighbor].floor - player.where.z;
            }

            int y1a  = SCREEN_H/2 - (int)(Yaw(yceil,  tz1) * yscale1);
            int y1b  = SCREEN_H/2 - (int)(Yaw(yfloor, tz1) * yscale1);
            int y2a  = SCREEN_H/2 - (int)(Yaw(yceil,  tz2) * yscale2);
            int y2b  = SCREEN_H/2 - (int)(Yaw(yfloor, tz2) * yscale2);

            int ny1a = SCREEN_H/2 - (int)(Yaw(nyceil,  tz1) * yscale1);
            int ny1b = SCREEN_H/2 - (int)(Yaw(nyfloor, tz1) * yscale1);
            int ny2a = SCREEN_H/2 - (int)(Yaw(nyceil,  tz2) * yscale2);
            int ny2b = SCREEN_H/2 - (int)(Yaw(nyfloor, tz2) * yscale2);

            for (int x = beginx; x <= endx; ++x)
            {
                float t = (float)(x - x1) / (float)(x2 - x1);
                float zf = tz1 + (tz2 - tz1) * t;
                int z = (int)(zf * 8.0f);

                int ya  = y1a  + (int)((y2a  - y1a)  * t);
                int yb  = y1b  + (int)((y2b  - y1b)  * t);
                int cya = clamp(ya, ytop[x], ybottom[x]);
                int cyb = clamp(yb, ytop[x], ybottom[x]);

                if (neighbor >= 0)
                {
                    int nya  = ny1a + (int)((ny2a - ny1a) * t);
                    int nyb  = ny1b + (int)((ny2b - ny1b) * t);
                    int cnya = clamp(nya, ytop[x], ybottom[x]);
                    int cnyb = clamp(nyb, ytop[x], ybottom[x]);

                    uint8_t upperCol = shadeRange(COL_WALL_NEAR,  COL_WALL_FAR,  z, 255);
                    uint8_t lowerCol = shadeRange(COL_LOWER_NEAR, COL_LOWER_FAR, z, 255);

                    if (cnya > cya) {
                        vline(x, cya, cnya - 1, 0, upperCol, 0);
                        anyPortalDraw = 1;
                    }

                    if (cyb > cnyb) {
                        vline(x, cnyb + 1, cyb, 0, lowerCol, 0);
                        anyPortalDraw = 1;
                    }

                    ytop[x] = clamp(max(cya, cnya), ytop[x], SCREEN_H - 1);
                    ybottom[x] = clamp(min(cyb, cnyb), 0, ybottom[x]);
                }
                else
                {
                    uint8_t wallCol = shadeRange(COL_WALL_NEAR, COL_WALL_FAR, z, 255);
                    vline(x, cya, cyb, 0, wallCol, 0);
                    anyPortalDraw = 1;
                }
            }

            if (neighbor >= 0)
            {
                int nexthead = (head + 1) % MAX_QUEUE;
                if (nexthead != tail)
                {
                    queue[head].sectorno = neighbor;
                    queue[head].sx1 = beginx;
                    queue[head].sx2 = endx;
                    head = nexthead;
                }
            }
        }
    }

    return anyPortalDraw;
}

/* ------------------------------------------------------------------------- */
/* frame render                                                              */
/* ------------------------------------------------------------------------- */

static void RenderFrameStable(void)
{
    memcpy(g_lastGoodFrame, fb, sizeof(fb));

    RepairPlayerSector(player.velocity.x, player.velocity.y);

    if (DrawScreenFromSector(player.sector)) {
        g_prevSector = player.sector;
        return;
    }

    if (DrawScreenFromSector(g_prevSector)) {
        return;
    }

    memcpy(fb, g_lastGoodFrame, sizeof(fb));
}

/* ------------------------------------------------------------------------- */
/* SDL2 setup                                                                */
/* ------------------------------------------------------------------------- */

int tmr1;
SDL_Window *sdl_win;
SDL_Renderer *ren;
SDL_Texture *tex;
int running = 1;

int updateFPS(void)
{
    static uint32_t fpsTimer = 0;
    static uint32_t lastFrameTime = 0;
    static int frameCount = 0;
    static int uncappedCount = 0;
    static int fps = 0;
    static int uncappedFPS = 0;

    uint32_t now = SDL_GetTicks();
    uncappedCount++;

    if (now - fpsTimer >= 1000) {
        fps = frameCount;
        uncappedFPS = uncappedCount;
        frameCount = 0;
        uncappedCount = 0;
        fpsTimer = now;
    }

    {
        char buf[64];
        sprintf(buf, "FPS: %d (Uncapped: %d)", fps, uncappedFPS);
        drawText(0, 1, buf, 16);
        drawText(2, 1, buf, 16);
        drawText(1, 0, buf, 16);
        drawText(1, 2, buf, 16);
        drawText(1, 1, buf, 15);
    }

    if (now - lastFrameTime >= A_FPS_MS) {
        lastFrameTime += A_FPS_MS;
        frameCount++;
        return 1;
    }

    return 0;
}

int BasicSDL2Setup(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    sdl_win = SDL_CreateWindow(
        "Portal Demo (SDL2)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W * ZOOM, SCREEN_H * ZOOM, 0
    );
    if (!sdl_win) {
        fprintf(stderr, "CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    ren = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdl_win);
        SDL_Quit();
        return 1;
    }

    tex = SDL_CreateTexture(
        ren,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_W,
        SCREEN_H
    );
    if (!tex) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(sdl_win);
        SDL_Quit();
        return 1;
    }

    return 0;
}

void EndSDL2Session(void)
{
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(sdl_win);
    SDL_Quit();
}

void screenupdate(void)
{
    videoMemToScreen();
    SDL_UpdateTexture(tex, NULL, pb, SCREEN_W * (int)sizeof(uint32_t));
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
}

/* ------------------------------------------------------------------------- */
/* main                                                                      */
/* ------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    const char *mapPath = "map-clear.txt";
    if (argc >= 2 && argv[1] && argv[1][0]) {
        mapPath = argv[1];
    }

    if (BasicSDL2Setup() != 0) {
        return 1;
    }

    LoadData(mapPath);

    DrawBackground();
    memcpy(g_lastGoodFrame, fb, sizeof(fb));

    SDL_SetRelativeMouseMode(SDL_TRUE);

    int wsad[4] = {0,0,0,0};
    int ground = 0;
    int falling = 1;
    int moving = 0;
    int ducking = 0;
    float yaw = 0.0f;

    uint32_t lastTicks = SDL_GetTicks();

    do
    {
        uint32_t nowTicks = SDL_GetTicks();
        float dt = (float)(nowTicks - lastTicks) / 1000.0f;
        lastTicks = nowTicks;

        if (dt > 0.05f) dt = 0.05f;
        g_prevSector = player.sector;

        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            switch (ev.type)
            {
                case SDL_QUIT:
                    running = 0;
                    break;

                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    switch (ev.key.keysym.sym)
                    {
                        case SDLK_w: wsad[0] = ev.type == SDL_KEYDOWN; break;
                        case SDLK_s: wsad[1] = ev.type == SDL_KEYDOWN; break;
                        case SDLK_a: wsad[2] = ev.type == SDL_KEYDOWN; break;
                        case SDLK_d: wsad[3] = ev.type == SDL_KEYDOWN; break;
                        case SDLK_q: running = 0; break;

                        case SDLK_SPACE:
                            if (ev.type == SDL_KEYDOWN && ground) {
                                player.velocity.z = JUMP_SPEED;
                                falling = 1;
                            }
                            break;

                        case SDLK_LCTRL:
                        case SDLK_RCTRL:
                            ducking = ev.type == SDL_KEYDOWN;
                            falling = 1;
                            break;

                        default:
                            break;
                    }
                    break;
            }
        }

        {
            int mx, my;
            SDL_GetRelativeMouseState(&mx, &my);

            player.angle += mx * TURN_SPEED * dt;
            yaw = clamp(yaw + my * LOOK_SPEED * dt, -5.0f, 5.0f);
            player.yaw = yaw - player.velocity.z * 0.05f * dt;
            player.anglesin = sinf(player.angle);
            player.anglecos = cosf(player.angle);
        }

        {
            float eyeheight = ducking ? DuckHeight : EyeHeight;
            ground = !falling;

            if (falling)
            {
                player.velocity.z -= GRAVITY * dt;
                float nextz = player.where.z + player.velocity.z * dt;

                if (player.velocity.z < 0.0f && nextz < sectors[player.sector].floor + eyeheight)
                {
                    player.where.z = sectors[player.sector].floor + eyeheight;
                    player.velocity.z = 0.0f;
                    falling = 0;
                    ground = 1;
                }
                else if (player.velocity.z > 0.0f && nextz > sectors[player.sector].ceil)
                {
                    player.velocity.z = 0.0f;
                    falling = 1;
                }

                if (falling) {
                    player.where.z += player.velocity.z * dt;
                    moving = 1;
                }
            }
        }

        if (moving)
        {
            float px = player.where.x;
            float py = player.where.y;
            float dx = player.velocity.x * dt;
            float dy = player.velocity.y * dt;

            const struct sector *sect = &sectors[player.sector];
            const struct xy *vert = sect->vertex;
            float eyeheight = ducking ? DuckHeight : EyeHeight;

            for (unsigned s = 0; s < sect->npoints; ++s)
            {
                if (IntersectBox(px, py, px + dx, py + dy,
                                 vert[s+0].x, vert[s+0].y,
                                 vert[s+1].x, vert[s+1].y)
                && PointSide(px + dx, py + dy,
                             vert[s+0].x, vert[s+0].y,
                             vert[s+1].x, vert[s+1].y) < 0)
                {
                    float hole_low  = (sect->neighbors[s] < 0) ?  9e9f : max(sect->floor, sectors[sect->neighbors[s]].floor);
                    float hole_high = (sect->neighbors[s] < 0) ? -9e9f : min(sect->ceil,  sectors[sect->neighbors[s]].ceil );

                    if (hole_high < player.where.z + HeadMargin
                    ||  hole_low  > player.where.z - eyeheight + KneeHeight)
                    {
                        float xd = vert[s+1].x - vert[s+0].x;
                        float yd = vert[s+1].y - vert[s+0].y;
                        float denom = (xd * xd) + (yd * yd);

                        if (denom > 0.0f) {
                            float dot = (dx * xd + dy * yd) / denom;
                            dx = xd * dot;
                            dy = yd * dot;
                        } else {
                            dx = 0.0f;
                            dy = 0.0f;
                        }

                        moving = 0;
                    }
                }
            }

            MovePlayer(dx, dy);
            falling = 1;
        }

        {
            float move_vec[2] = {0.f, 0.f};

            if (wsad[0]) { move_vec[0] += player.anglecos * MOVE_SPEED; move_vec[1] += player.anglesin * MOVE_SPEED; }
            if (wsad[1]) { move_vec[0] -= player.anglecos * MOVE_SPEED; move_vec[1] -= player.anglesin * MOVE_SPEED; }
            if (wsad[2]) { move_vec[0] += player.anglesin * MOVE_SPEED; move_vec[1] -= player.anglecos * MOVE_SPEED; }
            if (wsad[3]) { move_vec[0] -= player.anglesin * MOVE_SPEED; move_vec[1] += player.anglecos * MOVE_SPEED; }

            int pushing = wsad[0] || wsad[1] || wsad[2] || wsad[3];
            float acceleration = pushing ? 12.0f * dt : 8.0f * dt;

            if (acceleration > 1.0f) acceleration = 1.0f;

            player.velocity.x = player.velocity.x * (1.0f - acceleration) + move_vec[0] * acceleration;
            player.velocity.y = player.velocity.y * (1.0f - acceleration) + move_vec[1] * acceleration;

            if (pushing) moving = 1;
        }

        if (fabsf(player.velocity.x) < 0.001f) player.velocity.x = 0.0f;
        if (fabsf(player.velocity.y) < 0.001f) player.velocity.y = 0.0f;

        if (player.velocity.x == 0.0f && player.velocity.y == 0.0f && !falling) {
            moving = 0;
        }

        RenderFrameStable();
        updateFPS();
        screenupdate();

    } while (running);

    UnloadData();
    EndSDL2Session();
    return 0;
}