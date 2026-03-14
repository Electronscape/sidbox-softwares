#include <SDL2/SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>



// basic includes
#include "gfx.h"

#include "sb3d/sb3d.h"



//#include "worldspace.h"



#define FPS        144
#define A_FPS_MS   (1000 / FPS)

int tmr1;
//static uint8_t mouseAction = 0;

#define ZOOM 2

#define COLOUR_OFFSET   32

Vec3 Light1Pos = {0.0f, 0.0f, 0.0f};
//static uint32_t indexTmrTest = 0;

int main(void) {
    //// HOST STARTUP
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *sdl_win = SDL_CreateWindow(
        "3D Balls-up world Host (SDL2)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W * ZOOM, SCREEN_H * ZOOM, 0
    );
    if (!sdl_win) {
        fprintf(stderr, "CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture  *tex = SDL_CreateTexture (ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdl_win);
        SDL_Quit();
        return 1;
    }

    if (!tex) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(sdl_win);
        SDL_Quit();
        return 1;
    }
    
    int running = 1;
    const int tick_ms = A_FPS_MS;
    uint32_t last = SDL_GetTicks();

    // basic start screen direct to screen memory
    for(int32_t i = 0; i < SCREEN_H * SCREEN_W; i++){
        fb[i] = 80;
    }

    clearScreen(0);
    // base colour
    /* 16 base colours */
    uint32_t baseColors[16] = {
        0xFF5516e3, 0xFFFFFFFF, 0xFFFF0000, 0xFF00FF00,
        0xFF0000FF, 0xFFFFFF00, 0xFFFF00FF, 0xFF00FFFF,
        0xFF808080, 0xFFFF8000, 0xFF8000FF, 0xFF0080FF,
        0xFF80FF00, 0xFFFF0080, 0xFF00FF80, 0xFFC0C0C0
    };


    /* 4 darker shades after the base */
    float shades[5] = {1.0f, 0.75f, 0.55f, 0.35f, 0.20f};

    /* palette starts at 16 */
    for (int ci = 0; ci < 16; ci++) {
        for (int s = 0; s < 5; s++) {
            //clut[16 + (ci * 5) + s] = darken(baseColors[ci], shades[s]);
            clut[COLOUR_OFFSET + (s * 16) + ci] = darken(baseColors[ci], shades[s]);
        }
    }


    // test box, this works can now comment out
    //drawLine(100, 100, 300, 100, 2);
    //drawLine(300, 100, 300, 300, 3);
    //drawLine(300, 300, 100, 300, 4);
    //drawLine(100, 300, 100, 100, 5);


///[ WORLD 3D SETUP TEST ]////////////////////////////////////////////////////////////////////

    Camera cam = {
        .pos = { 0.0f, 40.0f, -100.0f },
        .right =   { 1.0f, 0.0f, 0.0f },
        .up =      { 0.0f, 1.0f, 0.0f },
        .forward = { 0.0f, 0.0f, 1.0f }
    };



    normalizeCamera(&cam);

    /* simple box building mesh */
    Vec3 buildingVerts[] = {
        {-20.0f,   0.0f, -20.0f},   // 0
        { 20.0f,   0.0f, -20.0f},   // 1
        { 20.0f,   0.0f,  20.0f},   // 2
        {-20.0f,   0.0f,  20.0f},   // 3

        {-20.0f,  60.0f, -20.0f},   // 4
        { 20.0f,  60.0f, -20.0f},   // 5
        { 20.0f,  60.0f,  20.0f},   // 6
        {-20.0f,  60.0f,  20.0f}    // 7
    };

    Edge buildingEdges[] = {
        {0,1}, {1,2}, {2,3}, {3,0},   /* bottom */
        {4,5}, {5,6}, {6,7}, {7,4},   /* top */
        {0,4}, {1,5}, {2,6}, {3,7}    /* verticals */
    };

    Tri buildingTris[] = {
        {0,1,2, COLOUR_OFFSET + 2}, {0,2,3, COLOUR_OFFSET + 2},   /* bottom */
        {4,6,5, COLOUR_OFFSET + 3}, {4,7,6, COLOUR_OFFSET + 3},   /* top */

        {0,4,5, COLOUR_OFFSET + 4}, {0,5,1, COLOUR_OFFSET + 4},   /* side 1 */
        {1,5,6, COLOUR_OFFSET + 5}, {1,6,2, COLOUR_OFFSET + 5},   /* side 2 */
        {2,6,7, COLOUR_OFFSET + 2}, {2,7,3, COLOUR_OFFSET + 2},   /* side 3 */
        {3,7,4, COLOUR_OFFSET + 3}, {3,4,0, COLOUR_OFFSET + 3}    /* side 4 */
    };

    Mesh buildingMesh = {
        .verts = buildingVerts,
        .vertCount = 8,
        .edges = buildingEdges,
        .edgeCount = 12,
        .tris = buildingTris,
        .triCount = 12
    };




    Vec3 building2Verts[] = {
        /* main base */
        {-30.0f,   0.0f, -30.0f},   // 0
        { 30.0f,   0.0f, -30.0f},   // 1
        { 30.0f,   0.0f,  30.0f},   // 2
        {-30.0f,   0.0f,  30.0f},   // 3

        {-30.0f,  50.0f, -30.0f},   // 4
        { 30.0f,  50.0f, -30.0f},   // 5
        { 30.0f,  50.0f,  30.0f},   // 6
        {-30.0f,  50.0f,  30.0f},   // 7

        /* upper tower */
        {-15.0f,  50.0f, -15.0f},   // 8
        { 15.0f,  50.0f, -15.0f},   // 9
        { 15.0f,  50.0f,  15.0f},   // 10
        {-15.0f,  50.0f,  15.0f},   // 11

        {-15.0f,  85.0f, -15.0f},   // 12
        { 15.0f,  85.0f, -15.0f},   // 13
        { 15.0f,  85.0f,  15.0f},   // 14
        {-15.0f,  85.0f,  15.0f}    // 15
    };
    Edge building2Edges[] = {
        /* base box */
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7},

        /* upper tower */
        {8,9}, {9,10}, {10,11}, {11,8},
        {12,13}, {13,14}, {14,15}, {15,12},
        {8,12}, {9,13}, {10,14}, {11,15}
    };
    
    Tri building2Tris[] = {
        /* ===== main base ===== */

        /* front (-z) */
        {0,5,1, COLOUR_OFFSET + 2}, {0,4,5, COLOUR_OFFSET + 2},

        /* back (+z) */
        {3,2,6, COLOUR_OFFSET + 3}, {3,6,7, COLOUR_OFFSET + 3},

        /* left (-x) */
        {0,3,7, COLOUR_OFFSET + 4}, {0,7,4, COLOUR_OFFSET + 4},

        /* right (+x) */
        {1,5,6, COLOUR_OFFSET + 5}, {1,6,2, COLOUR_OFFSET + 5},

        /* top of main base (around tower) */
        {4,8,9,  COLOUR_OFFSET +3},  {4,9,5,   COLOUR_OFFSET +3},    /* front strip */
        {5,9,10, COLOUR_OFFSET +3},  {5,10,6,  COLOUR_OFFSET +3},   /* right strip */
        {7,6,10, COLOUR_OFFSET +3},  {7,10,11, COLOUR_OFFSET +3},  /* back strip */
        {4,7,11, COLOUR_OFFSET +3},  {4,11,8,  COLOUR_OFFSET +3},   /* left strip */

        /* bottom */
        {0,1,2, COLOUR_OFFSET + 2}, {0,2,3, COLOUR_OFFSET + 2},

        /* ===== upper tower ===== */

        /* front (-z) */
        {8,13,9,   COLOUR_OFFSET + 4}, {8,12,13,  COLOUR_OFFSET + 4},

        /* back (+z) */
        {11,10,14, COLOUR_OFFSET + 5}, {11,14,15, COLOUR_OFFSET + 5},

        /* left (-x) */
        {8,11,15,  COLOUR_OFFSET + 2}, {8,15,12,  COLOUR_OFFSET + 2},

        /* right (+x) */
        {9,13,14,  COLOUR_OFFSET + 3}, {9,14,10,  COLOUR_OFFSET + 3},

        /* roof */
        {12,15,14, COLOUR_OFFSET + 1}, {12,14,13, COLOUR_OFFSET + 1}
    };


    Mesh building2Mesh = {
        .verts = building2Verts,
        .vertCount = 16,
        .edges = building2Edges,
        .edgeCount = 24,
        .tris = building2Tris,
        .triCount = 26,
        .boundsRadius = 0.0f
    };


    buildingMesh.boundsRadius = meshComputeBoundsRadius(&buildingMesh);
    building2Mesh.boundsRadius = meshComputeBoundsRadius(&building2Mesh);



    clearLights();

    uint8_t lightid = addPointLight((Vec3){  0.0f, 0.0f, 0.0f }, 1.0f, 1);
    uint8_t Camlightid = addPointLight((Vec3){  0.0f, 0.0f, 0.0f }, 2.0f, 1);

    //uint8_t sunlight = addDirectionalLight((Vec3){ 0.6f, -1.0f, 0.3f }, 0.65f, 1);
    //
    //addPointLight((Vec3){ 0.0f, 80.0f, 120.0f }, 1.8f, 1);


    /*
    Entity worldEntities[] = {
        entityCreate(&buildingMesh,  (Vec3){   0.0f, 0.0f, 200.0f }),
        entityCreate(&buildingMesh,  (Vec3){ 120.0f, 0.0f, 320.0f }),
        entityCreate(&buildingMesh,  (Vec3){-140.0f, 0.0f, 420.0f }),
        entityCreate(&building2Mesh, (Vec3){ 220.0f, 0.0f, 260.0f })
    };
    */
   worldClear();

    int box0 = entityCreate(&buildingMesh,  (Vec3){   0.0f, 0.0f, 200.0f });
    int box1 = entityCreate(&buildingMesh,  (Vec3){ 120.0f, 0.0f, 320.0f });
    int box2 = entityCreate(&buildingMesh,  (Vec3){-140.0f, 0.0f, 420.0f });
    int box3 = entityCreate(&building2Mesh, (Vec3){ 220.0f, 0.0f, 260.0f });

    //const int worldEntityCount = sizeof(worldEntities) / sizeof(worldEntities[0]);

    ///[ END WORLD 3D SETUP TEST ]////////////////////////////////////////////////////////////////



