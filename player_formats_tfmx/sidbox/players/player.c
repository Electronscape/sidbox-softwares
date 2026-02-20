/***************************************************************************
 *   Copyright (C) 2004 by David Banz                                      *
 *   neko@netcologne.de                                                    *
 *   GPL'ed                                                                *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>

#include "player.h"
#include "tfmxplay.h"

#include "../../main.h"


const int notevals[] = {
    0x6AE, 0x64E, 0x5F4, 0x59E,  0x54D, 0x501, 0x4B9, 0x475,
    0x435, 0x3F9, 0x3C0, 0x38C,  0x358, 0x32A, 0x2FC, 0x2D0,
    0x2A8, 0x282, 0x25E, 0x23B,  0x21B, 0x1FD, 0x1E0, 0x1C6,
    0x1AC, 0x194, 0x17D, 0x168,  0x154, 0x140, 0x12F, 0x11E,
    0x10E, 0x0FE, 0x0F0, 0x0E3,  0x0D6, 0x0CA, 0x0BF, 0x0B4,
    0x0AA, 0x0A0, 0x097, 0x08F,  0x087, 0x07F, 0x078, 0x071,
    0x0D6, 0x0CA, 0x0BF, 0x0B4,  0x0AA, 0x0A0, 0x097, 0x08F,
    0x087, 0x07F, 0x078, 0x071,  0x0D6, 0x0CA, 0x0BF, 0x0B4
};


tfx_stream_t g_tfmx;



void TFMX_SystemInit(){
    memset(&g_tfmx, 0x00, sizeof(g_tfmx));

    // --- Configure TFMX defaults like original ---

    g_tfmx.eClocks  = 14318;
    g_tfmx.outRate  = 44100;
    g_tfmx.jiffies  = 0;
    g_tfmx.startPat = -1;

    g_tfmx.over = -1;
    g_tfmx.loops = 1;
    g_tfmx.multimode = 0;

    memset(g_tfmx.act, 1, sizeof(g_tfmx.act));
}



void NotePort(uint32_t i){
	UNI x;
    Cdb *c;
	x.l=i;
    c=&TFMX_cdb[x.b.b2&(g_tfmx.multimode?7:3)];
	if (x.b.b0==0xFC)
	{ /* lock */
		c->SfxFlag=x.b.b1;
		c->SfxLockTime=x.b.b3;
		return;
	}
	if (c->SfxFlag) return;
	if (x.b.b0<0xC0)
	{
        if (!g_tfmx.dangerFreakHack)
			c->Finetune=(int)x.b.b3;
		else
			c->Finetune=0;

		c->Velocity=(x.b.b2>>4)&0xF;
		c->PrevNote=c->CurrNote;
		c->CurrNote=x.b.b0;
		c->ReallyWait=1;
		c->NewStyleMacro=0xFF;
        c->MacroPtr=TFMX_macros[c->MacroNum=x.b.b1];
		
		c->MacroStep=c->EfxRun=c->MacroWait=0;
		
		c->KeyUp=1;
		c->Loop=-1;
		c->MacroRun=-1;
	}
	else if (x.b.b0<0xF0)
	{
		c->PortaReset=x.b.b1;
		c->PortaTime=1;
		if (!c->PortaRate) c->PortaPer=c->DestPeriod;
		c->PortaRate=x.b.b3;
		c->DestPeriod=(notevals[c->CurrNote=(x.b.b0&0x3F)]);
	}
	else switch (x.b.b0)
	{
	case 0xF7: /* enve */
		c->EnvRate=x.b.b1;
		c->EnvReset=c->EnvTime=(x.b.b2>>4)+1;
		c->EnvEndvol=x.b.b3;
		break;
	case 0xF6: /* vibr */
		c->VibTime=(c->VibReset=(x.b.b1&0xFE))>>1;
		c->VibWidth=x.b.b3;
		c->VibFlag=1; /* ?! */
		c->VibOffset=0;
		break;
	case 0xF5: /* kup^ */
		c->KeyUp=0;
		break;
	}
}

#define MAYBEWAIT if (c->NewStyleMacro==0x0) {\
		c->NewStyleMacro=0xFF;\
		break;\
	} else {\
		return;\
	}

