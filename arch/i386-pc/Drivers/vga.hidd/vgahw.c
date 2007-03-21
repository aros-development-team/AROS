/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VGA Inside...
    Lang: English.
*/

/*
    NOTE: This file is mostly based on XFree86 vgaWH.c sources
*/

/****************************************************************************************/

#include <proto/exec.h>
#include <oop/oop.h>
#include "vgahw.h"
#include "vgaclass.h"
#include "bitmap.h"

/****************************************************************************************/

#undef SysBase
#define SysBase (*(struct ExecBase **)4UL)

/****************************************************************************************/

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

/****************************************************************************************/

/* Default ANSI (PC's) palette */

unsigned char vgaANSIPalette[]=
{
     0,  0,  0,    0,  0, 42,    0, 42,  0,    0, 42, 42,
    42,  0,  0,   42,  0, 42,   42, 21,  0,   42, 42, 42,
/*  21, 21, 21,   21, 21, 63,   21, 63, 21,   21, 63, 63,
    A temporary measure which prevents annoying effect of
    a pointer appearing in strange colors before the first
    screen opens.
    In fact the pointer should appear only upon opening of
    the first screen. */
    21, 21, 21,  224, 64, 64,    0,  0,  0,  224,224,192, 
    63, 21, 21,   63, 21, 63,   63, 63, 21,   63, 63, 63,
     0,  0,  0,    5,  5,  5,    8,  8,  8,   11, 11, 11,
    14, 14, 14,   17, 17, 17,   20, 20, 20,   24, 24, 24,
    28, 28, 28,   32, 32, 32,   36, 36, 36,   40, 40, 40,
    45, 45, 45,   50, 50, 50,   56, 56, 56,   63, 63, 63,
     0,  0, 63,   16,  0, 63,   31,  0, 63,   47,  0, 63,
    63,  0, 63,   63,  0, 47,   63,  0, 31,   63,  0, 16,
    63,  0,  0,   63, 16,  0,   63, 31,  0,   63, 47,  0,
    63, 63,  0,   47, 63,  0,   31, 63,  0,   16, 63,  0,
     0, 63,  0,    0, 63, 16,    0, 63, 31,    0, 63, 47,
     0, 63, 63,    0, 47, 63,    0, 31, 63,    0, 16, 63,
    31, 31, 63,   39, 31, 63,   47, 31, 63,   55, 31, 63,
    63, 31, 63,   63, 31, 55,   63, 31, 47,   63, 31, 39,
    63, 31, 31,   63, 39, 31,   63, 47, 31,   63, 55, 31,
    63, 63, 31,   55, 63, 31,   47, 63, 31,   39, 63, 31,
    31, 63, 31,   31, 63, 39,   31, 63, 47,   31, 63, 55,
    31, 63, 63,   31, 55, 63,   31, 47, 63,   31, 39, 63,
    45, 45, 63,   49, 45, 63,   54, 45, 63,   58, 45, 63,
    63, 45, 63,   63, 45, 58,   63, 45, 54,   63, 45, 49,
    63, 45, 45,   63, 49, 45,   63, 54, 45,   63, 58, 45,
    63, 63, 45,   58, 63, 45,   54, 63, 45,   49, 63, 45,
    45, 63, 45,   45, 63, 49,   45, 63, 54,   45, 63, 58,
    45, 63, 63,   45, 58, 63,   45, 54, 63,   45, 49, 63,
     0,  0, 28,    7,  0, 28,   14,  0, 28,   21,  0, 28,
    28,  0, 28,   28,  0, 21,   28,  0, 14,   28,  0,  7,
    28,  0,  0,   28,  7,  0,   28, 14,  0,   28, 21,  0,
    28, 28,  0,   21, 28,  0,   14, 28,  0,    7, 28,  0,
     0, 28,  0,    0, 28,  7,    0, 28, 14,    0, 28, 21,
     0, 28, 28,    0, 21, 28,    0, 14, 28,    0,  7, 28,
    14, 14, 28,   17, 14, 28,   21, 14, 28,   24, 14, 28,
    28, 14, 28,   28, 14, 24,   28, 14, 21,   28, 14, 17,
    28, 14, 14,   28, 17, 14,   28, 21, 14,   28, 24, 14,
    28, 28, 14,   24, 28, 14,   21, 28, 14,   17, 28, 14,
    14, 28, 14,   14, 28, 17,   14, 28, 21,   14, 28, 24,
    14, 28, 28,   14, 24, 28,   14, 21, 28,   14, 17, 28,
    20, 20, 28,   22, 20, 28,   24, 20, 28,   26, 20, 28,
    28, 20, 28,   28, 20, 26,   28, 20, 24,   28, 20, 22,
    28, 20, 20,   28, 22, 20,   28, 24, 20,   28, 26, 20,
    28, 28, 20,   26, 28, 20,   24, 28, 20,   22, 28, 20,
    20, 28, 20,   20, 28, 22,   20, 28, 24,   20, 28, 26,
    20, 28, 28,   20, 26, 28,   20, 24, 28,   20, 22, 28,
     0,  0, 16,    4,  0, 16,    8,  0, 16,   12,  0, 16,
    16,  0, 16,   16,  0, 12,   16,  0,  8,   16,  0,  4,
    16,  0,  0,   16,  4,  0,   16,  8,  0,   16, 12,  0,
    16, 16,  0,   12, 16,  0,    8, 16,  0,    4, 16,  0,
     0, 16,  0,    0, 16,  4,    0, 16,  8,    0, 16, 12,
     0, 16, 16,    0, 12, 16,    0,  8, 16,    0,  4, 16,
     8,  8, 16,   10,  8, 16,   12,  8, 16,   14,  8, 16,
    16,  8, 16,   16,  8, 14,   16,  8, 12,   16,  8, 10,
    16,  8,  8,   16, 10,  8,   16, 12,  8,   16, 14,  8,
    16, 16,  8,   14, 16,  8,   12, 16,  8,   10, 16,  8,
     8, 16,  8,    8, 16, 10,    8, 16, 12,    8, 16, 14,
     8, 16, 16,    8, 14, 16,    8, 12, 16,    8, 10, 16,
    11, 11, 16,   12, 11, 16,   13, 11, 16,   15, 11, 16,
    16, 11, 16,   16, 11, 15,   16, 11, 13,   16, 11, 12,
    16, 11, 11,   16, 12, 11,   16, 13, 11,   16, 15, 11,
    16, 16, 11,   15, 16, 11,   13, 16, 11,   12, 16, 11,
    11, 16, 11,   11, 16, 12,   11, 16, 13,   11, 16, 15,
    11, 16, 16,   11, 15, 16,   11, 13, 16,   11, 12, 16,
     0,  0,  0,    0,  0,  0,    0,  0,  0,    0,  0,  0,
     0,  0,  0,    0,  0,  0,    0,  0,  0,    0,  0,  0,
};

