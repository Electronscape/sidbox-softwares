#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "gfx.h"
#include "grid2d.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define res        1
#define SW         SCREEN_W
#define SH         SCREEN_H
#define SW2        (SW/2)
#define SH2        (SH/2)
#define pixelScale ZOOM
#define GLSW       (SW*pixelScale)
#define GLSH       (SH*pixelScale)

/* textures */
#include "textures/T_NUMBERS.h"
#include "textures/T_VIEW2D.h"
#include "textures/T_00.h"
#include "textures/T_01.h"
#include "textures/T_02.h"


/* highest valid texture index, not count */
int numText=2;
int numSect=0;
int numWall=0;

typedef struct
{
    int fr1,fr2;
} time_s; static time_s T;

typedef struct
{
    float cos[360];
    float sin[360];
} math_s; static math_s M;

typedef struct
{
    int w,s,a,d;
    int sl,sr;
    int m;
} keys_s; static keys_s K;

typedef struct
{
    int x,y,z;
    int a;
    int l;
} player_s; static player_s P;

typedef struct
{
    int x1,y1;
    int x2,y2;
    int wt,u,v;
    int shade;
} walls; static walls W[256];

typedef struct
{
    int ws,we;
    int z1,z2;
    int d;
    int st,ss;
    int surf[SW];
} sectors; static sectors S[128];

typedef struct
{
    int w,h;
    const unsigned char *name;
} TexureMaps; static TexureMaps Textures[64];

typedef struct
{
    int mx,my;
    int addSect;
    int wt,wu,wv;
    int st,ss;
    int z1,z2;
    int scale;
    int move[4];
    int selS,selW;
} grid; static grid G;

static uint8_t g_view2d_idx[SW * SH];
static int g_view2d_ready = 0;

/* -------------------------------------------------------------------------- */
/* palette helpers                                                            */
/* -------------------------------------------------------------------------- */

static int rgbToIndex(int r, int g, int b)
{
    int best = 0;
    int bestd = 0x7fffffff;

    for (int i = 0; i < 256; i++)
    {
        uint32_t c = clut[i];
        int cr = (int)((c >> 16) & 0xFF);
        int cg = (int)((c >> 8)  & 0xFF);
        int cb = (int)(c & 0xFF);

        int dr = cr - r;
        int dg = cg - g;
        int db = cb - b;
        int d  = dr*dr + dg*dg + db*db;

        if (d < bestd)
        {
            bestd = d;
            best = i;
        }
    }

    return best;
}

static uint8_t darkenIndex(uint8_t idx, float f)
{
    uint32_t c = clut[idx];
    int r = (int)(((c >> 16) & 0xFF) * f);
    int g = (int)(((c >> 8)  & 0xFF) * f);
    int b = (int)(( c        & 0xFF) * f);

    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;

    return (uint8_t)rgbToIndex(r, g, b);
}

/* bottom-left logical coords to top-left framebuffer */
static inline void putPixelBLIndex(int x, int y, uint8_t idx)
{
    if ((unsigned)x >= SW || (unsigned)y >= SH) return;
    fb[((SH - 1 - y) * SW) + x] = idx;
}

static inline uint8_t getPixelBLIndex(int x, int y)
{
    if ((unsigned)x >= SW || (unsigned)y >= SH) return 0;
    return fb[((SH - 1 - y) * SW) + x];
}

static inline void drawPixelIdx(int x, int y, uint8_t idx)
{
    putPixelBLIndex(x, y, idx);
}

static void buildView2DCache(void)
{
    for (int y = 0; y < SH; y++)
    {
        int y2 = (SH - y - 1) * 3 * 160;
        for (int x = 0; x < SW; x++)
        {
            int pixel = x * 3 + y2;
            int r = T_VIEW2D[pixel + 0];
            int g = T_VIEW2D[pixel + 1];
            int b = T_VIEW2D[pixel + 2];
            g_view2d_idx[y * SW + x] = (uint8_t)rgbToIndex(r, g, b);
        }
    }
    g_view2d_ready = 1;
}

/* -------------------------------------------------------------------------- */
/* original functions, ported to framebuffer                                  */
/* -------------------------------------------------------------------------- */