int LoopOff(/* struct Hdb *hw */){
	return 1;
}

int LoopOn(Hdb *hw){
	if (!hw->c) return 1;
	if (hw->c->WaitDMACount--) return 1;
	hw->loop=&LoopOff;
	hw->c->MacroRun=0xFF;
	return 1;
}

void RunMacro(Cdb *c, uint32_t nChannel){
	UNI x;
    register int a=0;
	c->MacroWait=0;
	loop:
    x.l=tfmx_be32(g_tfmx.editbuf[c->MacroPtr+(c->MacroStep++)]);
	a=x.b.b0;
	x.b.b0=0;

    switch (a){
	case 0: /* dmaoff+reset */
		c->EnvReset=c->VibReset=/*c->ArpRun=c->SIDSize=*/c->PortaRate=
		c->AddBeginTime=0;

        if (g_tfmx.gemx){
			if (x.b.b2)
				c->CurVol=x.b.b3;
			else
				c->CurVol=x.b.b3+(c->Velocity)*3;
		}
	case 0x13: /* dmaoff */
		c->hw->loop=&LoopOff;
        if (!x.b.b1) {
			c->hw->mode=0;
/* START Added by Stefan Ohlsson.
   Removes glitch in TurricanII World2 Song0, among others */
			if(c->NewStyleMacro)
			{
				c->hw->slen=0;
			}
/* END */
			break;
		}
		else
		{
			c->hw->mode|=4;
			c->NewStyleMacro=0;
			return;
		}
	case 0x1: /* dma on */
		c->EfxRun=x.b.b1;
		c->hw->mode=1;
        if ((!c->NewStyleMacro)||(g_tfmx.dangerFreakHack))
		{
            c->hw->SampleStart=&TFMX_smplbuf[c->SaveAddr];
			c->hw->SampleLength=(c->SaveLen)?c->SaveLen<<1:131072;
			c->hw->sbeg=c->hw->SampleStart;
			c->hw->slen=c->hw->SampleLength;
			c->hw->pos=0;
			c->hw->mode|=2;

			break;
		}
		else
		{
			/*printf("--- using new style macro ---\n");*/
			break;
		}
	case 0x2: /* setbegin */
		c->AddBeginTime=0;
        //c->SaveAddr=c->CurAddr=x.l;
        c->SaveAddr = c->CurAddr = (x.l & ~1u);   // force even byte offset

		break;
	case 0x11: /* addbegin */
		c->AddBeginTime=c->AddBeginReset=x.b.b1;
        a=c->CurAddr+(c->AddBegin=(int16_t)x.w.w1);
/*		if (c->SIDSize)
			c->SIDSrcSample=c->CurAddr=a;
		else
*/			a = c->CurAddr + (c->AddBegin=(int16_t)x.w.w1);
        a &= ~1;                         // even align
        c->SaveAddr = c->CurAddr = a;
		break;
	case 0x3: /* setlen */
		c->SaveLen=c->CurrLength=x.w.w1;
		break;
	case 0x12: /* addlen */
		c->CurrLength+=x.w.w1;
		a=c->CurrLength;
/*		if (c->SIDSize)
			c->SIDSrcLength=a;
		else*/
			c->SaveLen=a;
		break;
	case 0x4:
		if (x.b.b1&0x01)
		{
			if (c->ReallyWait++)
				return;
		}
		/* this fixes part of the Z-Out theme problem, but actually it is WRONG!
		bytes/words are already ordered according to byteorder in tfmxplay.h */
/*#ifdef WORDS_BIGENDIAN
		c->MacroWait=x.w.w0;
#else*/
		c->MacroWait=x.w.w1;
/*#endif*/
		MAYBEWAIT;
	case 0x1A:
		c->hw->loop=&LoopOn;
		c->hw->c=c;
		c->WaitDMACount=x.w.w1;
		c->MacroRun=0;
/*		return;*/
		MAYBEWAIT;
	case 0x1C: /* note split */
		if (c->CurrNote>x.b.b1)
			c->MacroStep=x.w.w1;
		break;
	case 0x1D: /* vol split */
		if (c->CurVol>x.b.b1)
			c->MacroStep=x.w.w1;
		break;
/*
TODO: add random play/random limit (0x1e/0x1b) for Master Blazer Ingame:
(need more docs!)
*/
	case 0x1B: /* TODO: random play */
            //printf("TODO: random play (0x1B)\n");
                //fprintf(stderr,"Found code %08x at step %04x in macro %02x",\
				x.l,c->MacroStep-1,c->MacroNum);
		break;
	case 0x1E: /* TODO:random limit */
            //printf("TODO: random limit (0x1E)\n");
                //fprintf(stderr,"Found code %08x at step %04x in macro %02x",\
				x.l,c->MacroStep-1,c->MacroNum);
	        break;
	case 0x10: /* loop key up */
		if (!c->KeyUp)
			break;
	case 0x5: /* loop */
		if (!(c->Loop--))
			break;
		else if (c->Loop<0)
			c->Loop=x.b.b1-1;
		c->MacroStep=x.w.w1;
		break;
	case 0x7: /* stop */
		c->MacroRun=0;
		return;
	case 0xD: /* add volume */
		if (x.b.b2!=0xFE)
        {
            int8_t tempVol;
			/* --- neofix --- */
			/*c->CurVol=(c->Velocity*3)+x.b.b3;*/
			tempVol=(c->Velocity*3)+x.b.b3;
			if (tempVol>0x40)
				c->CurVol=0x40;
			else
				c->CurVol=tempVol;
			/* --- neofix end --- */
			break;
		}
		break;
