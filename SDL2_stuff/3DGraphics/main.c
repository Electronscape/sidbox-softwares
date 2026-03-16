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



#define FPS        144
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

    /*
    for (int ci = 0; ci < 16; ci++) {
        for (int s = 0; s < 5; s++) {
            //clut[16 + (ci * 5) + s] = darken(baseColors[ci], shades[s]);
            clut[COLOUR_OFFSET + (s * 16) + ci] = darken(baseColors[ci], shades[s]);
        }
    }
    */


    // test box, this works can now comment out
    //drawLine(100, 100, 300, 100, 2);
    //drawLine(300, 100, 300, 300, 3);
    //drawLine(300, 300, 100, 300, 4);
    //drawLine(100, 300, 100, 100, 5);


///[ WORLD 3D SETUP TEST ]////////////////////////////////////////////////////////////////////

    setDefaultRenderMode();
    Camera cam = {
        .pos = { 0.0f, 40.0f, -100.0f },
        .right =   { 1.0f, 0.0f, 0.0f },
        .up =      { 0.0f, 1.0f, 0.0f },
        .forward = { 0.0f, 0.0f, 1.0f },
        .nearPlane = 0.01f,
        .farPlane = 1500.0f
    };



    normalizeCamera(&cam);

    clearLights();

    uint8_t lightid = addPointLight((Vec3){  0.0f, 0.0f, 0.0f }, 1.0f, 1);
    uint8_t Camlightid = addPointLight((Vec3){  0.0f, 0.0f, 0.0f }, 1.0f, 1);
    uint8_t SunlightId = addDirectionalLight((Vec3){ -1.0, -0.50f, 0.30}, 0.10, 1);
    //uint8_t SunlightId2 = addDirectionalLight((Vec3){ 1.0, -1.00f, -1.00}, 0.1, 1);

    //uint8_t sunlight = addDirectionalLight((Vec3){ 0.6f, -1.0f, 0.3f }, 0.65f, 1);
    //
    //addPointLight((Vec3){ 0.0f, 80.0f, 120.0f }, 1.8f, 1);

    worldClear();

    Mesh theCylinder = createCylinder(30.0f, 80.0f, 32);
    Mesh boxTemplate = createBox(40.0f, 160.0f, 40.0f); // a pointer so will be shared if you change geometry of this
    Mesh boxTemplate2 = createBox(40.0f, 40.0f, 40.0f);
    Mesh thePlaneBase = createPlane(1000.0f, 1000.0f, 40);
    Mesh theSphere = createSphere(80.0f, 16, 16);
    Mesh theCone = createCone(40, 40, 12);
    Mesh thePyramid = createPyramid(200, 200);
    Mesh theTorsu = createTorus(100, 20, 36, 20);

    Mesh myBoxInst = copyMesh(&boxTemplate);    // create the instance of this object, so changing ITS geometry doesnt affect the source Mesh (boxTemplate)
    Mesh myBoxInst2 = copyMesh(&boxTemplate2);   

    Mesh thePlane = copyMesh(&thePlaneBase);


    Mesh shipMesh;
    loadMeshSB3D("shipv1.sb3d", &shipMesh, 20.0f);
    int ship0 = entityCreate(&shipMesh, (Vec3){ 0.0f, 100.0f, 200.0f });
    meshSetMaterial(&shipMesh, 0.0f, 1.0f, 0.0f, 0.25f, 16.0f);

    Mesh houseMesh;
    loadMeshSB3D("house1.sb3d", &houseMesh, 50.0f);
    int house0 = entityCreate(&houseMesh, (Vec3){ 450, 0, 0});
    //meshSetMaterial(&houseMesh, 0.0f, 1.0f, 0.0f, 0.25f, 16.0f);

    Mesh ipenergyMesh;
    loadMeshSB3D("ip_energy.sb3d", &ipenergyMesh, 20.0f);
    int ipenergy0 = entityCreate(&ipenergyMesh, (Vec3){ 400, 10, 220});

    Mesh ipbadguy1Mesh;
    loadMeshSB3D("ip_badguy1.sb3d", &ipbadguy1Mesh, 20.0f);
    int ipbadguy1 = entityCreate(&ipbadguy1Mesh, (Vec3){ 400, 10, 120});

    Mesh carrierMesh;
    loadMeshSB3D("carrier.sb3d", &carrierMesh, 50.0f);
    int carrier0 = entityCreate(&carrierMesh, (Vec3) { 300, -50, -100});


    Mesh textMesh;
    loadMeshSB3D("text.sb3d", &textMesh, 20.0f);
    int text0 = entityCreate(&textMesh, (Vec3){ 0, 200, 0});
    meshSetMaterial(&textMesh, 0.00f, 0.55f, -0.02f, 1.50f, 64.0f);   // shiny metal
    meshSetMaterial(&theTorsu, 0.00f, 0.55f, 0.0f, 1.50f, 64.0f);   // shiny metal
    meshSetMaterial(&theSphere, 0.00f, 0.55f, 0.0f, 1.50f, 64.0f);

       
    
    int myBoxInstID = entityCreate(&myBoxInst, (Vec3){ 0,0,0});
    
    int cylinder0 = entityCreate(&theCylinder, (Vec3) {-200.0f, 50, 0});
    int plane0 = entityCreate(&thePlane, (Vec3){ 0.0f, -40.0f, 0.0f});
    int sphere0 = entityCreate(&theSphere, (Vec3){   0.0f, 50.0f, 0.0f } );
    int box0 = entityCreate(&myBoxInst2,  (Vec3){   0.0f, 0.0f, 200.0f });
    int box1 = entityCreate(&myBoxInst2,  (Vec3){ 120.0f, 0.0f, 320.0f });
    int box2 = entityCreate(&myBoxInst2,  (Vec3){-140.0f, 0.0f, 420.0f });

    int cone0 = entityCreate(&theCone, (Vec3){200, 50, 0});
    int pyra0 = entityCreate(&thePyramid, (Vec3){0, 30, -200});
    int tor0 = entityCreate(&theTorsu, (Vec3){-200, 100, 200});

    entityColour(plane0, 32 + 11);  
    entityColour(sphere0, 32 + 13);
    entityColour(pyra0, 32 + 4);


    entityColour(tor0, 32 + 5);

    //const int worldEntityCount = sizeof(worldEntities) / sizeof(worldEntities[0]);

    ///[ END WORLD 3D SETUP TEST ]////////////////////////////////////////////////////////////////



