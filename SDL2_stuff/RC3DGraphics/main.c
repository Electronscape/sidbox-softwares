#include <SDL2/SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>





// basic includes
#include "gfx.h"
#include "rc3d/rc3d.h"


#define FPS        75

int tmr1;
//static uint8_t mouseAction = 0;
static uint32_t fpsTimer = 0;
static Uint64 lastFrameTick = 0;
static Uint64 perfFreq = 0;
static int frameCount = 0;        // frames actually rendered
static int uncappedCount = 0;     // every loop iteration
static int fps = 0;
static int uncappedFPS = 0;




int updateFPS(void) {
    uint32_t now = SDL_GetTicks();
    Uint64 nowPerf = SDL_GetPerformanceCounter();
    int allowFrame = 0;

    if (perfFreq == 0) {
        perfFreq = SDL_GetPerformanceFrequency();
        lastFrameTick = nowPerf;
        fpsTimer = now;
        allowFrame = 1;
    }

    uncappedCount++; // count every loop iteration

    // Update FPS every second
    if (now - fpsTimer >= 1000) {
        fps = frameCount;
        uncappedFPS = uncappedCount;

        frameCount = 0;
        uncappedCount = 0;
        fpsTimer = now;
    }

    if (allowFrame) {
        frameCount++;
        return 1;
    }

    if (FPS <= 0) {
        lastFrameTick = nowPerf;
        frameCount++;
        return 1;
    }

    // Check if enough time has passed for the next frame.
    const Uint64 targetTickDelta = (Uint64)((double)perfFreq / (double)FPS);
    if ((targetTickDelta == 0) || ((nowPerf - lastFrameTick) >= targetTickDelta)) {
        lastFrameTick = nowPerf;
        frameCount++;
        return 1;
    }

    return 0; // skip this loop iteration for logic
}

static void drawFPSCounter(void) {
    char buf[64];
    snprintf(buf, sizeof(buf), "FPS: %d (Uncapped: %d)", fps, uncappedFPS);
    drawTextO(0, 1, buf, 15);
}

static int clampScreenCoord(int value, int maxValue)
{
    if (maxValue <= 0) {
        return 0;
    }

    if (value < 0) {
        return 0;
    }

    if (value >= maxValue) {
        return maxValue - 1;
    }

    return value;
}

static int windowToScreenCoord(int value, int screenSize)
{
    return clampScreenCoord(value / ZOOM, screenSize);
}

static int screenToWindowCoord(int value, int screenSize)
{
    const int clamped = clampScreenCoord(value, screenSize);
    return (clamped * ZOOM) + (ZOOM / 2);
}

static void drawCursorGlyph(int x, int y, const uint8_t glyph[8], uint8_t color)
{
    for (int row = 0; row < 8; ++row) {
        uint8_t bits = glyph[row];

        for (int col = 0; col < 8; ++col) {
            if (bits & (0x80u >> col)) {
                putPixel(x + col, y + row, color);
            }
        }
    }
}

static void drawMousePointerCursor(int x, int y, int clickable)
{
    static const uint8_t cursorNormal[8] = {
        0x80, 0xC0, 0xE0, 0xF0, 0xE0, 0xB0, 0x18, 0x08
    };
    static const uint8_t cursorClickable[8] = {
        0x86, 0xC1, 0xE1, 0xF2, 0xE0, 0xB2, 0x18, 0x08
    };
    const uint8_t *glyph = clickable ? cursorClickable : cursorNormal;
    const uint8_t color = clickable ? 29u : 2u;

    drawCursorGlyph(x + 1, y + 1, glyph, 16u);
    drawCursorGlyph(x, y, glyph, color);
}


SDL_Window *sdl_win;
SDL_Renderer *ren;
SDL_Texture *tex;

int BasicSDL2Setup(){
    //// HOST STARTUP
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    sdl_win = SDL_CreateWindow(
        "Raycasting GAME Test (SDL2)",
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

void EndSDL2Session(){
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(sdl_win);
    SDL_Quit();
}



int running = 1;

void screenupdate(){
    videoMemToScreen();
    SDL_UpdateTexture(tex, NULL, pb, SCREEN_W * (int)sizeof(uint32_t));
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);


#if(0)
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running = 0;
        }


        if (e.type == SDL_MOUSEBUTTONDOWN) {
            running = 0;
        }
    }

    //if(running==0) EndSDL2Session();// extermely gross way to close app!
