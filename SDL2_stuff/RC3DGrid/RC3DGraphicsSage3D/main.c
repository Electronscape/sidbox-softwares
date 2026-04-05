#include <SDL2/SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>





// basic includes
#include "gfx.h"
//#include "rc3d/rc3d.h"


#define FPS        60
#define A_FPS_MS   (1000 / FPS)
int tmr1;

#define M_PI 3.14159265358979323846

//////////////////////////////////////////////////////////////////////////////////


#include "textures/T_00.h"
#include "textures/T_01.h"
#include "textures/T_02.h"

int numText=2;
int numSect=0;
int numWall=0;


typedef struct {
    int w,s,a,d;
    int sl, sr;
    int m;
} keys; keys K;

typedef struct {
    float cos[360];
    float sin[360];
} math; math M;

typedef struct {
    int x, y, z;    // player position
    int a;  // player angle of rotation left right
    int l;  // look up and down
} player; player P;

typedef struct
{
    /* data */
    int x1, y1;
    int x2, y2;
    int c;          // wall colour

    int wt, u, v;   // wall texture and u/v tile
    int shade;
} walls; walls W[256];

typedef struct 
{
    /* data */
    int ws, we; // wall number start / end
    int z1, z2; // height of bottom and top
    int x, y;   // center position for sector
    int d;      // add y distances to sort drawing order.
    int st, ss;

    int c1, c2; // bottom/top colours
    int surf[SCREEN_W]; // hold the points for surfaces
    int surface;        // is there a surface neededs
} sectors; sectors S[128];

typedef struct {
    int w,h;                    // texture width/height
    const unsigned char *name;  // texture name;
} TextureMaps; TextureMaps Textures[64]; // increase for more textures later



void load(void)
{
    FILE *fp = fopen("level.h","r");
    if(fp == NULL){ printf("Error opening level.h\n"); return; }
    int s;

    fscanf(fp,"%i",&numSect);
    for(s=0;s<numSect;s++)
    {
        fscanf(fp,"%i",&S[s].ws);
        fscanf(fp,"%i",&S[s].we);
        fscanf(fp,"%i",&S[s].z1);
        fscanf(fp,"%i",&S[s].z2);
        fscanf(fp,"%i",&S[s].st);
        fscanf(fp,"%i",&S[s].ss);
    }
    fscanf(fp,"%i",&numWall);
    for(s=0;s<numWall;s++)
    {
        fscanf(fp,"%i",&W[s].x1);
        fscanf(fp,"%i",&W[s].y1);
        fscanf(fp,"%i",&W[s].x2);
        fscanf(fp,"%i",&W[s].y2);
        fscanf(fp,"%i",&W[s].wt);
        fscanf(fp,"%i",&W[s].u);
        fscanf(fp,"%i",&W[s].v);
        fscanf(fp,"%i",&W[s].shade);
    }
    //fscanf(fp,"%i %i %i %i %i",&P.x,&P.y,&P.z,&P.a,&P.l);
    fclose(fp);
}

void movePlayer(uint8_t *key){

    if(key[SDL_SCANCODE_Q]) K.a = 1; else K.a = 0;
    if(key[SDL_SCANCODE_E]) K.d = 1; else K.d = 0;

    if(key[SDL_SCANCODE_W]) K.w = 1; else K.w = 0;
    if(key[SDL_SCANCODE_S]) K.s = 1; else K.s = 0;
    if(key[SDL_SCANCODE_A]) K.sl = 1; else K.sl = 0;
    if(key[SDL_SCANCODE_D]) K.sr = 1; else K.sr = 0;

    if(key[SDL_SCANCODE_LSHIFT]) K.m = 1; else K.m = 0;

    if(key[SDL_SCANCODE_RETURN]) {
        printf("loading level\n");
        load();
        printf("loaded: numSect=%d numWall=%d\n", numSect, numWall);
    }

    // forward, back, turn-left, turn-right
    if(K.a == 1 && K.m == 0){ P.a -=3.0f; if(P.a < 0  ) {P.a += 360; }}
    if(K.d == 1 && K.m == 0){ P.a +=3.0f; if(P.a >= 360) {P.a -= 360; }}

    int dx = M.sin[P.a] * 4;
    int dy = M.cos[P.a] * 4;
    if(K.w == 1 && K.m == 0)  { P.x += dx; P.y += dy; };      // turn left
    if(K.s == 1 && K.m == 0)  { P.x -= dx; P.y -= dy; };     // turn right;

    // strafe left / right
    if (K.sl == 1 && K.m == 0) { P.x -= dy; P.y += dx; }
    if (K.sr == 1 && K.m == 0) { P.x += dy; P.y -= dx; }
    
    // 
    if(K.a == 1 && K.m == 1) { P.l -= 1;}
    if(K.d == 1 && K.m == 1) { P.l += 1;}
    if(K.w == 1 && K.m == 1) { P.z += 4;}
    if(K.s == 1 && K.m == 1) { P.z -= 4;}


}