void save(void)
{
    int w,s;
    FILE *fp = fopen("../RC3DGraphics/level.h","w");
    if(fp == NULL){ printf("Error opening the file level.h\n"); return; }
    if(numSect==0){ fclose(fp); return; }

    fprintf(fp,"%i\n",numSect);
    for(s=0;s<numSect;s++)
    {
        fprintf(fp,"%i %i %i %i %i %i\n",S[s].ws,S[s].we,S[s].z1,S[s].z2,S[s].st,S[s].ss);
    }

    fprintf(fp,"%i\n",numWall);
    for(w=0;w<numWall;w++)
    {
        fprintf(fp,"%i %i %i %i %i %i %i %i\n",W[w].x1,W[w].y1,W[w].x2,W[w].y2,W[w].wt,W[w].u,W[w].v,W[w].shade);
    }
    fprintf(fp,"\n%i %i %i %i %i\n",P.x,P.y,P.z,P.a,P.l);
    fclose(fp);
}

void load(void)
{
    FILE *fp = fopen("../RC3DGraphics/level.h","r");
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
    fscanf(fp,"%i %i %i %i %i",&P.x,&P.y,&P.z,&P.a,&P.l);
    fclose(fp);
}

void initGlobals(void)
{
    G.scale=4;
    G.selS=0; G.selW=0;
    G.z1=0;   G.z2=40;
    G.st=1;   G.ss=4;
    G.wt=0;   G.wu=1; G.wv=1;
}

void drawPixel(int x,int y,int r,int g,int b)
{
    putPixelBLIndex(x, y, (uint8_t)rgbToIndex(r,g,b));
}

void drawLineRGB(float x1,float y1,float x2,float y2,int r,int g,int b)
{
    int n;
    float x = x2 - x1;
    float y = y2 - y1;
    float max = fabsf(x);
    if(fabsf(y) > max){ max = fabsf(y); }

    if(max < 1.0f)
    {
        drawPixel((int)x1,(int)y1,r,g,b);
        return;
    }

    x /= max;
    y /= max;
    for(n=0;n<(int)max;n++)
    {
        drawPixel((int)x1,(int)y1,r,g,b);
        x1 += x;
        y1 += y;
    }
}

void drawNumber(int nx,int ny,int n)
{
    int x,y;
    if(n < 0) return;

    for(y=0;y<5;y++)
    {
        int y2=((5-y-1)+5*n)*3*12;
        for(x=0;x<12;x++)
        {
            int x2=x*3;
            if(T_NUMBERS[y2+x2]==0){ continue; }
            drawPixel(x+nx,y+ny,255,255,255);
        }
    }
}

void draw2D(void)
{
    int s,w,x,y,c;

    if (!g_view2d_ready) buildView2DCache();

    /* draw background */
    for(y=0;y<120;y++)
    {
        for(x=0;x<160;x++)
        {
            uint8_t idx = g_view2d_idx[y * SW + x];

            if(G.addSect>0 && y>48-8 && y<56-8 && x>144)
            {
                idx = darkenIndex(idx, 0.5f);
            }

            putPixelBLIndex(x, y, idx);
        }
    }

    /* draw sectors */
    for(s=0;s<numSect;s++)
    {
        for(w=S[s].ws;w<S[s].we;w++)
        {
            if(s==G.selS-1)
            {
                S[G.selS-1].z1=G.z1;
                S[G.selS-1].z2=G.z2;
                S[G.selS-1].st=G.st;
                S[G.selS-1].ss=G.ss;

                     if(G.selW==0)          { c=80; }
                else if(G.selW+S[s].ws-1==w){ c=80; W[w].wt=G.wt; W[w].u=G.wu; W[w].v=G.wv; }
                else                        { c=0; }
            }
            else { c=0; }

            drawLineRGB((float)(W[w].x1/G.scale),(float)(W[w].y1/G.scale),
                        (float)(W[w].x2/G.scale),(float)(W[w].y2/G.scale),
                        128+c,128+c,128-c);

            drawPixel(W[w].x1/G.scale,W[w].y1/G.scale,255,255,255);
            drawPixel(W[w].x2/G.scale,W[w].y2/G.scale,255,255,255);
        }
    }

    /* draw player */
    {
        int dx=(int)(M.sin[P.a]*12.0f);
        int dy=(int)(M.cos[P.a]*12.0f);
        drawPixel(P.x/G.scale,P.y/G.scale,0,255,0);
        drawPixel((P.x+dx)/G.scale,(P.y+dy)/G.scale,0,175,0);
    }

    /* draw wall texture */
    {
        float tx=0, tx_stp=(float)Textures[G.wt].w/15.0f;
        float ty=0, ty_stp=(float)Textures[G.wt].h/15.0f;
        for(y=0;y<15;y++)
        {
            tx=0;
            for(x=0;x<15;x++)
            {
                int x2=(int)tx % Textures[G.wt].w; tx+=tx_stp;
                int y2=(int)ty % Textures[G.wt].h;

                uint8_t idx =
                    Textures[G.wt].name[(Textures[G.wt].h - y2 - 1) * Textures[G.wt].w + x2];

                drawPixelIdx(x + 145, y + 105 - 8, idx);
            }
            ty+=ty_stp;
        }
    }

    /* draw surface texture */
    {
        float tx=0, tx_stp=(float)Textures[G.st].w/15.0f;
        float ty=0, ty_stp=(float)Textures[G.st].h/15.0f;
        for(y=0;y<15;y++)
        {
            tx=0;
            for(x=0;x<15;x++)
            {
                int x2=(int)tx % Textures[G.st].w; tx+=tx_stp;
                int y2=(int)ty % Textures[G.st].h;

                uint8_t idx =
                    Textures[G.st].name[(Textures[G.st].h - y2 - 1) * Textures[G.st].w + x2];

                drawPixelIdx(x + 145, y + 105 - 24 - 8, idx);
            }
            ty+=ty_stp;
        }
    }

    drawNumber(140,90,G.wu);
    drawNumber(148,90,G.wv);
    drawNumber(148,66,G.ss);
    drawNumber(148,58,G.z2);
    drawNumber(148,50,G.z1);
    drawNumber(148,26,G.selS);
    drawNumber(148,18,G.selW);
}