///[ END WORLD 3D SETUP TEST ]////////////////////////////////////////////////////////////////

    
    while (running) {
        uint32_t frame_start = SDL_GetTicks();

        // ---- 1) process all pending events (NO WAIT here) ----
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if ((e.type == SDL_KEYDOWN) && (e.key.repeat == 0) && (e.key.keysym.sym == SDLK_ESCAPE))
                running = 0;

            switch (e.type) {

                case SDL_TEXTINPUT: {
                    // UTF-8 string; you only want ASCII for now
                    const char *s = e.text.text;
                    for (int i = 0; s[i]; i++) {
                        unsigned char c = (unsigned char)s[i];
                        if (c >= 32 && c <= 126) {
                            //SBOSIO_KeyboardInterface(c, 0);
                            // a keyboard interface
                        }
                    }
                } break;
                

                case SDL_KEYDOWN: {
                    //if (e.key.repeat) break;

                    //uint8_t mod = 0;
                    //SDL_Keymod m = SDL_GetModState();
                    /*
                    if (m & KMOD_SHIFT) mod |= KM_SHIFT;
                    if (m & KMOD_CTRL)  mod |= KM_CTRL;
                    if (m & KMOD_ALT)   mod |= KM_ALT;

                    switch (e.key.keysym.sym) {
                        case SDLK_BACKSPACE: SBOSIO_KeyboardInterface(KEY_BACKSPACE, mod); break;
                        case SDLK_DELETE:    SBOSIO_KeyboardInterface(KEY_DEL, mod);       break;
                        case SDLK_LEFT:      SBOSIO_KeyboardInterface(KEY_LEFT, mod);      break;
                        case SDLK_RIGHT:     SBOSIO_KeyboardInterface(KEY_RIGHT, mod);     break;
                        case SDLK_UP:        SBOSIO_KeyboardInterface(KEY_UP, mod);        break;
                        case SDLK_DOWN:      SBOSIO_KeyboardInterface(KEY_DOWN, mod);      break;
                        case SDLK_HOME:      SBOSIO_KeyboardInterface(KEY_HOME, mod);      break;
                        case SDLK_END:       SBOSIO_KeyboardInterface(KEY_END, mod);       break;
                        case SDLK_RETURN:
                        case SDLK_KP_ENTER:  SBOSIO_KeyboardInterface(KEY_ENTER, mod);     break;
                        case SDLK_TAB:       SBOSIO_KeyboardInterface(KEY_TAB, mod);       break;
                        case SDLK_ESCAPE:    SBOSIO_KeyboardInterface(KEY_ESC, mod);       break;
                        default: break;
                    }
                    */
                } break;
            }
        }

        // ---- 2) your per-frame stuff ----
        //pump_mouse_sdl();

        uint32_t now = SDL_GetTicks();
        uint32_t dt  = now - last;
        last = now;

        tmr1 += dt;
        if (tmr1 >= 1000) {
            tmr1 = 0;
            
        }



    ///////////////////////////// RENDER 3D world ////////////////////////////////////

        float moveSpeed = 0.30f;
        float turnSpeed = 0.002f;

        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        if (keys[SDL_SCANCODE_W]) {
            cam.pos = vec3Add(cam.pos, vec3Scale(cam.forward, moveSpeed));
        }

        if (keys[SDL_SCANCODE_S]) {
            cam.pos = vec3Add(cam.pos, vec3Scale(cam.forward, -moveSpeed));
        }

        if (keys[SDL_SCANCODE_A]) {
            cam.pos = vec3Add(cam.pos, vec3Scale(cam.right, -moveSpeed));
        }

        if (keys[SDL_SCANCODE_D]) {
            cam.pos = vec3Add(cam.pos, vec3Scale(cam.right, moveSpeed));
        }

        if (keys[SDL_SCANCODE_R]) {
            cam.pos = vec3Add(cam.pos, vec3Scale(cam.up, moveSpeed));
        }

        if (keys[SDL_SCANCODE_F]) {
            cam.pos = vec3Add(cam.pos, vec3Scale(cam.up, -moveSpeed));
        }

        if (keys[SDL_SCANCODE_Q]) turnCameraGlobal(&cam, -turnSpeed, 0.0f, 0.0f);
        if (keys[SDL_SCANCODE_E]) turnCameraGlobal(&cam,  turnSpeed, 0.0f, 0.0f);

        if (keys[SDL_SCANCODE_1]) turnCameraLocal(&cam, -turnSpeed, 0.0f, 0.0f);
        if (keys[SDL_SCANCODE_3]) turnCameraLocal(&cam,  turnSpeed, 0.0f, 0.0f);

        if (keys[SDL_SCANCODE_Z]) turnCameraGlobal(&cam, 0.0f, 0.0f, -0.001f);
        if (keys[SDL_SCANCODE_C]) turnCameraGlobal(&cam, 0.0f, 0.0f,  0.001f);


        if (keys[SDL_SCANCODE_KP_8]) {
            Light1Pos.z += 1;
            setLightPosition(lightid, Light1Pos);
        }

        if (keys[SDL_SCANCODE_KP_2]) {
            Light1Pos.z -= 1;
            setLightPosition(lightid, Light1Pos);
        }

        if (keys[SDL_SCANCODE_KP_4]) {
            Light1Pos.x -= 1;
            setLightPosition(lightid, Light1Pos);
        }

        if (keys[SDL_SCANCODE_KP_6]) {
            Light1Pos.x += 1;
            setLightPosition(lightid, Light1Pos);
        }

        if (keys[SDL_SCANCODE_SPACE]) {
            cam.pos = (Vec3){ 0.0f, 40.0f, -100.0f };
            cam.right =   (Vec3){ 1.0f, 0.0f, 0.0f };
            cam.up =      (Vec3){ 0.0f, 1.0f, 0.0f };
            cam.forward = (Vec3){ 0.0f, 0.0f, 1.0f };
        }

        clearScreen(0);

        resetRenderList();

        //addDirectionalLight
        static uint8_t lighton;
        uint8_t qflip;

        qflip = rand() % 255;

        if(qflip < 3) lighton = 1;
        if(qflip > 250) lighton = 0;

        lightEnable(0, lighton);
        //lightEnable(0, 0);
        setLightPosition(Camlightid, cam.pos);

        entityMoveForward(box0, 0.1f);
        entityTurnLocal(box0, 0.001f, 0, 0);
        entityMoveRight(box2, -0.1f);

        
        Render3D(&cam);


    ///////////////////////////// RENDER 3D world ////////////////////////////////////

        /*
        uint8_t colInd = 0;
        for(int gy = 0; gy < 16; gy ++){
            for(int gx = 0; gx < 16; gx ++){
                drawRect(gx * 8, gy * 8, 8,8, colInd);
                colInd ++;
            }
        }
        */


        videoMemToScreen();
        SDL_UpdateTexture(tex, NULL, pb, SCREEN_W * (int)sizeof(uint32_t));
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);

        // ---- 3) sleep remainder of frame (THIS is your cap) ----
        uint32_t frame_end = SDL_GetTicks();
        uint32_t elapsed = frame_end - frame_start;

        if (elapsed < tick_ms) {
            //SDL_Delay((uint32_t)(tick_ms - elapsed));
        }
        //SDL_GL_SetSwapInterval(1);
        
    }


    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(sdl_win);
    SDL_Quit();
    printf("END OF PLAY\n");

    return 0;
}