void clipBehindPlayer(int *x1, int *y1, int *z1, int x2, int y2, int z2){
    float da = *y1;
    float db = y2;
    float d = da - db; if (d == 0) d = 1;
    float s = da/(da-db);
    *x1 = *x1 + s*(x2-(*x1));
    *y1 = *y1 + s*(y2-(*y1)); if(*y1==0) *y1=1;
    *z1 = *z1 + s*(z2-(*z1));

}

int dist(int x1, int y1, int x2, int y2){
    int distance = sqrt( (x2-x1) * (x2-x1) + (y2-y1) * (y2-y1));
    return distance;
}

void testTextures(){
    int x,y, t;
    t = 0;
    for(y = 0; y < Textures[t].h; y++){
        for(x = 0; x < Textures[t].w; x++){
            int pixel = y * Textures[t].w + x;
            int col = Textures[t].name[pixel];
            putPixel(x, 100+y, col);

        }
    }
}

void floors(){
    int x,y;
    int xo = SCREEN_W / 2;
    int yo = SCREEN_H / 2;
    float fov = 200.0f;
    float loopUpDown = -P.l * 6.3;
    if (loopUpDown > SCREEN_H) loopUpDown = SCREEN_H;

    for(y =-yo; y < -loopUpDown; y++){
        for(x =-xo; x < xo; x++){
            float  z = y + loopUpDown; if(z ==0) z = 0.0001;
            float fx = x / z;       // world floor x;
            float fy = fov / z;  // world floot y

            float rx = fx * M.sin[P.a] - fy * M.cos[P.a] + (P.y /22.0f);
            float ry = fx * M.cos[P.a] + fy * M.sin[P.a] - (P.x/22.0f);


            if( rx < 0) { rx =- rx+1; }
            if( ry < 0) { ry =- ry+1; }
        
            if((int) rx % 2 == (int)ry%2){
                putPixel(x + xo, SCREEN_H - (y + yo), 3);
            } else
                putPixel(x + xo, SCREEN_H - (y + yo), 5);

        }
    }

}

