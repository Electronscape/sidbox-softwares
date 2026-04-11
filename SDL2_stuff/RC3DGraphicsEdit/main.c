#include <SDL2/SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>





// basic includes
#include "gfx.h"
#include "rc3d/rc3dedit.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#define FPS        60
#define A_FPS_MS   (1000 / FPS)

int tmr1;
//static uint8_t mouseAction = 0;




int updateFPS() {
    static uint32_t fpsTimer = 0;
    static uint32_t lastFrameTime = 0;
    static int frameCount = 0;        // frames actually rendered
    static int uncappedCount = 0;     // every loop iteration
    static int fps = 0;
    static int uncappedFPS = 0;

    uint32_t now = SDL_GetTicks();
    uncappedCount++; // count every loop iteration

    // Update FPS every second
    if (now - fpsTimer >= 1000) {
        fps = frameCount;
        uncappedFPS = uncappedCount;

        frameCount = 0;
        uncappedCount = 0;
        fpsTimer = now;
    }

        char buf[32];
        sprintf(buf, "FPS: %d (Uncapped: %d)", fps, uncappedFPS);
        drawText(0, 1, buf, 16);
        drawText(2, 1, buf, 16);
        drawText(1, 0, buf, 16);
        drawText(1, 2, buf, 16);
        drawText(1, 1, buf, 15);
        
        
    // Check if enough time has passed for next frame
    const uint32_t targetMs = A_FPS_MS; // e.g., 1000 / 144
    if (now - lastFrameTime >= targetMs) {
        lastFrameTime += targetMs; // advance last frame time
        frameCount++;              // count as a rendered frame

        // Display FPS info

        return 1; // time to update logic/render
    }


    
    return 0; // skip this loop iteration for logic
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
        "Raycast world editor V1.0 (SDL2)",
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


void launchRaycastGame(const char *mapPath)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return;
    }
    if (pid == 0) {
        execl("./raycast3ddemo", "./raycast3ddemo", mapPath, (char *)NULL);
        perror("execl failed");
        _exit(127);
    }
}

int exportBinaryMap(const char *path);
void doRunDemoGame();
int rc3dEditConsumeQuitRequest(void);

int main(void)
{
    if (BasicSDL2Setup() != 0) {
        return 1;
    }

    rc3dEditInit();

    int running = 1;
    int forceFirstFrame = 1;
    uint32_t lastTicks = SDL_GetTicks();

    int lastMouseX = -1;
    int lastMouseY = -1;

    while (running) {
        uint32_t nowTicks = SDL_GetTicks();
        float dt = (float)(nowTicks - lastTicks) / 1000.0f;
        lastTicks = nowTicks;

        int mouseWheelY = 0;
        int dirty = 0;

        SDL_Event e;

        if (SDL_WaitEventTimeout(&e, 8)) {
            do {
                switch (e.type) {
                    case SDL_QUIT:
                        running = 0;
                        break;

                    case SDL_KEYDOWN:
                        if ((e.key.repeat == 0) && (e.key.keysym.sym == SDLK_F10)) {
                            
                        }
                        if ((e.key.repeat == 0) && (e.key.keysym.sym == SDLK_F12)) {
                            //launch the demo map.bin ;)
                            doRunDemoGame();
                        }
                        dirty = 1;
                        break;


                    case SDL_KEYUP:
                        dirty = 1;
                        break;

                    case SDL_MOUSEWHEEL:
                        mouseWheelY += e.wheel.y;
                        dirty = 1;
                        break;

                    case SDL_MOUSEMOTION:
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_MOUSEBUTTONUP:
                        dirty = 1;
                        break;

                    default:
                        break;
                }
            } while (SDL_PollEvent(&e));
        }

        if (forceFirstFrame) {
            dirty = 1;
            forceFirstFrame = 0;
        }

        const uint8_t *keys = SDL_GetKeyboardState(NULL);
        if (rc3dEditConsumeQuitRequest()) {
            running = 0;
        }

        if (keys[SDL_SCANCODE_F] || keys[SDL_SCANCODE_G] ||
            keys[SDL_SCANCODE_C] || keys[SDL_SCANCODE_V] ||
            keys[SDL_SCANCODE_J] || keys[SDL_SCANCODE_K] ||
            keys[SDL_SCANCODE_N] || keys[SDL_SCANCODE_M] ||
            keys[SDL_SCANCODE_Q] || keys[SDL_SCANCODE_E]) {
            dirty = 1;
        }

        int mouseX, mouseY;
        uint32_t mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);

        if (mouseX != lastMouseX || mouseY != lastMouseY) {
            dirty = 1;
            lastMouseX = mouseX;
            lastMouseY = mouseY;
        }

        if (mouseButtons != 0) {
            dirty = 1;
        }

        dirty += rc3dGuiCheckDirty();

        if (dirty) {
            rc3dEditUpdate(dt, keys, mouseX, mouseY, mouseButtons, mouseWheelY);
            rc3dEditRender();

            videoMemToScreen();
            SDL_UpdateTexture(tex, NULL, pb, SCREEN_W * (int)sizeof(uint32_t));
            SDL_RenderCopy(ren, tex, NULL, NULL);
            SDL_RenderPresent(ren);
        }
    }

    EndSDL2Session();
    printf("Did you have fun??\n");
    return 0;
}