/****************************************************************************************/

/*
** Load specified palette, if NULL load default
*/
void vgaLoadPalette(struct vgaHWRec *regs, unsigned char *pal)
{
    int i;
    
    if (!pal)
	pal = (unsigned char *)&vgaANSIPalette;

    for (i=0; i<768; i++)
    {
	regs->DAC[i]=*(unsigned char*)pal++;
    }
}

/****************************************************************************************/

#define SS_START	1
#define SS_FINISH	0

/****************************************************************************************/

/*
** vgaHWSaveScreen
**      perform a sequencer reset.
*/
void vgaSaveScreen(int start)
{
    if (start == SS_START)
    {
	outw(0x3C4, 0x0100);        /* synchronous reset */
    }
    else
    {
	outw(0x3C4, 0x0300);        /* end reset */
    }
}

/****************************************************************************************/

/*
** Blank the screen.
*/
int vgaBlankScreen(int on)
{
    unsigned char scrn;

    outb(0x3C4,1);
    scrn = inb(0x3C5);

    if(on)
    {
	scrn &= 0xDF;			/* enable screen */
    }
    else
    {
	scrn |= 0x20;			/* blank screen */
    }

    vgaSaveScreen(SS_START);
    outw(0x3C4, (scrn << 8) | 0x01); /* change mode */
    vgaSaveScreen(SS_FINISH);
    
    return TRUE;
}

/****************************************************************************************/

#define vgaIOBase	0x3d0

/****************************************************************************************/