/*			c->CurVol=c->Velocity*3+x.b.b3;
		PutNote */
	case 0xE: /* set volume */
		if (x.b.b2!=0xFE)
		{
			c->CurVol=x.b.b3;
			break;
		}
		break;
	case 0x21: /* start macro */
		x.b.b0=c->CurrNote;
		x.b.b2|=c->Velocity<<4;
		NotePort(x.l);
		break;
	case 0x1F: /* set prev note */
		a=c->PrevNote;
		goto SetNote;
	case 0x8:
		a=c->CurrNote;
		goto SetNote;
	case 0x9:
		a=0;
		SetNote:
		
		/*a=(notevals[a+x.b.b1&0x3F]*(0x100+c->Finetune+(S8)x.b.b3))>>8;*/
        a = (notevals[(a+x.b.b1) & (0x3F)] * ( 0x100 + c->Finetune + (int8_t)x.b.b3 )) >> 8;
		
        c->DestPeriod = a;
		if (!c->PortaRate) c->CurPeriod=a;
		MAYBEWAIT;
	case 0x17: /* setperiod */
		c->DestPeriod=x.w.w1;
		if (!c->PortaRate) c->CurPeriod=x.w.w1;
		break;
	case 0xB: /* portamento FIXME: for R-Type (to high) */
		c->PortaReset=x.b.b1;
		c->PortaTime=1;
		if (!c->PortaRate) c->PortaPer=c->DestPeriod;
		c->PortaRate=x.w.w1;
		break;
	case 0xC: /* vibrato FIXME: X-Out loader, Apprentice (too fast) */
		c->VibTime=(c->VibReset=x.b.b1)>>1;
		c->VibWidth=x.b.b3;
		c->VibFlag=1;
		if (!c->PortaRate)
		{
			c->CurPeriod=c->DestPeriod;
			c->VibOffset=0;
		}
		break;
	case 0xF: /* envelope */
		c->EnvReset=c->EnvTime=x.b.b2;
		c->EnvEndvol=x.b.b3;
		c->EnvRate=x.b.b1;
		break;
	case 0xA: /* reset */
		c->EnvReset=c->VibReset=/*c->ArpRun=c->SIDSize=*/c->PortaRate=
		c->AddBeginTime=0;
		break;
	case 0x14: /* wait key up */
		if (!c->KeyUp) c->Loop=0;
		if (!c->Loop)
		{
			c->Loop=-1;
			break;
		}
		if (c->Loop==-1)
			c->Loop=x.b.b3-1;
		else
			c->Loop--;
		c->MacroStep--;
		return;
	case 0x15: /* go sub */
		c->ReturnPtr=c->MacroPtr;
		c->ReturnStep=c->MacroStep;
	case 0x6: /* cont */
        c->MacroPtr=(c->MacroNum = TFMX_macros[x.b.b1]);
        c->MacroStep = x.w.w1;
        c->Loop = 0xFFFF;
		break;
	case 0x16: /* return sub */
		c->MacroPtr=c->ReturnPtr;
		c->MacroStep=c->ReturnStep;
		break;
	case 0x18: /* sampleloop */
		c->SaveAddr+=(x.w.w1&0xFFFE);
		c->SaveLen-=x.w.w1>>1;
		c->CurrLength=c->SaveLen;
		c->CurAddr=c->SaveAddr;
		break;
	case 0x19: /* oneshot */
		c->AddBeginTime=0;
		c->SaveAddr=c->CurAddr=0;
		c->SaveLen=c->CurrLength=1;
		break;
	case 0x20: /* cue */
        TFMX_idb.Cue[x.b.b1&0x03]=x.w.w1;
		break;