int dark=0;
void darken(void)
{
    int x,y,xs=0,xe=0,ys=0,ye=0;
    if(dark==0){ return; }

    if(dark== 1){ xs= 0; xe=15; ys= 0/G.scale;   ye= 32/G.scale; }
    if(dark== 2){ xs= 0; xe= 3; ys=96/G.scale;   ye=128/G.scale; }
    if(dark== 3){ xs= 4; xe= 8; ys=96/G.scale;   ye=128/G.scale; }
    if(dark== 4){ xs= 7; xe=11; ys=96/G.scale;   ye=128/G.scale; }
    if(dark== 5){ xs=11; xe=15; ys=96/G.scale;   ye=128/G.scale; }
    if(dark== 6){ xs= 0; xe= 8; ys=192/G.scale;  ye=224/G.scale; }
    if(dark== 7){ xs= 8; xe=15; ys=192/G.scale;  ye=224/G.scale; }
    if(dark== 8){ xs= 0; xe= 7; ys=224/G.scale;  ye=256/G.scale; }
    if(dark== 9){ xs= 7; xe=15; ys=224/G.scale;  ye=256/G.scale; }
    if(dark==10){ xs= 0; xe= 7; ys=256/G.scale;  ye=288/G.scale; }
    if(dark==11){ xs= 7; xe=15; ys=256/G.scale;  ye=288/G.scale; }
    if(dark==12){ xs= 0; xe= 7; ys=352/G.scale;  ye=386/G.scale; }
    if(dark==13){ xs= 7; xe=15; ys=352/G.scale;  ye=386/G.scale; }
    if(dark==14){ xs= 0; xe= 7; ys=386/G.scale;  ye=416/G.scale; }
    if(dark==15){ xs= 7; xe=15; ys=386/G.scale;  ye=416/G.scale; }
    if(dark==16){ xs= 0; xe=15; ys=416/G.scale;  ye=448/G.scale; }
    if(dark==17){ xs= 0; xe=15; ys=448/G.scale;  ye=480/G.scale; }

    for(y=ys;y<ye;y++)
    {
        for(x=xs;x<xe;x++)
        {
            int lx = x + 145;
            int ly = 119 - y;
            uint8_t idx = getPixelBLIndex(lx, ly);
            putPixelBLIndex(lx, ly, darkenIndex(idx, 0.6f));
        }
    }
}