/*
** vgaDACLoad --
**      load the DAC
*/
void vgaDACLoad(struct vgaHWRec *restore, unsigned char start, int num)
{
    int i, n;

    n = start * 3;
    outb(0x3C8,start);
    for (i=0; i<num*3; i++)
    {
	outb(0x3C9, restore->DAC[n++]);
/*	DACDelay;
	I beleive incrementing variables and checking cycle counter
	provides enough delay. Uncomment this in case of problems */
    }
}

/*
** vgaRestore --
**      restore a video mode
*/
void vgaRestore(struct vgaHWRec *restore, BOOL onlyDac)
{
    int i,tmp;

    if (!onlyDac)
    {
    tmp = inb(vgaIOBase + 0x0A);		/* Reset flip-flop */
    outb(0x3C0, 0x00);			/* Enables pallete access */

    restore->MiscOutReg |= 0x01;
    outb(0x3C2, restore->MiscOutReg);

    for (i=1; i<5;  i++) outw(0x3C4, (restore->Sequencer[i] << 8) | i);

    /* Ensure CRTC registers 0-7 are unlocked by clearing bit 7 or CRTC[17] */

    outw(vgaIOBase + 4, ((restore->CRTC[17] & 0x7F) << 8) | 17);

    for (i=0; i<25; i++) outw(vgaIOBase + 4,(restore->CRTC[i] << 8) | i);

    for (i=0; i<9;  i++) outw(0x3CE, (restore->Graphics[i] << 8) | i);

    for (i=0; i<21; i++) {
	tmp = inb(vgaIOBase + 0x0A);
	outb(0x3C0,i); outb(0x3C0, restore->Attribute[i]);
    }

    outb(0x3C6,0xFF);
    }

    vgaDACLoad(restore, 0, 256);

    if (!onlyDac)
    {
    /* Turn on PAS bit */
    tmp = inb(vgaIOBase + 0x0A);
    outb(0x3C0, 0x20);
    }
}

/****************************************************************************************/

/*
** vgaSave --
**      save the current video mode
*/
void * vgaSave(struct vgaHWRec *save)
{
    int	i,tmp;
    int	first_time = FALSE;	/* Should be static? */

    /*
     * Here we are, when we first save the videostate. This means we came here
     * to save the original Text mode. Because some drivers may depend
     * on NoClock we set it here to a resonable value.
     */
    first_time = TRUE;
    save->NoClock = (inb(0x3CC) >> 2) & 3;
    save->MiscOutReg = inb(0x3CC);

    tmp = inb(vgaIOBase + 0x0A); /* reset flip-flop */
    outb(0x3C0, 0x00);

    /*			 
     * save the colorlookuptable 
     */
    outb(0x3C7,0x01);
    for (i=3; i<768; i++)
    {
	save->DAC[i] = inb(0x3C9); 
	DACDelay;
    }

    for (i=0; i<25; i++)
    {
    	outb(vgaIOBase + 4,i);
	save->CRTC[i] = inb(vgaIOBase + 5);
    }

    for (i=0; i<21; i++)
    {
	tmp = inb(vgaIOBase + 0x0A);
	outb(0x3C0,i);
	save->Attribute[i] = inb(0x3C1);
    }

    for (i=0; i<9;  i++)
    {
    	outb(0x3CE,i);
	save->Graphics[i]  = inb(0x3CF);
    }

    for (i=0; i<5;  i++)
    {
    	outb(0x3C4,i);
	save->Sequencer[i] = inb(0x3C5);
    }

    tmp = inb(vgaIOBase + 0x0A);
    outb(0x3C0, 0x20);
    
    return ((void *) save);
}

/****************************************************************************************/

