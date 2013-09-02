#ifndef HIDD_VGAHW_H
#define HIDD_VGAHW_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VGA hardwired.
    Lang: English.
*/

/* I have to move away this defines - should be in separate file */

#define inb(port) \
    ({	char __value;	\
	__asm__ __volatile__ ("inb %%dx,%%al":"=a"(__value):"d"(port));	\
	__value;	})

#define outb(port,val) \
    ({	char __value=(val);	\
	__asm__ __volatile__ ("outb %%al,%%dx"::"a"(__value),"d"(port)); })

#define inw(port) \
    ({	short __value;	\
	__asm__ __volatile__ ("inw %%dx,%%ax":"=a"(__value):"d"(port));	\
	__value;	})

#define outw(port,val) \
    ({	short __value=(val);	\
	__asm__ __volatile__ ("outw %%ax,%%dx"::"a"(__value),"d"(port)); })

#define inl(port) \
    ({	LONG __value;	\
	__asm__ __volatile__ ("inl %%dx,%%eax":"=a"(__value):"d"(port));	\
	__value;	})

#define outl(port,val) \
    ({	LONG __value=(val);	\
	__asm__ __volatile__ ("outl %%eax,%%dx"::"a"(__value),"d"(port)); })

#define DACDelay \
	{ \
		unsigned char temp = inb(vgaIOBase + 0x0A); \
		temp = inb(vgaIOBase + 0x0A); \
	}

/* This structure keeps contents of mode registers including palette */

struct vgaHWRec
{
  unsigned char MiscOutReg;     /* */
  unsigned char CRTC[25];       /* Crtc Controller */
  unsigned char Sequencer[5];   /* Video Sequencer */
  unsigned char Graphics[9];    /* Video Graphics */
  unsigned char Attribute[21];  /* Video Atribute */
  unsigned char DAC[768];       /* Internal Colorlookuptable */
  char NoClock;                 /* number of selected clock */
};

/* CRT controller register indices */

#define CRTC_H_TOTAL	    0
#define CRTC_H_DISPLAY	    1
#define CRTC_H_BLANK_START  2
#define CRTC_H_BLANK_END    3
#define CRTC_H_SYNC_START   4
#define CRTC_H_SYNC_END	    5
#define CRTC_V_TOTAL	    6
#define CRTC_OVERFLOW	    7
#define CRTC_PRESET_ROW	    8
#define CRTC_MAX_SCAN	    9
#define CRTC_CURSOR_START   10
#define CRTC_CURSOR_END	    11
#define CRTC_START_HI	    12
#define CRTC_START_LO	    13
#define CRTC_CURSOR_HI	    14
#define CRTC_CURSOR_LO	    15
#define CRTC_V_SYNC_START   16
#define CRTC_V_SYNC_END	    17
#define CRTC_V_DISP_END	    18
#define CRTC_OFFSET	    19
#define CRTC_UNDERLINE	    20
#define CRTC_V_BLANK_START  21
#define CRTC_V_BLANK_END    22
#define CRTC_MODE	    23
#define CRTC_LINE_COMPARE   24

#endif /* HIDD_VGAHW_H */