void mouse(int button, int state, int x, int y)
{
    int s,w;

    G.mx=x/pixelScale;
    G.my=SH-y/pixelScale;
    G.mx=((G.mx+4)>>3)<<3;
    G.my=((G.my+4)>>3)<<3;

    if(button == SDL_BUTTON_LEFT && state == 1)
    {
        if(x>580)
        {
            if(y>0 && y<32){ save(); dark=1; }

            if(y>32 && y<96)
            {
                if(x<610){ G.wt-=1; if(G.wt<0){ G.wt=numText; } }
                else     { G.wt+=1; if(G.wt>numText){ G.wt=0; } }
            }

            if(y>96 && y<128)
            {
                     if(x<595){ dark=2; G.wu-=1; if(G.wu<1){ G.wu=1; } }
                else if(x<610){ dark=3; G.wu+=1; if(G.wu>9){ G.wu=9; } }
                else if(x<625){ dark=4; G.wv-=1; if(G.wv<1){ G.wv=1; } }
                else if(x<640){ dark=5; G.wv+=1; if(G.wv>9){ G.wv=9; } }
            }

            if(y>128 && y<192)
            {
                if(x<610){ G.st-=1; if(G.st<0){ G.st=numText; } }
                else     { G.st+=1; if(G.st>numText){ G.st=0; } }
            }

            if(y>192 && y<222)
            {
                if(x<610){ dark=6; G.ss-=1; if(G.ss<1){ G.ss=1; } }
                else     { dark=7; G.ss+=1; if(G.ss>9){ G.ss=9; } }
            }

            if(y>222 && y<256)
            {
                if(x<610){ dark=8; G.z2-=5; if(G.z2==G.z1){ G.z1-=5; } }
                else     { dark=9; G.z2+=5; }
            }

            if(y>256 && y<288)
            {
                if(x<610){ dark=10; G.z1-=5; }
                else     { dark=11; G.z1+=5; if(G.z1==G.z2){ G.z2+=5; } }
            }

            if(y>288 && y<318){ G.addSect+=1; G.selS=0; G.selW=0; if(G.addSect>1){ G.addSect=0; } }

            if(G.z1<0){ G.z1=0; }   if(G.z1>145){ G.z1=145; }
            if(G.z2<5){ G.z2=5; }   if(G.z2>150){ G.z2=150; }

            if(y>352 && y<386)
            {
                G.selW=0;
                if(x<610){ dark=12; G.selS-=1; if(G.selS<0){ G.selS=numSect; } }
                else     { dark=13; G.selS+=1; if(G.selS>numSect){ G.selS=0; } }

                if(G.selS==0)
                {
                    initGlobals();
                }
                else
                {
                    int ss=G.selS-1;
                    G.z1=S[ss].z1;
                    G.z2=S[ss].z2;
                    G.st=S[ss].st;
                    G.ss=S[ss].ss;
                    G.wt=W[S[ss].ws].wt;
                    G.wu=W[S[ss].ws].u;
                    G.wv=W[S[ss].ws].v;
                }
            }

            if(G.selS>0 && y>386 && y<416)
            {
                int snw=S[G.selS-1].we-S[G.selS-1].ws;
                if(x<610)
                {
                    dark=14;
                    G.selW-=1; if(G.selW<0){ G.selW=snw; }
                }
                else
                {
                    dark=15;
                    G.selW+=1; if(G.selW>snw){ G.selW=0; }
                }

                if(G.selW>0)
                {
                    G.wt=W[S[G.selS-1].ws+G.selW-1].wt;
                    G.wu=W[S[G.selS-1].ws+G.selW-1].u;
                    G.wv=W[S[G.selS-1].ws+G.selW-1].v;
                }
            }

            if(y>416 && y<448)
            {
                dark=16;
                if(G.selS>0)
                {
                    int d=G.selS-1;
                    numWall-=(S[d].we-S[d].ws);
                    for(x=d;x<numSect;x++){ S[x]=S[x+1]; }
                    numSect-=1;
                    G.selS=0; G.selW=0;
                }
            }

            if(y>448 && y<480){ dark=17; load(); }
        }
        else
        {
            if(G.addSect==1)
            {
                S[numSect].ws=numWall;
                S[numSect].we=numWall+1;
                S[numSect].z1=G.z1;
                S[numSect].z2=G.z2;
                S[numSect].st=G.st;
                S[numSect].ss=G.ss;
                W[numWall].x1=G.mx*G.scale; W[numWall].y1=G.my*G.scale;
                W[numWall].x2=G.mx*G.scale; W[numWall].y2=G.my*G.scale;
                W[numWall].wt=G.wt;
                W[numWall].u=G.wu;
                W[numWall].v=G.wv;
                printf("Bork!");
                numWall+=1;
                numSect+=1;
                G.addSect=3;
            }
            else if(G.addSect==3)
            {
                if(S[numSect-1].ws==numWall-1 && G.mx*G.scale<=W[S[numSect-1].ws].x1)
                {
                    numWall-=1; numSect-=1; G.addSect=0;
                    printf("walls must be counter clockwise\n");
                    return;
                }

                W[numWall-1].x2=G.mx*G.scale; W[numWall-1].y2=G.my*G.scale;

                {
                    float ang = atan2f((float)(W[numWall-1].y2-W[numWall-1].y1),
                                       (float)(W[numWall-1].x2-W[numWall-1].x1));
                    ang=(ang*180.0f)/(float)M_PI;
                    if(ang<0){ ang+=360; }
                    int shade=(int)ang;
                    if(shade>180){ shade=180-(shade-180); }
                    if(shade>90 ){ shade= 90-(shade- 90); }
                    W[numWall-1].shade=shade;
                }

                if(W[numWall-1].x2==W[S[numSect-1].ws].x1 && W[numWall-1].y2==W[S[numSect-1].ws].y1)
                {
                    W[numWall-1].wt=G.wt;
                    W[numWall-1].u=G.wu;
                    W[numWall-1].v=G.wv;
                    G.addSect=0;
                }
                else
                {
                    S[numSect-1].we+=1;
                    W[numWall].x1=G.mx*G.scale; W[numWall].y1=G.my*G.scale;
                    W[numWall].x2=G.mx*G.scale; W[numWall].y2=G.my*G.scale;
                    W[numWall-1].wt=G.wt;
                    W[numWall-1].u=G.wu;
                    W[numWall-1].v=G.wv;
                    W[numWall].shade=0;
                    numWall+=1;
                }
            }
        }
    }

    for(w=0;w<4;w++){ G.move[w]=-1; }

    if(G.addSect==0 && button == SDL_BUTTON_RIGHT && state == 1)
    {
        for(s=0;s<numSect;s++)
        {
            for(w=S[s].ws;w<S[s].we;w++)
            {
                int x1=W[w].x1, y1=W[w].y1;
                int x2=W[w].x2, y2=W[w].y2;
                int mx=G.mx*G.scale, my=G.my*G.scale;

                if(mx<x1+3 && mx>x1-3 && my<y1+3 && my>y1-3){ G.move[0]=w; G.move[1]=1; }
                if(mx<x2+3 && mx>x2-3 && my<y2+3 && my>y2-3){ G.move[2]=w; G.move[3]=2; }
            }
        }
    }

    if(button == SDL_BUTTON_LEFT && state == 0){ dark=0; }
}