#endif
}

void moveSprite();

RC3D_Texture forcefield[4];
RC3D_Texture water[4];

static void freeTextureFrames(RC3D_Texture *frames, int count)
{
    if (!frames || count <= 0) {
        return;
    }

    for (int i = 0; i < count; ++i) {
        rc3dTextureFree(&frames[i]);
    }
}

static int loadTextureFrames(RC3D_Texture *frames, const char *const *paths, int count)
{
    if (!frames || !paths || count <= 0) {
        return 0;
    }

    for (int i = 0; i < count; ++i) {
        if (!rc3dTextureAlloc(&frames[i])) {
            freeTextureFrames(frames, count);
            return 0;
        }

        LoadPPB(paths[i], frames[i].pix);
    }

    return 1;
}

#define MAP_SPECIFIC_OBJECT_TEST    20
void doCustomGameLogic(){   // to demonstrate how to use the engine for the bits available
    //void rc3dSetSectorLightLevel(int sectorId, uint8_t level)
    int theSector = rc3dGetSectorByTag(20); // this map should only Have one sector with this tag number
    if (theSector){
        if(getObjectState(MAP_SPECIFIC_OBJECT_TEST))
            rc3dSetSectorLightLevel(theSector, 7);
        else 
            rc3dSetSectorLightLevel(theSector, 0);
    }

}

