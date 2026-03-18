#include <SDL2/SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>



// basic includes
#include "gfx.h"

#include "sb3d/3dloader.h"
#include "sb3d/sb3d.h"



//#include "worldspace.h"
uint8_t toggleLightSun     = 0,
        toggleLightShip    = 1,
        toggleWireFrame    = 0,
        toggleZOrdering    = 0,
        toggleflatMode     = 0,   // flat mode, use non dithered shaded, triangles
        toggleTwoshadeMode = 0,   // this shades in full dither, but only uses the base colour selected, and black (colour 16)
        togglewireframe    = 0;   // 
;



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
        
        sprintf(buf, "TRIS: %d",  getRenderTriCount());

        drawText(1, 15, buf, 16);
        drawText(1, 17, buf, 16);
        drawText(0, 16, buf, 16);
        drawText(2, 16, buf, 16);
        drawText(1, 16, buf, 15);

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


Vec3 Light1Pos = {0.0f, 0.0f, 0.0f};
uint8_t HosS, HosH, HosG, SeaDots;
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

    //SDL_Renderer *ren = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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

    clut[0] = 0xff000000;
    //clut[16] = 0xff2B5792;
    clut[16] = 0xff000000;
    


    /* 4 darker shades after the base */
    float shades[5] = {1.0f, 0.75f, 0.55f, 0.35f, 0.20f};
    //float shades[5] = {1.0f, 1.20f, 1.35f, 1.55f, 1.75f};

    uint32_t lightTarget = clut[16]; // e.g., warm sunlight tint

    // replace your old nested loop
    buildLightingCLUT(clut, baseColors, 16, lightTarget, shades);

///[ WORLD 3D SETUP TEST ]////////////////////////////////////////////////////////////////////

    setDefaultRenderMode();
    Camera cam = cameraCreate();
    cameraSetRange(&cam, 0.01, 5000.0f);
    cameraNormalize(&cam);

    lightsClear();

    uint8_t Camlightid = addPointLight((Vec3){  0.0f, 0.0f, 0.0f }, 0.01f, 1);
    uint8_t SunlightId = addDirectionalLight((Vec3){ -1.0, -0.50f, 0.30}, 0.10, 1);
    uint8_t fighterLightId = addPointLight(vec3(0,0,0), 1.0f, 1);
    lightSetRanges(fighterLightId, 100.0f, 320.0f, 530.0f);


    worldClear();


    Mesh shipMesh; loadMeshSB3D("shipv1.sb3d", &shipMesh, 10.0f);
    Mesh shipTestMesh = copyMesh(&shipMesh);
    int shipTest = entityWorldSpawn(&shipTestMesh, vec3(-135,42, -200) );

    Mesh ShipYard[3]; int shipYardID[3];
    for(int i = 0; i < 3; i++){
        ShipYard[i] = copyMesh(&shipMesh);
        shipYardID[i] = entityWorldSpawn(&ShipYard[i], vec3(-245, 42, 100 + (i * 100)));
        entityRotation(shipYardID[i], degrees(90), 0, 0);
    }




    Mesh houseMesh;
    loadMeshSB3D("house1.sb3d", &houseMesh, 50.0f);
    int house0 = entityWorldSpawn(&houseMesh, vec3(280, 32, 650) );
    entityTurnLocal(house0, -2.32,0,0);

    Mesh ipenergyMesh;
    loadMeshSB3D("ip_energy.sb3d", &ipenergyMesh, 20.0f);
    int ipenergy0 = entityWorldSpawn(&ipenergyMesh, vec3( 300, 32, 320) );

    Mesh ipbadguy1Mesh;
    loadMeshSB3D("ip_badguy1.sb3d", &ipbadguy1Mesh, 20.0f);
    int ipbadguy1 = entityWorldSpawn(&ipbadguy1Mesh, (Vec3){ 280, 32, 420});

    Mesh carrierMesh;
    loadMeshSB3D("carrier.sb3d", &carrierMesh, 50.0f);
    int carrier0 = entityWorldSpawn(&carrierMesh, (Vec3) { 1950, 0, -100});

    Mesh recogMesh;
    loadMeshSB3D("recogniser.sb3d", &recogMesh, 100.0f);
    int recog0 = entityWorldSpawn(&recogMesh, vec3(-200, 100, 0));


    Mesh islandMesh;
    loadMeshSB3D("islandx.sb3d", &islandMesh, 200.0f);
    int island0 = entityWorldSpawn(&islandMesh, vec3(012, 0, 0));

    Mesh textMesh;
    loadMeshSB3D("text.sb3d", &textMesh, 50.0f);
    int text0 = entityWorldSpawn(&textMesh, vec3(0,200,0));
    meshSetMaterial(&textMesh, 0.00f, 0.55f, 0.0f, 1.50f, 64.0f);   // shiny metal




       
    ///[ END WORLD 3D SETUP TEST ]////////////////////////////////////////////////////////////////