void mouseMoving(int x, int y)
{
    if(x<580 && G.addSect==0 && G.move[0]>-1)
    {
        int Aw=G.move[0], Ax=G.move[1];
        int Bw=G.move[2], Bx=G.move[3];

        if(Ax==1){ W[Aw].x1=((x+16)>>5)<<5; W[Aw].y1=((GLSH-y+16)>>5)<<5; }
        if(Ax==2){ W[Aw].x2=((x+16)>>5)<<5; W[Aw].y2=((GLSH-y+16)>>5)<<5; }
        if(Bx==1){ W[Bw].x1=((x+16)>>5)<<5; W[Bw].y1=((GLSH-y+16)>>5)<<5; }
        if(Bx==2){ W[Bw].x2=((x+16)>>5)<<5; W[Bw].y2=((GLSH-y+16)>>5)<<5; }
    }
}

void KeysDown(unsigned char key,int x,int y)
{
    (void)x; (void)y;
    if(key=='w'){ K.w =1; }
    if(key=='s'){ K.s =1; }
    if(key=='a'){ K.a =1; }
    if(key=='d'){ K.d =1; }
    if(key=='m'){ K.m =1; }
    if(key==','){ K.sr=1; }
    if(key=='.'){ K.sl=1; }
}

void KeysUp(unsigned char key,int x,int y)
{
    (void)x; (void)y;
    if(key=='w'){ K.w =0; }
    if(key=='s'){ K.s =0; }
    if(key=='a'){ K.a =0; }
    if(key=='d'){ K.d =0; }
    if(key=='m'){ K.m =0; }
    if(key==','){ K.sr=0; }
    if(key=='.'){ K.sl=0; }
}