/*
TODO:
About macros 22-30 (as used in GemZ Title/Credits):
Not much is known about them, JHP wrote the following
stuff in his unofficial TFMX docs (regarding 22-29):

MacrSIDSampleMsg        dc.b    'SID setbeg  xxxxxx   sample-startadress',0
MacrSIDLengthMsg        dc.b    'SID setlen  xx/xxxx  buflen/sourcelen  ',0
MacrSID2OfsMsg          dc.b    'SID op3 ofs xxxxxx   offset            ',0
MacrSID2VibMsg          dc.b    'SID op3 frq xx/xxxx  speed/amplitude   ',0
MacrSID1OfsMsg          dc.b    'SID op2 ofs xxxxxx   offset            ',0
MacrSID1VibMsg          dc.b    'SID op2 frq xx/xxxx  speed/amplitude   ',0
MacrSIDFilterMsg        dc.b    'SID op1     xx/xx/xx speed/amplitude/TC',0
MacrSIDStopMsg          dc.b    'SID stop    xx....   flag (1=clear all)',0
*/
        case 0x22:
            //printf("TODO: SIDSampleMsg (0x22)\n");
                //fprintf(stderr,"Found code %08x at step %04x in macro %02x",\
				x.l,c->MacroStep-1,c->MacroNum);
		/* seems to work similar to 02... (not 100% sure, though) */
		c->AddBeginTime=0;
		c->CurAddr=x.l;
		break;
        case 0x23:
            //printf("TODO: SIDLengthMsg (0x23)\n");
                //fprintf(stderr,"Found code %08x at step %04x in macro %02x",\
				x.l,c->MacroStep-1,c->MacroNum);
		break;
        case 0x24:
            //printf("TODO: SID2OfsMsg (0x24)\n");
                //fprintf(stderr,"Found code %08x at step %04x in macro %02x",\
				x.l,c->MacroStep-1,c->MacroNum);
	        break;
        case 0x25:
            //printf("TODO: SID2VibMsg (0x25)\n");
                //fprintf(stderr,"Found code %08x at step %04x in macro %02x",\
				x.l,c->MacroStep-1,c->MacroNum);
	        break;
        case 0x26:
            //printf("TODO: SID1OfsMsg (0x26)\n");
                //fprintf(stderr,"Found code %08x at step %04x in macro %02x",\
				x.l,c->MacroStep-1,c->MacroNum);
	        break;
        case 0x27:
            //printf("TODO: SID1VibMsg (0x27)\n");
                //fprintf(stderr,"Found code %08x at step %04x in macro %02x",\
				x.l,c->MacroStep-1,c->MacroNum);
	        break;
        case 0x28:
            //printf("TODO: SIDFilterMsg (0x28)\n");
                //fprintf(stderr,"Found code %08x at step %04x in macro %02x",\
				x.l,c->MacroStep-1,c->MacroNum);
	        break;
        case 0x29:
            //printf("TODO: SIDStopMsg (0x29)\n");
                //fprintf(stderr,"Found code %08x at step %04x in macro %02x",\
				x.l,c->MacroStep-1,c->MacroNum);
	        break;
        case 0x30:
            //printf("TODO: ??? (0x30)\n");
                //fprintf(stderr,"Found code %08x at step %04x in macro %02x",\
				x.l,c->MacroStep-1,c->MacroNum);
	        break;
	case 0x31: /* turrican 3 title - we can safely ignore */
		break;
	default:
		break;
		c->MacroRun=0;
		return;
	}
	goto loop;
}