int main(int argc, char **argv)
{
    static const char *const forcefieldFrames[4] = {
        "textures/15a.ppb",
        "textures/15b.ppb",
        "textures/15c.ppb",
        "textures/15d.ppb"
    };
    static const char *const waterFrames[4] = {
        "textures/05a.ppb",
        "textures/05b.ppb",
        "textures/05c.ppb",
        "textures/05d.ppb"
    };

    if (BasicSDL2Setup() != 0) {
        return 1;
    }

    //const char *mapPath = "./load-default-map.notmap-ext";  // intentionally crazy to fall back test map.c
    const char *mapPath = "./testmap.bin";  // intentionally crazy to fall back test map.c

    if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
        mapPath = argv[1];
    }

    if (!rc3dLoadMapBinary(mapPath)) {
        printf("Failed to load map: %s\n", mapPath);
        printf("Using built-in map instead\n");
    }

    if (!loadTextureFrames(forcefield, forcefieldFrames, 4) ||
        !loadTextureFrames(water, waterFrames, 4))
    {
        freeTextureFrames(forcefield, 4);
        freeTextureFrames(water, 4);
        EndSDL2Session();
        fprintf(stderr, "Failed to allocate animated texture buffers\n");
        return 1;
    }
    
    rc3dInit();
    rc3dPreparePalette();
    rc3dLightRange(1.0f, 0.75f, 6.0f);
    rc3dSetDrawDistance(32.0f);

    SDL_ShowCursor(SDL_DISABLE);

    
    uint32_t lastTicks = SDL_GetTicks();

    int pendingMouseDx = 0;
    int mousePointerMode = 1;
    int pointerX = SCREEN_W / 2;
    int pointerY = SCREEN_H / 2;
    int hoveredWallIndex = -1;

    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_WarpMouseInWindow(
        sdl_win,
        screenToWindowCoord(pointerX, SCREEN_W),
        screenToWindowCoord(pointerY, SCREEN_H));

    //rc3dSetViewport(0, 60, 360, 200);
    randSeed(12345);

    do{
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            }

            if ((e.type == SDL_KEYDOWN) && (e.key.repeat == 0)){
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    running = 0;
            }

            if (e.type == SDL_MOUSEMOTION) {
                if (mousePointerMode) {
                    pointerX = windowToScreenCoord(e.motion.x, SCREEN_W);
                    pointerY = windowToScreenCoord(e.motion.y, SCREEN_H);
                } else {
                    pendingMouseDx += e.motion.xrel;
                }
            }

            if (e.type == SDL_MOUSEBUTTONDOWN){
                if (e.button.button == SDL_BUTTON_RIGHT) {
                    if (mousePointerMode) {
                        pendingMouseDx = 0;
                        pointerX = windowToScreenCoord(e.button.x, SCREEN_W);
                        pointerY = windowToScreenCoord(e.button.y, SCREEN_H);
                        mousePointerMode = 0;
                        hoveredWallIndex = -1;
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                    }
                    continue;
                }

                if (mousePointerMode && e.button.button == SDL_BUTTON_LEFT) {
                    int wallIndex = -1;

                    pointerX = windowToScreenCoord(e.button.x, SCREEN_W);
                    pointerY = windowToScreenCoord(e.button.y, SCREEN_H);

                    if (rc3dGetClickableWallAtScreen(pointerX, pointerY, &wallIndex)) {
                        rc3dActivateClickableWall(wallIndex);
                        printf("wall number %d was clicked\n", wallIndex);
                    }
                }
            }

            if (e.type == SDL_MOUSEBUTTONUP) {
                if ((e.button.button == SDL_BUTTON_RIGHT) && !mousePointerMode) {
                    pendingMouseDx = 0;
                    mousePointerMode = 1;
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    SDL_WarpMouseInWindow(
                        sdl_win,
                        screenToWindowCoord(pointerX, SCREEN_W),
                        screenToWindowCoord(pointerY, SCREEN_H));
                    continue;
                }
            }
        }

        int gFrame = updateFPS();

        uint32_t nowTicks = SDL_GetTicks();
        float dt = (float)(nowTicks - lastTicks) / 1000.0f;
        int mouseDx = mousePointerMode ? 0 : pendingMouseDx;
        const uint8_t *keys = SDL_GetKeyboardState(NULL);
        static float nframeT[2] = {
            0.250f,
            0.150f
        };
        static float nextFrame[2] = {
            0.0f,
            0.0f
        };
        static int ffFrame[2] = {0,0};

        if (mousePointerMode) {
            int mouseWindowX = 0;
            int mouseWindowY = 0;

            SDL_GetMouseState(&mouseWindowX, &mouseWindowY);
            pointerX = windowToScreenCoord(mouseWindowX, SCREEN_W);
            pointerY = windowToScreenCoord(mouseWindowY, SCREEN_H);
        }

        if(gFrame){
            lastTicks = nowTicks;
            pendingMouseDx = 0;


            doCustomGameLogic();

            rc3dUpdate(dt, keys, mouseDx);

            for(int tfrm = 0; tfrm < 2; tfrm++){
                nextFrame[tfrm] += dt;
                if(nextFrame[tfrm] > nframeT[tfrm]){
                    nextFrame[tfrm] = 0.0f;
                    ffFrame[tfrm] ++;
                    if(ffFrame[tfrm] > 3){
                        ffFrame[tfrm] = 0;
                    }
                    if(tfrm == 0) copyTextureToTexture(forcefield[ffFrame[tfrm]].pix, rc3d_GetTexturePtr(15), RC3D_TEX_SIZE, RC3D_TEX_SIZE);
                    if(tfrm == 1) copyTextureToTexture(water[ffFrame[tfrm]].pix, rc3d_GetTexturePtr(5), RC3D_TEX_SIZE, RC3D_TEX_SIZE);
                }
            }            

            shiftTexture(4, TEXSHIFT_UP);
            shiftTextureFX(4, TEXSHIFT_SINOUSSX | TEXSHIFT_SINOUSCY, 12.0f, 12.0f, 2.0f, 1.0f, dt);
            static float bum = 0.0f;
            bum += (0.10 * dt);
            if(bum>1.0f) bum = 0.0f;
            rc3dSetSectorWallTextureOffset(14, bum,0);
        
            rc3dRender();

            hoveredWallIndex = -1;
            if (mousePointerMode) {
                rc3dGetClickableWallAtScreen(pointerX, pointerY, &hoveredWallIndex);
                drawMousePointerCursor(pointerX, pointerY, hoveredWallIndex >= 0);
            }
        }
            
        if(!gFrame){
            drawFPSCounter();
            screenupdate();
        }
    }while (running) ;

    freeTextureFrames(forcefield, 4);
    freeTextureFrames(water, 4);
    EndSDL2Session();
    printf("Dead cool wasnt it!!?\n");
    return 0;
}