void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, int c, int s, int w, int frontBack){
    int x,y;
    int dyb = b2-b1;
    int dyt = t2-t1;

    int dx  = x2 - x1;  if (dx == 0)  dx = 1;
    int xs = x1;

    int wt = W[w].wt;   // wall texture
    float ht = 0, ht_step=(float)Textures[wt].w * W[w].u / (float)(x2 - x1);

    //clip X
    if( x1< 0) { ht -=ht_step * x1; x1 = 0;}
    if( x2< 0) x2 = 0;
    if( x1 > SCREEN_W) x1 = SCREEN_W;
    if( x2 > SCREEN_W) x2 = SCREEN_W;

    for(x = x1; x < x2; x++){
        int y1 = dyb * (x - xs+0.5f) / dx+b1;
        int y2 = dyt * (x - xs+0.5f) / dx+t1;

        float vt=0, vt_step=(float)Textures[wt].h *W[w].v / (float)(y2-y1);

        //clip y
        if( y1< 0) {vt -= vt_step * y1; y1 = 0;}
        if( y2< 0) y2 = 0;
        if( y1 > SCREEN_H) y1 = SCREEN_H;
        if( y2 > SCREEN_H) y2 = SCREEN_H;

        // surface
        //if(S[s].surface == 1) { S[s].surf[x]=y1; continue; }    // save bottom points
        //if(S[s].surface == 2) { S[s].surf[x]=y2; continue; }    // save top points
        //if(S[s].surface ==-1) { for(y = S[s].surf[x]; y < y1; y++) {putPixel(x, SCREEN_H-y, S[s].c1);};}  // buttom 
        //if(S[s].surface ==-2) { for(y = y2; y < S[s].surf[x]; y++) {putPixel(x, SCREEN_H-y, S[s].c2);};}  // buttom 

        if(frontBack == 0){
            if(S[s].surface == 1) { S[s].surf[x] = y1; }    // bottom surface save
            if(S[s].surface == 2) { S[s].surf[x] = y2; }    // top surface save

            //for(y = y1; y < y2; y++) putPixel(x,SCREEN_H-y,1);  // normal walls
            for(y = y1; y < y2; y++){
                int pixel = (int)(Textures[wt].h - ((int)vt%Textures[wt].h) - 1) * Textures[wt].w + ((int)ht % Textures[wt].w) ;
                int col = Textures[wt].name[pixel];
                putPixel(x, SCREEN_H - y, col);
                vt+=vt_step;
            }
            ht+=ht_step;
        }
        if(frontBack == 1){

            int xo = SCREEN_W / 2;
            int yo = SCREEN_H / 2;
            float fov = 200.0f;
            int x2 = x - xo;
            int wo ;
            float tile = S[s].ss * 7;

            if(S[s].surface == 1) { y2 = S[s].surf[x]; wo=S[s].z1; }    // bottom surface save
            if(S[s].surface == 2) { y1 = S[s].surf[x]; wo=S[s].z2; }    // top surface save

            float lookUpDown = -P.l * 6.2;
            if(lookUpDown >SCREEN_H) lookUpDown = SCREEN_H;

            float moveUpDown = (float)(P.z-wo) / (float)yo;   
            if(moveUpDown == 0) moveUpDown =0.001;

            int ys = y1 - yo, ye = y2-yo;//-lookUpDown;



            //for(y = y1; y < y2; y++) putPixel(x, SCREEN_H - y, 2);
            for(y = ys; y < ye; y++){
                float  z = y + lookUpDown; if(z==0) z=0.0001;
                float fx = x2  / z * moveUpDown * tile;
                float fy = fov / z * moveUpDown * tile;

                float rx = fx * M.sin[P.a] - fy * M.cos[P.a] + (P.y / 235.0 * tile);
                float ry = fx * M.cos[P.a] + fy * M.sin[P.a] - (P.x / 235.0 * tile);
                if(rx < 0) { rx =- rx+1; }
                if(ry < 0) { ry =- ry+1; }
                //if(rx <= 0 || ry <= 0 || rx>=5 || ry>=5){ continue; }
                int st = S[s].st;
                int pixel = (int)Textures[st].h - ((int)ry % Textures[st].h)-1 * ((int)rx % Textures[st].w);
                int col = Textures[st].name[pixel];
                putPixel(x2+xo, SCREEN_H - (y+yo), col);


            }
        }
    }

}



void draw3D(){
    
    int s, x, w, wx[4], wy[4], wz[4];
    float CS = M.cos[P.a], SN = M.sin[P.a];
    int frontBack, cycles;

    // order sort
    for(s = 0; s < numSect-1; s++){
        for(w=0; w<numSect-s-1;w++){
            if(S[w].d < S[w+1].d){
                sectors st = S[w]; S[w]=S[w+1]; S[w+1] = st;
            }
        }
    }

    // draw sectors
    for(s=0; s<numSect; s++){
        S[s].d = 0;

             if(P.z < S[s].z1) { S[s].surface = 1; cycles = 2; for(x=0; x<SCREEN_W;x++) {S[s].surf[x]=SCREEN_H;}}    // bum surface
        else if (P.z > S[s].z2){ S[s].surface = 2; cycles = 2; for(x=0; x<SCREEN_W;x++) {S[s].surf[x]=0;}}   // top surface
        else                   { S[s].surface = 0; cycles = 1; }  // no surface



        for(frontBack = 0; frontBack < cycles; frontBack++){
            for(w = S[s].ws; w<S[s].we; w++){

                int x1 = W[w].x1 - P.x, y1 = W[w].y1 - P.y;
                int x2 = W[w].x2 - P.x, y2 = W[w].y2 - P.y;

                //swap for surfaces
                if(frontBack == 1) {
                    int swp=x1; x1=x2; x2=swp; swp=y1; y1=y2; y2=swp;
                }

                // world x
                wx[0] = x1 * CS - y1 * SN;
                wx[1] = x2 * CS - y2 * SN;
                wx[2] = wx[0];
                wx[3] = wx[1];
                // world y
                wy[0] = y1 * CS + x1 * SN;
                wy[1] = y2 * CS + x2 * SN;
                wy[2] = wy[0];
                wy[3] = wy[1];
                // world z
                wz[0] = S[s].z1 - P.z + ((P.l * wy[0])/32.0f);
                wz[1] = S[s].z1 - P.z + ((P.l * wy[1])/32.0f);;
                wz[2] = S[s].z2 - P.z + ((P.l * wy[0])/32.0f);
                wz[3] = S[s].z2 - P.z + ((P.l * wy[1])/32.0f);;

                S[s].d += dist(0,0, (wx[0]+wx[1])/2, (wy[0]+wy[1])/2);  // store this wall distance

                if(wy[0] <1 && wy[1]<1) continue;
                if(wy[0]<1){
                    clipBehindPlayer(&wx[0], &wy[0], &wz[0],  wx[1], wy[1], wz[1]); // bottomline
                    clipBehindPlayer(&wx[2], &wy[2], &wz[2],  wx[3], wy[3], wz[3]); // topline

                }
                if(wy[1]<1){
                    clipBehindPlayer(&wx[1], &wy[1], &wz[1],  wx[0], wy[0], wz[0]); // bottomline
                    clipBehindPlayer(&wx[3], &wy[3], &wz[3],  wx[2], wy[2], wz[2]); // bottomline
                }

                // screen x, screen y pos
                wx[0] = wx[0] * 200.0f / wy[0] + SCREEN_W2; wy[0] = wz[0] * 200.0f / wy[0] + SCREEN_H2;
                wx[1] = wx[1] * 200.0f / wy[1] + SCREEN_W2; wy[1] = wz[1] * 200.0f / wy[1] + SCREEN_H2;
                wx[2] = wx[2] * 200.0f / wy[2] + SCREEN_W2; wy[2] = wz[2] * 200.0f / wy[2] + SCREEN_H2;
                wx[3] = wx[3] * 200.0f / wy[3] + SCREEN_W2; wy[3] = wz[3] * 200.0f / wy[3] + SCREEN_H2;

                // draw points
                //if((wx[0] > 1 && wx[0] < SCREEN_W) && (wy[0] > 1 && wy[0] < SCREEN_H)) { putPixel(wx[0], wy[0], 1);}
                //if((wx[1] > 1 && wx[1] < SCREEN_W) && (wy[1] > 1 && wy[1] < SCREEN_H)) { putPixel(wx[1], wy[1], 1);}
                drawWall(wx[0], wx[1], wy[0], wy[1], wy[2], wy[3], W[w].c, s, w, frontBack);
                //drawWall(wx[0], wx[1], wy[0], wy[1]);
            }
            S[s].d /= (S[s].we - S[s].ws);  // average sector dist
        }
    }
    
}