void DoEffects(Cdb *c){
	register int a=0;
	if (c->EfxRun<0) return;
	if (!c->EfxRun)	{
		c->EfxRun=1;
		return;
	}
	if (c->AddBeginTime)	{
		c->CurAddr+=c->AddBegin;
/*		if (c->SIDSize)
			c->SIDSrcSample=c->CurAddr;
		else*/
			c->SaveAddr=c->CurAddr;
		c->AddBeginTime--;
		if (!c->AddBeginTime)
		{
			c->AddBegin=-c->AddBegin;
			c->AddBeginTime=c->AddBeginReset;
		}
	}
/*
	if (c->SIDSize) {
		fputs("SID not supported\n",stderr);
		c->SIDSize=0;
	}
*/
	if (c->VibReset)	{
		a=(c->VibOffset+=c->VibWidth);
		a=(c->DestPeriod*(0x800+a))>>11;
		if (!c->PortaRate) c->CurPeriod=a;
		if (!(--c->VibTime))
		{
			c->VibTime=c->VibReset;
			c->VibWidth=-c->VibWidth;
		}
	}
	if ((c->PortaRate)&&((--c->PortaTime)==0))	{
		c->PortaTime=c->PortaReset;
		if (c->PortaPer>c->DestPeriod)		{
			a=(c->PortaPer*(256-c->PortaRate)-128)>>8;
			if (a<=c->DestPeriod)
				c->PortaRate=0;
		}
		else if (c->PortaPer<c->DestPeriod)
		{
			a=(c->PortaPer*(256+c->PortaRate))>>8;
			if (a>=c->DestPeriod)
				c->PortaRate=0;
		}
		else c->PortaRate=0;
		if (!c->PortaRate)
			a=c->DestPeriod;
		c->PortaPer=c->CurPeriod=a;
	}
	if ((c->EnvReset)&&(!(c->EnvTime--)))	{
		c->EnvTime=c->EnvReset;
		if (c->CurVol > c->EnvEndvol)		{
			if (c->CurVol<c->EnvRate) c->EnvReset=0; else
			c->CurVol -= c->EnvRate;
			if (c->EnvEndvol > c->CurVol)
				c->EnvReset=0;
		}
		else if (c->CurVol < c->EnvEndvol)		{
			c->CurVol += c->EnvRate;
			if (c->EnvEndvol < c->CurVol)
				c->EnvReset=0;
		}
		if (!c->EnvReset)		{
				c->EnvReset=c->EnvTime=0;
				c->CurVol=c->EnvEndvol;
		}
	}
/*	if (c->ArpRun) {
		fputs("Arpeggio/randomplay not supported\n",stderr);
		c->ArpRun=0;
	}
*/
    if ((TFMX_mdb.FadeSlope)&&((--TFMX_mdb.FadeTime)==0))	{
        TFMX_mdb.FadeTime = TFMX_mdb.FadeReset;
        TFMX_mdb.MasterVol += TFMX_mdb.FadeSlope;
        if (TFMX_mdb.FadeDest == TFMX_mdb.MasterVol) TFMX_mdb.FadeSlope=0;
	}
}