void movePlayer(void)
{
    if(K.a==1 && K.m==0){ P.a-=4; if(P.a<0){ P.a+=360; } }
    if(K.d==1 && K.m==0){ P.a+=4; if(P.a>359){ P.a-=360; } }

    int dx=(int)(M.sin[P.a]*10.0f);
    int dy=(int)(M.cos[P.a]*10.0f);

    if(K.w==1 && K.m==0){ P.x+=dx; P.y+=dy; }
    if(K.s==1 && K.m==0){ P.x-=dx; P.y-=dy; }

    if(K.sr==1){ P.x+=dy; P.y-=dx; }
    if(K.sl==1){ P.x-=dy; P.y+=dx; }

    if(K.a==1 && K.m==1){ P.l-=1; }
    if(K.d==1 && K.m==1){ P.l+=1; }
    if(K.w==1 && K.m==1){ P.z-=4; }
    if(K.s==1 && K.m==1){ P.z+=4; }
}

void display(void)
{
    T.fr1 = (int)SDL_GetTicks();

    if(T.fr1-T.fr2>=50)
    {
        movePlayer();
        draw2D();
        darken();
        T.fr2=T.fr1;
    }
}

int shade(int w)
{
    float ang = atan2f((float)(W[w].y2-W[w].y1),(float)(W[w].x2-W[w].x1));
    ang=(ang*180.0f)/(float)M_PI;
    if(ang<0){ ang+=360; }
    int shadev=(int)ang;
    if(shadev>180){ shadev=180-(shadev-180); }
    if(shadev>90 ){ shadev= 90-(shadev- 90); }
    return (int)(shadev*0.75f);
}

void init(void)
{
    int x;
    initGlobals();

    P.x=32*9; P.y=48; P.z=30; P.a=0; P.l=0;

    for(x=0;x<360;x++)
    {
        M.cos[x]=cosf((float)x/180.0f*(float)M_PI);
        M.sin[x]=sinf((float)x/180.0f*(float)M_PI);
    }

    Textures[ 0].name=T_00; Textures[ 0].h=T_00_HEIGHT; Textures[ 0].w=T_00_WIDTH;
    Textures[ 1].name=T_01; Textures[ 1].h=T_01_HEIGHT; Textures[ 1].w=T_01_WIDTH;
    Textures[ 2].name=T_02; Textures[ 2].h=T_02_HEIGHT; Textures[ 2].w=T_02_WIDTH;

    buildView2DCache();
    memset(fb, 0, sizeof(fb));
}

/* -------------------------------------------------------------------------- */
/* SDL bridge                                                                 */
/* -------------------------------------------------------------------------- */

void grid2dInit(void)
{
    init();
}

void grid2dHandleEvent(const SDL_Event *e)
{
    switch (e->type)
    {
        case SDL_MOUSEBUTTONDOWN:
            mouse(e->button.button, 1, e->button.x, e->button.y);
            break;

        case SDL_MOUSEBUTTONUP:
            mouse(e->button.button, 0, e->button.x, e->button.y);
            break;

        case SDL_MOUSEMOTION:
            mouseMoving(e->motion.x, e->motion.y);
            break;

        case SDL_KEYDOWN:
            if (e->key.repeat == 0)
            {
                SDL_Keycode k = e->key.keysym.sym;
                if (k == SDLK_w) KeysDown('w',0,0);
                if (k == SDLK_s) KeysDown('s',0,0);
                if (k == SDLK_a) KeysDown('a',0,0);
                if (k == SDLK_d) KeysDown('d',0,0);
                if (k == SDLK_m) KeysDown('m',0,0);
                if (k == SDLK_COMMA)  KeysDown(',',0,0);
                if (k == SDLK_PERIOD) KeysDown('.',0,0);
            }
            break;

        case SDL_KEYUP:
        {
            SDL_Keycode k = e->key.keysym.sym;
            if (k == SDLK_w) KeysUp('w',0,0);
            if (k == SDLK_s) KeysUp('s',0,0);
            if (k == SDLK_a) KeysUp('a',0,0);
            if (k == SDLK_d) KeysUp('d',0,0);
            if (k == SDLK_m) KeysUp('m',0,0);
            if (k == SDLK_COMMA)  KeysUp(',',0,0);
            if (k == SDLK_PERIOD) KeysUp('.',0,0);
        } break;

        default:
            break;
    }
}

void grid2dFrame(void)
{
    display();
}