/*
** Init VGA registers for given displaymode
*/
int vgaInitMode(struct vgaModeDesc *mode, struct vgaHWRec *regs)
{
    unsigned int    i;
    int     	    hblankstart;
    int     	    hblankend;
    int     	    vblankstart;
    int     	    vblankend;
    
    /*
	initialize default colormap for monochrome
    */
    for (i = 0; i < 3;   i++) regs->DAC[i] = 0x00;
    for (i = 3; i < 768; i++) regs->DAC[i] = 0x3F;

    /* Initialise overscan register */
    regs->Attribute[17] = 0xFF;

    regs->NoClock = mode->clock;

    /*
	compute correct Hsync & Vsync polarity 
    */
    {
    	int VDisplay = mode->Height;
//	if (mode->Flags & V_DBLSCAN)
//	  VDisplay *= 2;
	if      (VDisplay < 400)
		regs->MiscOutReg = 0xA3;	/* +hsync -vsync */
	else if (VDisplay < 480)
		regs->MiscOutReg = 0x63;	/* -hsync +vsync */
	else if (VDisplay < 768)
		regs->MiscOutReg = 0xE3;	/* -hsync -vsync  0xE3 */
	else
		regs->MiscOutReg = 0x23;	/* +hsync +vsync */
    }

    regs->MiscOutReg |= (regs->NoClock & 0x03) << 2;
    
    /*
	Time Sequencer
    */
    regs->Sequencer[0] = 0x02;
//    if (mode->Flags & V_CLKDIV2) 	/* TODO */
//	regs->Sequencer[1] = 0x09;
//    else
	regs->Sequencer[1] = 0x01;
    
    regs->Sequencer[2] = 0x0F;
    regs->Sequencer[3] = 0x00;                             /* Font select */
    regs->Sequencer[4] = 0x06;                             /* Misc */

    /*
	CRTC Controller
    */
    
    hblankstart = min(mode->HSyncStart, mode->HDisplay);
    hblankend   = max(mode->HSyncEnd, mode->HTotal);
    if ((hblankend - hblankstart) >= 63 * 8)
    {
    	hblankstart = hblankend - 63 * 8;
    }
    
    vblankstart = min(mode->VSyncStart, mode->VDisplay);
    vblankend   = max(mode->VSyncEnd, mode->VTotal);
    if ((vblankend - vblankstart) >= 127)
    {
    	vblankstart = vblankend - 127;
    }
    
    regs->CRTC[CRTC_H_TOTAL]  	    = (mode->HTotal / 8) - 5;
    regs->CRTC[CRTC_H_DISPLAY]      = (mode->HDisplay / 8) - 1;
    regs->CRTC[CRTC_H_BLANK_START]  = (hblankstart / 8) -1;
    regs->CRTC[CRTC_H_BLANK_END]    = (((hblankend / 8) - 1) & 0x1F) | 0x80;
    
    i = (((mode->HSkew << 2) + 0x10) & ~0x1F);
    if (i < 0x80)
    {
	regs->CRTC[CRTC_H_BLANK_END] |= i;
    }
    
    regs->CRTC[CRTC_H_SYNC_START]   = (mode->HSyncStart / 8);
    regs->CRTC[CRTC_H_SYNC_END]     = ((((hblankend / 8) - 1) & 0x20 ) << 2 ) |
	    	    	    	      (((mode->HSyncEnd / 8)) & 0x1F);
    regs->CRTC[CRTC_V_TOTAL]  	    = (mode->VTotal - 2) & 0xFF;
    regs->CRTC[CRTC_OVERFLOW]       = (((mode->VTotal -2) & 0x100) >> 8 )   |
	    	    	    	      (((mode->VDisplay -1) & 0x100) >> 7 ) |
            	    	    	      ((mode->VSyncStart & 0x100) >> 6 )    |
	    	    	    	      (((vblankstart - 1) & 0x100) >> 5 )   |
	      	    	    	      0x10  	    	    	    	    |
		    	    	      (((mode->VTotal -2) & 0x200)   >> 4 ) |
	            	    	      (((mode->VDisplay -1) & 0x200) >> 3 ) |
		    	    	      ((mode->VSyncStart & 0x200) >> 2 );
    regs->CRTC[CRTC_PRESET_ROW]     = 0x00;
    regs->CRTC[CRTC_MAX_SCAN]       = (((vblankstart - 1) & 0x200) >>4) | 0x40;
//    if (mode->Flags & V_DBLSCAN)
//	new->CRTC[9] |= 0x80;
    regs->CRTC[CRTC_CURSOR_START]   = 0x00;
    regs->CRTC[CRTC_CURSOR_END]     = 0x00;
    regs->CRTC[CRTC_START_HI] 	    = 0x00;
    regs->CRTC[CRTC_START_LO] 	    = 0x00;
    regs->CRTC[CRTC_CURSOR_HI]      = 0x00;
    regs->CRTC[CRTC_CURSOR_LO]      = 0x00;
    regs->CRTC[CRTC_V_SYNC_START]   = mode->VSyncStart & 0xFF;
    regs->CRTC[CRTC_V_SYNC_END]     = (mode->VSyncEnd & 0x0F) | 0x20;
    regs->CRTC[CRTC_V_DISP_END]     = (mode->VDisplay -1) & 0xFF;
    regs->CRTC[CRTC_OFFSET] 	    = mode->Width >> (8 - mode->Depth);  /* just a guess */
    regs->CRTC[CRTC_UNDERLINE]      = 0x00;
    regs->CRTC[CRTC_V_BLANK_START]  = (vblankstart - 1) & 0xFF; 
    regs->CRTC[CRTC_V_BLANK_END]    = (vblankend - 1) & 0xFF;
    regs->CRTC[CRTC_MODE]   	    = 0xE3;
    regs->CRTC[CRTC_LINE_COMPARE]   = 0xFF;

    if ((hblankend / 8) == (mode->HTotal / 8))
    {
    	i = (regs->CRTC[CRTC_H_BLANK_END] & 0x1f) | ((regs->CRTC[CRTC_H_SYNC_END] & 0x80) >> 2);
	if ((i-- > (regs->CRTC[CRTC_H_BLANK_START] & 0x3F)) && 
	    (hblankend == mode->HTotal))
	{
	    i = 0;
	}
	
	regs->CRTC[CRTC_H_BLANK_END] = (regs->CRTC[CRTC_H_BLANK_END] & ~0x1F) | (i & 0x1f);
	regs->CRTC[CRTC_H_SYNC_END]  = (regs->CRTC[CRTC_H_SYNC_END] & ~0x80) | ((i << 2) & 0x80);
    }
    
    if (vblankend == mode->VTotal)
    {
    	i = regs->CRTC[CRTC_V_BLANK_END];
	if ((i > regs->CRTC[CRTC_V_BLANK_START]) &&
	    ((i & 0x7F) > (regs->CRTC[CRTC_V_BLANK_START] & 0x7F)) &&
	    !(regs->CRTC[CRTC_MAX_SCAN] & 0x9f))
	{
	    i = 0;
	}
	else
	{
	    i = (UBYTE)(i - 1);
	}
	regs->CRTC[CRTC_V_BLANK_END] = i;
    }
    
    /*
	Graphics Display Controller
    */
    regs->Graphics[0] = 0x00;
    regs->Graphics[1] = 0x00;
    regs->Graphics[2] = 0x00;
    regs->Graphics[3] = 0x00;
    regs->Graphics[4] = 0x00;
    regs->Graphics[5] = 0x00;
    regs->Graphics[6] = 0x05;   /* only map 64k VGA memory !!!! */
    regs->Graphics[7] = 0x0F;
    regs->Graphics[8] = 0xFF;

    regs->Attribute[0]  = 0x00; /* standard colormap translation */
    regs->Attribute[1]  = 0x01;
    regs->Attribute[2]  = 0x02;
    regs->Attribute[3]  = 0x03;
    regs->Attribute[4]  = 0x04;
    regs->Attribute[5]  = 0x05;
    regs->Attribute[6]  = 0x06;
    regs->Attribute[7]  = 0x07;
    regs->Attribute[8]  = 0x08;
    regs->Attribute[9]  = 0x09;
    regs->Attribute[10] = 0x0A;
    regs->Attribute[11] = 0x0B;
    regs->Attribute[12] = 0x0C;
    regs->Attribute[13] = 0x0D;
    regs->Attribute[14] = 0x0E;
    regs->Attribute[15] = 0x0F;
    regs->Attribute[16] = 0x81; /* wrong for the ET4000 */
    regs->Attribute[17] = 0x00; /* GJA -- overscan. */
    /*
	Attribute[17] is the overscan, and is initalised above only at startup
	time, and not when mode switching.
    */
    regs->Attribute[18] = 0x0F;
    regs->Attribute[19] = 0x00;
    regs->Attribute[20] = 0x00;

    return TRUE;
}