void DoMacro(int cc){
    Cdb *c=&TFMX_cdb[cc];

	int a;int nRun;int nWait;
/* locking */
	if (c->SfxLockTime>=0)
		c->SfxLockTime--;
	else
		c->SfxFlag=c->SfxPriority=0;
	
	a=c->SfxCode;
	if (a)
	{
		c->SfxFlag=c->SfxCode=0;
		NotePort(a);
		c->SfxFlag=c->SfxPriority;
	}
    //DEBUG(3)
    //printf("%01x:\t",cc);
	
	/*if ((c->MacroWait)&&(!(c->MacroWait--)))*/
	
	/* FIXME with weird Z-Out theme,
	c->MacroRun and c->MacroWait differ sometimes from
	the correct values, when run on Mac OS X 
		
	c->MacroRun: S8
	c->MacroWait: U16
	*/
	nRun=c->MacroRun;
	nWait=c->MacroWait;
	c->MacroWait=c->MacroWait-1;
		
	if ((nRun)&&(!(nWait)))	{
        RunMacro(c, cc);
	}
    else {
	}
	DoEffects(c);
	/* has to be here because of if(efxrun=1) */
    c->hw->delta=(c->CurPeriod)?(3579545<<9)/(c->CurPeriod * g_tfmx.outRate>>5):0;
    c->hw->SampleStart=&TFMX_smplbuf[c->SaveAddr];
	c->hw->SampleLength=(c->SaveLen)?c->SaveLen<<1:131072;
	if ((c->hw->mode&3)==1)
	{
		c->hw->sbeg=c->hw->SampleStart;
		c->hw->slen=c->hw->SampleLength;
	}
    c->hw->vol=(c->CurVol * TFMX_mdb.MasterVol)>>6;
}

void DoAllMacros(){
	DoMacro(0);
	DoMacro(1);
	DoMacro(2);
    if (g_tfmx.multimode)	{
		DoMacro(4);
		DoMacro(5);
		DoMacro(6);
		DoMacro(7);
	} /* else -- DoMacro(3) should always run so fade speed is right */
	DoMacro(3);
}

void ChannelOff(int i){
    Cdb *c;
    c = &TFMX_cdb[i&0xF];
	if (!c->SfxFlag)	{
		c->hw->mode=0;
        c->AddBeginTime=c->AddBeginReset=c->MacroRun=/*c->SIDSize=c->ArpRun=*/0;
		c->NewStyleMacro=0xFF;
		c->SaveAddr=c->CurVol=c->hw->vol=0;
		c->SaveLen=c->CurrLength=1;
		c->hw->loop=&LoopOff;
		c->hw->c=c;
	}
}

void DoFade(int sp,int dv){
    TFMX_mdb.FadeDest=dv;
    if (!(TFMX_mdb.FadeTime = TFMX_mdb.FadeReset=sp)||(TFMX_mdb.MasterVol==sp))	{
        TFMX_mdb.MasterVol=dv;
        TFMX_mdb.FadeSlope=0;
		return;
	}
    TFMX_mdb.FadeSlope = (TFMX_mdb.MasterVol > TFMX_mdb.FadeDest)?-1:1;
}

void GetTrackStep(){
    uint16_t *l;
	int x,y;
	loop:
	/* Fixed by Sven Janssen 15 August 2004 */
    if ((TFMX_pdb.CurrPos == TFMX_pdb.FirstPos) && (g_tfmx.loops<=0)){
        if (g_tfmx.loops<0){
            TFMX_mdb.PlayerEnable=0;
			return;
		}
        g_tfmx.loops--;
	}

    l=(uint16_t *)&g_tfmx.editbuf[TFMX_hdr.trackstart+(TFMX_pdb.CurrPos*4)];
    g_tfmx.jiffies=0;
    if ((l[0])==0xEFFE){
		switch (l[1]) {
		case 0: /* stop */
            TFMX_mdb.PlayerEnable=0;
			return;
		case 1: /* loop */
            if (g_tfmx.loops){
                if (!(--g_tfmx.loops)){
                    TFMX_mdb.PlayerEnable=0;
					return;
				}
			}
            if (!(TFMX_mdb.TrackLoop--)){
                TFMX_mdb.TrackLoop=-1;
                TFMX_pdb.CurrPos++;
				goto loop;
			}
            else if (TFMX_mdb.TrackLoop<0)
                TFMX_mdb.TrackLoop=l[3];
            TFMX_pdb.CurrPos=l[2];
			goto loop;
		case 2: /* speed */ 
            TFMX_mdb.SpeedCnt = TFMX_pdb.Prescale=l[2];
            if (!(l[3]&0xF200)&&(x=(l[3]&0x1FF)>0xF))
                TFMX_mdb.CIASave = g_tfmx.eClocks=0x1B51F8/x;
            TFMX_pdb.CurrPos++;
			goto loop;
		case 3: /* timeshare */
			if (!((x=l[3])&0x8000))
			{
				x=((char)x)<-0x20?-0x20:(char)x;
                TFMX_mdb.CIASave = g_tfmx.eClocks=(14318*(x+100))/100;
                g_tfmx.multimode=1;
			} /* else multimode=0;*/
            TFMX_pdb.CurrPos++;
			goto loop;
		case 4: /* fade */
			DoFade(l[2]&0xFF,l[3]&0xFF);
            TFMX_pdb.CurrPos++;
			goto loop;
		default:
            TFMX_pdb.CurrPos++;
			goto loop;
		}
	}
	else
	{
		for (x=0;x<8;x++)		{
            TFMX_pdb.p[x].PXpose = (int)(l[x]&0xff);
            if ((y = TFMX_pdb.p[x].PNum = (l[x]>>8))<0x80) {
                TFMX_pdb.p[x].PStep = 0;
                TFMX_pdb.p[x].PWait = 0;
                TFMX_pdb.p[x].PLoop = 0xFFFF;
                TFMX_pdb.p[x].PAddr = TFMX_patterns[y];
			}
		}
	}
}

