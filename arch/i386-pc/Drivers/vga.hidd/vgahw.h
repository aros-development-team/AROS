#ifndef HIDD_VGAHW_H
#define HIDD_VGAHW_H

/*
    (C) 1999 AROS - The Amiga Research OS
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
    ({	long __value;	\
	__asm__ __volatile__ ("inl %%dx,%%eax":"=a"(__value):"d"(port));	\
	__value;	})

#define outl(port,val) \
    ({	long __value=(val);	\
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

#endif /* HIDD_VGAHW_H */