///[ END WORLD 3D SETUP TEST ]////////////////////////////////////////////////////////////////
    static int nextLogicUpdate = 0;

    entityTurnLocal(tor0, 0.000f, M_PI/2,0.00f);
    static float waveTime = 0.0f;


    while (running) {
        //uint32_t frame_start = SDL_GetTicks();

        // ---- 1) process all pending events (NO WAIT here) ----
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if ((e.type == SDL_KEYDOWN) && (e.key.repeat == 0) && (e.key.keysym.sym == SDLK_ESCAPE))
                running = 0;

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
        }

        if(nextLogicUpdate)
        {
            nextLogicUpdate = 0;
        
            ///////////////////////////// RENDER 3D world ////////////////////////////////////

            float moveSpeed = 1.20f;
            float turnSpeed = 0.015f;

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

            if (keys[SDL_SCANCODE_Z]) turnCameraGlobal(&cam, 0.0f, 0.0f,  0.01f);
            if (keys[SDL_SCANCODE_C]) turnCameraGlobal(&cam, 0.0f, 0.0f, -0.01f);


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

            //lightEnable(0, lighton);
            lightEnable(0, 0);
            lightEnable(Camlightid, toggleLightShip);
            setLightIntensity(Camlightid, 2.0f);


            lightEnable(SunlightId, toggleLightSun);
            setLightIntensity(SunlightId, 1.0);
            setLightPosition(Camlightid, cam.pos);

            entityMoveForward(box0, 1.0f);
            entityTurnLocal(box0, 0.01f, 0, 0);
            entityMoveRight(box2, -1.0f);

            entityTurnLocal(tor0, 0.000f, 0.00f,0.010f);

            entityTurnLocal(text0, 0.01f, 0, 0);


            //ipenergy0
            //ipbadguy1

            entityTurnLocal(ipenergy0, 0.01f,0.00f,0.00f);


            

            
            static Vec3 enemyPos = {80.0f,0.0f,0.0f}, enemyPosSpeed = {0.2f,0,0};
            enemyPos.x += enemyPosSpeed.x;
            if(enemyPos.x < 80.0f || enemyPos.x > 150.0f) enemyPosSpeed.x = -enemyPosSpeed.x;



            entitySetPosition(ipbadguy1, enemyPos);
            entityTurnLocal(ipbadguy1, 0.03f,0.00f,0.00f);





            waveTime += 0.014f;
            meshResetFromSource(&thePlane, &thePlaneBase);
            meshDeformWavePlaneY(&thePlane, waveTime, 6.0f, 0.020f, 0.020f, 1.5f);

            float t = waveTime;
            for(int f=0; f<theSphere.triCount; f++){
                uint8_t familyBase = 8;   // start column
                uint8_t familySpan = 3;   // use 4 related colours only

                uint8_t family = familyBase +
                    (uint8_t)((sinf(f * 0.21f + t) * 0.5f + 0.5f) * (float)familySpan);

                uint8_t shade  =
                    (uint8_t)((sinf(f * 0.37f - t * 1.7f) * 0.5f + 0.5f) * 4.0f);

                entityColourFace(sphere0, f, COLOUR_OFFSET + family + (shade * 16));
            }
            

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