int DoTrack(Pdb *p/* ,int pp */){
	UNI x;
	int t;
	if (p->PNum==0xFE)
	{
		p->PNum++;
		ChannelOff(p->PXpose);
		return(0);
	}
	if (!p->PAddr) return(0);
	if (p->PNum>=0x90) return(0);
	if (p->PWait--) return(0);
	while(1)
	{
		loop:
        x.l=tfmx_be32(g_tfmx.editbuf[p->PAddr+p->PStep++]);
		t=x.b.b0;
		/*printf("%x: %02x:%02x:%02x:%02x (%04x)\n",pp,t,x.b.b1,x.b.b2,
		       x.b.b3,jiffies);*/
		if (t<0xF0)
		{
			fflush(stdout);
			if ((t&0xC0)==0x80)
			{
				p->PWait=x.b.b3;
				x.b.b3=0;
			}
			x.b.b0=((t+p->PXpose)&0x3F);
			if ((t&0xC0)==0xC0)
				x.b.b0|=0xC0;
			NotePort(x.l);
			if ((t&0xC0)==0x80)
				return(0);
			goto loop;
		}
		switch (t&0xF)
		{
		case 15: /* NOP */
			break;
		case 0:	/* End */
			p->PNum=0xFF;
            TFMX_pdb.CurrPos = (TFMX_pdb.CurrPos == TFMX_pdb.LastPos)?TFMX_pdb.FirstPos:TFMX_pdb.CurrPos+1;
			GetTrackStep();
			return(1);
		case 1:
			if (!(p->PLoop))
			{
				p->PLoop=0xFFFF;
				break;
			}
			else if (p->PLoop==0xFFFF) /* FF --'ed */
				p->PLoop=x.b.b1;
			p->PLoop--;
			p->PStep=x.w.w1;
			break;
		case 8: /* GsPt */
			p->PRoAddr=p->PAddr;
			p->PRoStep=p->PStep;
			/* fall through to... */
		case 2: /* Cont */
            p->PAddr = TFMX_patterns[x.b.b1];
			p->PStep=x.w.w1;
			break;
		case 3: /* Wait */
			p->PWait=x.b.b1;
			return(0);
		case 14: /* StCu */
            TFMX_mdb.PlayPattFlag = 0;
		case 4: /* Stop */
            p->PNum = 0xFF;
			return(0);
		case 5: /* Kup^ */
		case 6: /* Vibr */
		case 7: /* Enve */
		case 12: /* Lock */
			NotePort(x.l);
			break;
		case 9: /* RoPt */
			p->PAddr=p->PRoAddr;
			p->PStep=p->PRoStep;
			break;
		case 10: /* Fade */
			DoFade(x.b.b1,x.b.b3);
			break;
		case 13: /* Cue */
            TFMX_idb.Cue[x.b.b1&0x03] = x.w.w1;
			break;
		case 11: /* PPat */
            t = x.b.b2 & 0x07;
            TFMX_pdb.p[t].PNum = x.b.b1;
            TFMX_pdb.p[t].PAddr = TFMX_patterns[x.b.b1];
            TFMX_pdb.p[t].PXpose = x.b.b3;
            TFMX_pdb.p[t].PStep = 0;
            TFMX_pdb.p[t].PWait = 0;
            TFMX_pdb.p[t].PLoop = 0xFFFF;
			break;
		}
	}
}