/****************************************************************************************/

void vgaRefreshArea(struct bitmap_data *bmap, int num, struct Box *pbox)
{
    int     	    	    width, height, FBPitch, left, right, i, j, SRCPitch, phase;
    register unsigned long  m;
    unsigned char   	    s1, s2, s3, s4;
    unsigned long   	    *src, *srcPtr;
    unsigned char   	    *dst, *dstPtr;
   
    FBPitch  = bmap->width >> 3;
    SRCPitch = bmap->width >> 2;

    outw(0x3ce, 0x0005);
    outw(0x3ce, 0x0001);
    outw(0x3ce, 0x0000);
    outw(0x3ce, 0x0003);
    outw(0x3ce, 0xff08);

    left = pbox->x1 & ~7;
    right = (pbox->x2 & ~7) + 7;

    while(num--)
    {
        width  = (right - left + 1) >> 3;
        height = pbox->y2 - pbox->y1 + 1;
        src    = (unsigned long*)bmap->VideoData + (pbox->y1 * SRCPitch) + (left >> 2); 
        dst    = (unsigned char*)0x000a0000 + (pbox->y1 * FBPitch) + (left >> 3);

	if((phase = ((long)dst & 3L)))
	{
	    phase = 4 - phase;
	    if(phase > width) phase = width;
	    width -= phase;
	}

        while(height--)
	{
	    outw(0x3c4, 0x0102);	//pvgaHW->writeSeq(pvgaHW, 0x02, 1);
	    dstPtr = dst;
	    srcPtr = src;
	    i = width;
	    j = phase;
	    
	    while(j--)
	    {
		m = (srcPtr[1] & 0x01010101) | ((srcPtr[0] & 0x01010101) << 4);
 		*dstPtr++ = (m >> 24) | (m >> 15) | (m >> 6) | (m << 3);
		srcPtr += 2;
	    }
	    
	    while(i >= 4)
	    {
		m = (srcPtr[1] & 0x01010101) | ((srcPtr[0] & 0x01010101) << 4);
 		s1 = (m >> 24) | (m >> 15) | (m >> 6) | (m << 3);
		m = (srcPtr[3] & 0x01010101) | ((srcPtr[2] & 0x01010101) << 4);
 		s2 = (m >> 24) | (m >> 15) | (m >> 6) | (m << 3);
		m = (srcPtr[5] & 0x01010101) | ((srcPtr[4] & 0x01010101) << 4);
 		s3 = (m >> 24) | (m >> 15) | (m >> 6) | (m << 3);
		m = (srcPtr[7] & 0x01010101) | ((srcPtr[6] & 0x01010101) << 4);
 		s4 = (m >> 24) | (m >> 15) | (m >> 6) | (m << 3);
		*((unsigned long*)dstPtr) = s1 | (s2 << 8) | (s3 << 16) | (s4 << 24);
		srcPtr += 8;
		dstPtr += 4;
		i -= 4;
	    }
	    
	    while(i--)
	    {
		m = (srcPtr[1] & 0x01010101) | ((srcPtr[0] & 0x01010101) << 4);
 		*dstPtr++ = (m >> 24) | (m >> 15) | (m >> 6) | (m << 3);
		srcPtr += 2;
	    }

	    outw(0x3c4, 0x0202);	//pvgaHW->writeSeq(pvgaHW, 0x02, 1 << 1);
	    dstPtr = dst;
	    srcPtr = src;
	    i = width;
	    j = phase;
	    
	    while(j--)
	    {
		m = (srcPtr[1] & 0x02020202) | ((srcPtr[0] & 0x02020202) << 4);
 		*dstPtr++ = (m >> 25) | (m >> 16) | (m >> 7) | (m << 2);
		srcPtr += 2;
	    }
	    
	    while(i >= 4)
	    {
		m = (srcPtr[1] & 0x02020202) | ((srcPtr[0] & 0x02020202) << 4);
 		s1 = (m >> 25) | (m >> 16) | (m >> 7) | (m << 2);
		m = (srcPtr[3] & 0x02020202) | ((srcPtr[2] & 0x02020202) << 4);
 		s2 = (m >> 25) | (m >> 16) | (m >> 7) | (m << 2);
		m = (srcPtr[5] & 0x02020202) | ((srcPtr[4] & 0x02020202) << 4);
 		s3 = (m >> 25) | (m >> 16) | (m >> 7) | (m << 2);
		m = (srcPtr[7] & 0x02020202) | ((srcPtr[6] & 0x02020202) << 4);
 		s4 = (m >> 25) | (m >> 16) | (m >> 7) | (m << 2);
		*((unsigned long*)dstPtr) = s1 | (s2 << 8) | (s3 << 16) | (s4 << 24);
		srcPtr += 8;
		dstPtr += 4;
		i -= 4;
	    }
	    
	    while(i--)
	    {
		m = (srcPtr[1] & 0x02020202) | ((srcPtr[0] & 0x02020202) << 4);
 		*dstPtr++ = (m >> 25) | (m >> 16) | (m >> 7) | (m << 2);
		srcPtr += 2;
	    }

	    outw(0x3c4, 0x0402);	//pvgaHW->writeSeq(pvgaHW, 0x02, 1 << 2);
	    dstPtr = dst;
	    srcPtr = src;
	    i = width;
	    j = phase;
	    
	    while(j--)
	    {
		m = (srcPtr[1] & 0x04040404) | ((srcPtr[0] & 0x04040404) << 4);
 		*dstPtr++ = (m >> 26) | (m >> 17) | (m >> 8) | (m << 1);
		srcPtr += 2;
	    }
	    
	    while(i >= 4) {
		m = (srcPtr[1] & 0x04040404) | ((srcPtr[0] & 0x04040404) << 4);
 		s1 = (m >> 26) | (m >> 17) | (m >> 8) | (m << 1);
		m = (srcPtr[3] & 0x04040404) | ((srcPtr[2] & 0x04040404) << 4);
 		s2 = (m >> 26) | (m >> 17) | (m >> 8) | (m << 1);
		m = (srcPtr[5] & 0x04040404) | ((srcPtr[4] & 0x04040404) << 4);
 		s3 = (m >> 26) | (m >> 17) | (m >> 8) | (m << 1);
		m = (srcPtr[7] & 0x04040404) | ((srcPtr[6] & 0x04040404) << 4);
 		s4 = (m >> 26) | (m >> 17) | (m >> 8) | (m << 1);
		*((unsigned long*)dstPtr) = s1 | (s2 << 8) | (s3 << 16) | (s4 << 24);
		srcPtr += 8;
		dstPtr += 4;
		i -= 4;
	    }
	    
	    while(i--)
	    {
		m = (srcPtr[1] & 0x04040404) | ((srcPtr[0] & 0x04040404) << 4);
 		*dstPtr++ = (m >> 26) | (m >> 17) | (m >> 8) | (m << 1);
		srcPtr += 2;
	    }
	    
	    outw(0x3c4, 0x0802);	//pvgaHW->writeSeq(pvgaHW, 0x02, 1 << 3);
	    dstPtr = dst;
	    srcPtr = src;
	    i = width;
	    j = phase;
	    
	    while(j--)
	    {
		m = (srcPtr[1] & 0x08080808) | ((srcPtr[0] & 0x08080808) << 4);
 		*dstPtr++ = (m >> 27) | (m >> 18) | (m >> 9) | m;
		srcPtr += 2;
	    }
	    
	    while(i >= 4)
	    {
		m = (srcPtr[1] & 0x08080808) | ((srcPtr[0] & 0x08080808) << 4);
 		s1 = (m >> 27) | (m >> 18) | (m >> 9) | m;
		m = (srcPtr[3] & 0x08080808) | ((srcPtr[2] & 0x08080808) << 4);
 		s2 = (m >> 27) | (m >> 18) | (m >> 9) | m;
		m = (srcPtr[5] & 0x08080808) | ((srcPtr[4] & 0x08080808) << 4);
 		s3 = (m >> 27) | (m >> 18) | (m >> 9) | m;
		m = (srcPtr[7] & 0x08080808) | ((srcPtr[6] & 0x08080808) << 4);
 		s4 = (m >> 27) | (m >> 18) | (m >> 9) | m;
		*((unsigned long*)dstPtr) = s1 | (s2 << 8) | (s3 << 16) | (s4 << 24);
		srcPtr += 8;
		dstPtr += 4;
		i -= 4;
	    }
	    
	    while(i--)
	    {
		m = (srcPtr[1] & 0x08080808) | ((srcPtr[0] & 0x08080808) << 4);
 		*dstPtr++ = (m >> 27) | (m >> 18) | (m >> 9) | m;
		srcPtr += 2;
	    }
	    
            dst += FBPitch;
            src += SRCPitch;
	    
        } /* while(height--) */
        pbox++;
	
    } /* while(num--) */ 
} 

/****************************************************************************************/
