//// TFMX AUDIO.C ////
#include <stdio.h>
#include <stdint.h>

#include "player.h"
#include "tfmxplay.h"




#define FRACTION_BITS 14
#define INTEGER_MASK (0xFFFFFFFF << FRACTION_BITS)
#define FRACTION_MASK (~ INTEGER_MASK)


void tfmxIrqIn();

//char act[8]={1,1,1,1,1,1,1,1};

uint8_t samplememory[512 * 1024];   // will have to place this in external ram

union {
    uint8_t b8[BUFSIZE];
} buf;



int32_t tbuf[HALFBUFSIZE*2];    // put this somewhere in AXI ram



static int nul=0;
void (*mix)(Hdb *,int,int32_t *);

void mix_add(Hdb *hw,int n,int32_t *b){
    register int8_t * p = hw->sbeg;
    register uint32_t ps=hw->pos;
	int v=hw->vol;
    uint32_t d=hw->delta;
    uint32_t l=(hw->slen<<14);

	if (v>0x40)v=0x40;
    if ((p==(int8_t *)&nul)||( ((hw->mode)&1)==0 )||(l<0x10000))
		return;
    if ((hw->mode&3)==1){
		p=hw->sbeg=hw->SampleStart;
		l=(hw->slen=hw->SampleLength)<<14;
		ps=0;
		hw->mode|=2;
	}
	while(n--){
		(*b++)+=(p[(ps+=d)>>14]*v);
		if (ps<l) continue;
		ps-=l;
		p=hw->SampleStart;
		if ( ((l=((hw->slen=hw->SampleLength)<<14))<0x10000) ||
		     (!hw->loop(hw)) )
				 {
			hw->slen=ps=d=0;
            p = TFMX_smplbuf;
			break;
		}
	}
	hw->sbeg=p;
	hw->pos=ps;
	hw->delta=d;
	if (hw->mode&4) (hw->mode=0);
}

void mix_add_ov(Hdb *hw, int n, int32_t *b){
    register int8_t * p = hw->sbeg;
    register uint32_t ps=hw->pos;
    register uint32_t psreal;
	int v=hw->vol;
    uint32_t d=hw->delta;
    uint32_t l=(hw->slen<<14);

	int v1;
	int v2;

	if (v>0x40)v=0x40;
    if ((p==(int8_t *)&nul)||( ((hw->mode)&1)==0 )||(l<0x10000))
		return;
    if ((hw->mode&3)==1){
		p=hw->sbeg=hw->SampleStart;
		l=(hw->slen=hw->SampleLength)<<14;
		ps=0;
		hw->mode|=2;
	}


	while(n--){
		psreal = ps>>FRACTION_BITS;
		v1 = p[psreal];
        if (psreal+1 < hw->slen){
			v2 = p[psreal+1];
        } else {
			v2 = hw->SampleStart[0];
			/* fprintf(stderr, "H"); */
			/* (*b++) += v*v1; */
		}
		(*b++) += v*((v1 +
			      (((signed) ((v2-v1) * (ps & FRACTION_MASK)))
			       >> FRACTION_BITS)));
		ps += d;

		if (ps<l) continue;
        ps -= l;
        p = hw->SampleStart;
        if ( ((l = ((hw->slen=hw->SampleLength)<<14))<0x10000) ||
		     (!hw->loop(hw)) )
				 {
			hw->slen=ps=d=0;
            p = TFMX_smplbuf;
			break;
		}
	}
    hw->sbeg = p;
    hw->pos = ps;
    hw->delta = d;
    if (hw->mode&4) (hw->mode = 0);
}
	
void (*mix)(Hdb *,int, int32_t *)=&mix_add;

void mixit(int n,int b){
	int x;
    int32_t *y;
    if (g_tfmx.multimode)	{
        if(g_tfmx.act[4]) mix(&TFMX_hdb[4], n, &tbuf[b]);
        if(g_tfmx.act[5]) mix(&TFMX_hdb[5], n, &tbuf[b]);
        if(g_tfmx.act[6]) mix(&TFMX_hdb[6], n, &tbuf[b]);
        if(g_tfmx.act[7]) mix(&TFMX_hdb[7], n, &tbuf[b]);
		y=&tbuf[HALFBUFSIZE+b];
		for (x=0;x<n;x++,y++)
			*y=(*y>16383)?16383:
			   (*y<-16383)?-16383:*y;
	}
	else
        if(g_tfmx.act[3]) mix(&TFMX_hdb[3], n, &tbuf[b]);

    if(g_tfmx.act[0]) mix(&TFMX_hdb[0], n, &tbuf[b]);
    if(g_tfmx.act[1]) mix(&TFMX_hdb[1], n, &tbuf[HALFBUFSIZE+b]);
    if(g_tfmx.act[2]) mix(&TFMX_hdb[2], n, &tbuf[HALFBUFSIZE+b]);
}

void mixem(uint32_t nb, uint32_t bd){
    if (g_tfmx.over==-1) mix=&mix_add_ov; else mix=&mix_add;
    mixit(nb, bd);
}