///[ END WORLD 3D SETUP TEST ]////////////////////////////////////////////////////////////////
    static int nextLogicUpdate = 0;

    float mouseYaw = 0.0f;
    float mousePitch = 0.0f;
    float mouseSensitivity = 0.0015f;
    int mouseLookEnabled = 1;

    uint8_t mbLeft=0, mbRight=0;

    cameraSetPosition(&cam, vec3(0,50,0));
    cameraRotate(&cam, 0,0,0);

    toggleLightSun = 1;
    toggleLightShip = 0;
    toggleflatMode = 1;
    enableFlatMode(toggleflatMode);

    // sky colour
    SeaDots = 2;
    HosS = 9;
    HosG = 59;
    HosH = 43;


    SDL_SetRelativeMouseMode(SDL_TRUE);
    while (running) {
        //uint32_t frame_start = SDL_GetTicks();

        //mbLeft  = 0;
        //mbRight = 0;
        // ---- 1) process all pending events (NO WAIT here) ----
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if ((e.type == SDL_KEYDOWN) && (e.key.repeat == 0) && (e.key.keysym.sym == SDLK_ESCAPE))
                running = 0;

            if((e.type == SDL_MOUSEBUTTONDOWN)){
                if(e.button.button == SDL_BUTTON_LEFT) mbLeft = 1;
            }

            if((e.type == SDL_MOUSEBUTTONUP)){
                if(e.button.button == SDL_BUTTON_LEFT) mbLeft = 0;
            }

            if ((e.type == SDL_KEYDOWN) && (e.key.repeat == 0)) {
                switch (e.key.keysym.sym) {
                    case SDLK_KP_0:
                        toggleWireFrame = 1 - toggleWireFrame;
                        enableWireFrame(toggleWireFrame);
                        break;

                    case SDLK_KP_1:
                        toggleLightShip = 1 - toggleLightShip;
                        break;
                    
                    case SDLK_KP_2:
                        toggleLightSun = 1 - toggleLightSun;

                        if(toggleLightSun){
                            HosS = 9;   // sky
                            HosG = 59;  // horizon
                            HosH = 43;  // Sea
                            SeaDots = 2;
                        } else {
                            HosS = 17;
                            HosG = 18;
                            HosH = 20;
                            SeaDots = 23;
                        }


                        break;

                    case SDLK_KP_4:
                        toggleZOrdering = 1 - toggleZOrdering;
                        enableZOrdering(toggleZOrdering);
                        break;

                    case SDLK_KP_5:
                        toggleflatMode = 1 - toggleflatMode;
                        enableFlatMode(toggleflatMode);
                        break;

                    case SDLK_KP_6:
                        toggleTwoshadeMode = 1 - toggleTwoshadeMode;
                        enableTwoShade(toggleTwoshadeMode);
                        break;
                    
                    default:
                        break;
                }
            }


            if(e.type == SDL_MOUSEMOTION && mouseLookEnabled){
                mouseYaw   -= (float)e.motion.xrel * mouseSensitivity;
                mousePitch -= (float)e.motion.yrel * mouseSensitivity;
            }
        }

        if(nextLogicUpdate)
        {
            nextLogicUpdate = 0;
        
            ///////////////////////////// RENDER 3D world ////////////////////////////////////

            float moveSpeed = 5.40f;
            float turnSpeed = 0.030f;

            const Uint8 *keys = SDL_GetKeyboardState(NULL);


            // navigational test

            // air craft style motion #######################################################
            float rx = 0.0f;
            float rz = 0.0f;
            float ryGlobal = 0.0f;

            // thrust
            if (keys[SDL_SCANCODE_W] || mbLeft) cameraMove(&cam, 0.0f, 0.0f,  moveSpeed);
            if (keys[SDL_SCANCODE_S]) cameraMove(&cam, 0.0f, 0.0f, -moveSpeed);
            if (keys[SDL_SCANCODE_A]) cameraMove(&cam, -moveSpeed, 0, 0);
            if (keys[SDL_SCANCODE_D]) cameraMove(&cam,  moveSpeed, 0, 0);
            if (keys[SDL_SCANCODE_R]) cameraMove(&cam, 0, moveSpeed, 0);
            if (keys[SDL_SCANCODE_F]) cameraMove(&cam, 0, -moveSpeed, 0);


            // keyboard yaw
            if (keys[SDL_SCANCODE_Q]) ryGlobal += turnSpeed;
            if (keys[SDL_SCANCODE_E]) ryGlobal -= turnSpeed;

            // keyboard roll
            if (keys[SDL_SCANCODE_LEFT])  rz += turnSpeed;
            if (keys[SDL_SCANCODE_RIGHT]) rz -= turnSpeed;

            // keyboard pitch
            if (keys[SDL_SCANCODE_UP])    rx += turnSpeed;
            if (keys[SDL_SCANCODE_DOWN])  rx -= turnSpeed;

            // mouse adds pitch + roll
            rx += mousePitch;
            rz += mouseYaw;

            // local pitch/roll
            cameraTurn(&cam, rx, 0.0f, rz, 0);

            // optional bank-to-turn assist
            ryGlobal += cam.right.y * 0.02f;

            // global yaw after local update
            if (ryGlobal != 0.0f) {
                cameraTurn(&cam, 0.0f, -ryGlobal, 0.0f, 1);
            }

            // consume mouse deltas
            mouseYaw = 0.0f;
            mousePitch = 0.0f;

            // air craft style motion #######################################################
            if (keys[SDL_SCANCODE_SPACE]) {
                cameraSetPosition(&cam, vec3(0,50,0));
                cameraRotate(&cam, 0,0,0);
            }


            lightEnable(Camlightid, toggleLightShip);
            lightSetIntensity(Camlightid, 2.0f);
            lightSetRanges(Camlightid, 100.0f, 320.0f, 530.0f);

            lightEnable(SunlightId, toggleLightSun);
            lightSetIntensity(SunlightId, 1.0);
            lightSetPosition(Camlightid, cam.pos);
            

            entityTurnLocal(ipenergy0, 0.02f, 0,0);
            // crap AI for one ship
            //////////////////////////
            entityMoveForward(shipTest, 6.3f);//vec3(0,0,0.7f));

            float speed = 0.2;
            entityTurnLocal(carrier0, -0.003f * speed, 0, 0);
            entityMoveForward(carrier0, 6.8f * speed);//vec3(0,0,0.7f));
            

            Vec3 theShipPos = entityGetPosition(shipTest);
            if(theShipPos.z > 2500){
                entitySetPosition(shipTest, vec3(-135,42, -400));
            }

            lightSetPosition(fighterLightId, theShipPos);
            //lightSetRanges(fighterLightId, 100.0f, 320.0f, 530.0f);


            entityLookAt(text0, shipTest, 1);


            Vec3 pv1 = entityLookAt(shipYardID[0], shipTest, 1);
            //entityRotation(shipYardID[0], pv1.y, 0, 0);

            //////////////////////////

            //clearScreen(0);

            resetRenderList();
            drawFakeHorizon(&cam, HosS, HosG, HosH, 0);
            drawFakeHorizonDots(&cam, SeaDots, 128, 0, 110);
            //drawFakeHorizonGrid(&cam, 2, 128, 0.0f, 32);
            Render3D(&cam);
            ///////////////////////////// RENDER 3D world ////////////////////////////////////

        }
        
        uint8_t colInd = 0;
        for(int gy = 0; gy < 16; gy ++){
            for(int gx = 0; gx < 16; gx ++){
                drawRect((SCREEN_W - 4 * 16) + gx * 4, gy * 4, 4, 4, colInd);
                colInd ++;
            }
        }
        

        nextLogicUpdate = updateFPS();

        videoMemToScreen();
        SDL_UpdateTexture(tex, NULL, pb, SCREEN_W * (int)sizeof(uint32_t));
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);

        //SDL_GL_SetSwapInterval(1);
        
    }


    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(sdl_win);
    SDL_Quit();
    printf("END OF PLAY\n");

    return 0;
}

