#include <SDL2/SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>





// basic includes
#include "gfx.h"
#include "rc3d/rc3d.h"


#define FPS        60

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
    drawText(0, 1, buf, 16);
    drawText(2, 1, buf, 16);
    drawText(1, 0, buf, 16);
    drawText(1, 2, buf, 16);
    drawText(1, 1, buf, 15);
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

int main(int argc, char **argv)
{
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

    
    rc3dInit();
    rc3dPreparePalette();
    rc3dLightRange(2.0f, 1.0f, 6.0f);
    rc3dSetDrawDistance(32.0f);

    SDL_SetRelativeMouseMode(SDL_TRUE);

    
    uint32_t lastTicks = SDL_GetTicks();

    int pendingMouseDx = 0;

    //rc3dSetViewport(0, 60, 360, 200);

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
                pendingMouseDx += e.motion.xrel;
            }

            if (e.type == SDL_MOUSEBUTTONDOWN){
                moveSprite();
            }
        }

        int gFrame = updateFPS();

        uint32_t nowTicks = SDL_GetTicks();
        float dt = (float)(nowTicks - lastTicks) / 1000.0f;
        int mouseDx = pendingMouseDx;
        const uint8_t *keys = SDL_GetKeyboardState(NULL);

        if(gFrame){
            lastTicks = nowTicks;
            pendingMouseDx = 0;
            rc3dUpdate(dt, keys, mouseDx);
        }

        rc3dRender();

        if(!gFrame){
            drawFPSCounter();
            screenupdate();
        }
        //videoMemToScreen();
        //SDL_UpdateTexture(tex, NULL, pb, SCREEN_W * (int)sizeof(uint32_t));
        //SDL_RenderCopy(ren, tex, NULL, NULL);
        //SDL_RenderPresent(ren);
    }while (running) ;

    EndSDL2Session();
    printf("Dead cool wasnt it!!?\n");
    return 0;
}