void init(){
    int x;
    // init mathing
    for(x = 0; x < 360; x++){
        M.cos[x] = cos(x/180.0f * M_PI);
        M.sin[x] = sin(x/180.0f * M_PI);
    }

    // init player
    P.x = 70; P.y =-100; P.z = 20; P.a = 0; P.l = 0;


    Textures[0].name = T_00; Textures[0].h = T_00_HEIGHT; Textures[0].w = T_00_WIDTH;
    Textures[1].name = T_01; Textures[1].h = T_01_HEIGHT; Textures[1].w = T_01_WIDTH;
    Textures[2].name = T_02; Textures[2].h = T_02_HEIGHT; Textures[2].w = T_02_WIDTH;
   
}





















































//////////////////////////////////////////////////////////////////////////////////


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
        "Raycasting demo (SDL2)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W * ZOOM, SCREEN_H * ZOOM, 0
    );
    if (!sdl_win) {
        fprintf(stderr, "CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    ren = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED);
    //ren = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_PRESENTVSYNC);
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


static int fnext = 0;
int main(void)
{
    if (BasicSDL2Setup() != 0) {
        return 1;
    }

    init();
    
    //SDL_SetRelativeMouseMode(SDL_TRUE);

    int running = 1;
    uint32_t lastTicks = SDL_GetTicks();

    while (running) {
        uint32_t nowTicks = SDL_GetTicks();
        float dt = (float)(nowTicks - lastTicks) / 1000.0f;
        int mouseDx = 0;

        lastTicks = nowTicks;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            }

            if ((e.type == SDL_KEYDOWN) &&
                (e.key.repeat == 0) &&
                (e.key.keysym.sym == SDLK_ESCAPE)) {
                running = 0;
            }

            if (e.type == SDL_MOUSEMOTION) {
                mouseDx += e.motion.xrel;
            }
        }

        const uint8_t *keys = SDL_GetKeyboardState(NULL);
       
        if(fnext){
            fnext = 0;
            movePlayer(keys);
        }
        {    
            clearScreen(0);
            //floors();
            draw3D();

            fnext= updateFPS();
           
            
            testTextures();
            videoMemToScreen();
        }
        SDL_UpdateTexture(tex, NULL, pb, SCREEN_W * (int)sizeof(uint32_t));
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    EndSDL2Session();
    printf("END OF PLAY\n");
    return 0;
}