void DoTracks(){
	int x;
    g_tfmx.jiffies++;
    if (!TFMX_mdb.SpeedCnt--)	{
        TFMX_mdb.SpeedCnt = TFMX_pdb.Prescale;
		/* sortof fix Oops Up tempo */
        if (g_tfmx.oopsUpHack){
            TFMX_mdb.SpeedCnt=5;
		}

        for (x=0;x<8;x++){
            if (DoTrack(&TFMX_pdb.p[x]/* ,x */)){
				x=-1;
				continue;
			}
		}
	}
}

void tfmxIrqIn(){
    if (!TFMX_mdb.PlayerEnable) return;
	DoAllMacros();
    if (TFMX_mdb.CurrSong>=0) DoTracks();
}

void AllOff(){
	int x;
    Cdb *c;
    TFMX_mdb.PlayerEnable = 0;
	for (x=0;x<8;x++) {
        c = &TFMX_cdb[x];
        c->hw = &TFMX_hdb[x];
        c->hw->c = c;	/* wait on dma */
        TFMX_hdb[x].mode = 0;
		
        c->MacroWait=c->MacroRun=c->SfxFlag=/*c->SIDSize=c->ArpRun=*/c->CurVol = c->SfxFlag=c->SfxCode=c->SaveAddr=0;

        TFMX_hdb[x].vol = 0;
        c->Loop = c->NewStyleMacro = c->SfxLockTime = -1;
        c->hw->sbeg = c->hw->SampleStart = TFMX_smplbuf;
        c->hw->SampleLength = c->hw->slen = c->SaveLen = 2;
		c->hw->loop=&LoopOff;
	}
}

void TfmxInit(){
	int x;
	AllOff();
	for (x=0;x<8;x++) {
        TFMX_hdb[x].c = &TFMX_cdb[x];
        TFMX_pdb.p[x].PNum  = 0xFF;
        TFMX_pdb.p[x].PAddr = 0;
		ChannelOff(x);
	}
	return;
}

void StartSong(int song, int mode){
	int x;
    TFMX_mdb.PlayerEnable = 0; /* sort of locking mechanism */
    TFMX_mdb.MasterVol    = 0x40;
    TFMX_mdb.FadeSlope    = 0;
    TFMX_mdb.TrackLoop    = -1;
    TFMX_mdb.PlayPattFlag = 0;
    TFMX_mdb.CIASave = g_tfmx.eClocks = 14318; /* assume 125bpm, NTSC timing */
	if (mode!=2) {
        TFMX_pdb.CurrPos = TFMX_pdb.FirstPos = TFMX_hdr.start[song];
        TFMX_pdb.LastPos = TFMX_hdr.end[song];
        if ((x = TFMX_hdr.tempo[song])>=0x10){
                TFMX_mdb.CIASave = g_tfmx.eClocks = 0x1B51F8/x;
                TFMX_pdb.Prescale = 0;
		}
		else
                TFMX_pdb.Prescale = x;
	}
	for (x=0;x<8;x++) {
        TFMX_pdb.p[x].PAddr = 0;
        TFMX_pdb.p[x].PNum = 0xFF;
        TFMX_pdb.p[x].PXpose = 0;
        TFMX_pdb.p[x].PStep = 0;
	}
    if (mode != 2) GetTrackStep();
    if (g_tfmx.startPat != -1) {
        TFMX_pdb.CurrPos = TFMX_pdb.FirstPos = g_tfmx.startPat;
		GetTrackStep();
        g_tfmx.startPat = -1;
	}
    TFMX_mdb.SpeedCnt = TFMX_mdb.EndFlag = 0;
    TFMX_mdb.PlayerEnable = 1;
}
