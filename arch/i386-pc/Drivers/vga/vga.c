/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VGA driver for AROS
    Lang: english
*/

#define VERSION		"$VER: vga 1.0 (25.8.1999)"
#define NAME		"VGA card driver"

static const char version[];
static const char name[];
static char mode_640x480x4[];
static char mode_320x200x8[];
static const char DefPal[];

extern void SetVGAMode(char * Parameters);
extern void SetVGAPal(char * Pal, char first, int count);
extern unsigned char AROS_planes[];
extern void xputc(char chr,int num);

static const char version[] = VERSION;
static const char name[] = NAME;

struct ModeDesc {
    char *	Name;
    char *	Parameters;
    };

char mode_640x480x4[63]={
    0x00,0x01,0x02,0x03,0x04,
    0x05,0x06,0x07,0x08,0x09,
    0x0a,0x0b,0x0c,0x0d,0x0e,
    0x0f,
    0x01,0x00,0x0f,0x00,0x00,	// 0x03c0
    0xe3,			// 0x03c2
    0x03,0x01,0x0f,0x00,0x06,	// 0x03c4
    0xff,0x00,			// 0x03c6,0x03da
    0x00,0x00,0x00,0x00,0x00,
    0x00,0x05,0x0f,0xff,	// 0x03ce
    0x5f,0x4f,0x50,0x82,0x54,
    0x80,0x0b,0x3e,0x00,0x40,
    0x00,0x00,0x00,0x00,0x00,
    0x00,0xea,0x8c,0xdf,0x28,	// 0x03d4
    0x00,0xe7,0x04,0xe3,0xff};

char mode_320x200x8[47]={
    0x41,0x00,0x0f,0x00,0x00,	// 0x03c0
    0x63,			// 0x03c2
    0x03,0x01,0x0f,0x00,0x0e,	// 0x03c4
    0xff,0x00,			// 0x03c6,0x03da
    0x00,0x00,0x00,0x00,0x00,
    0x40,0x01,0x0f,0xff,	// 0x03ce
    0x5f,0x4f,0x50,0x82,0x54,
    0x80,0xbf,0x1f,0x00,0x41,
    0x00,0x00,0x00,0x00,0x00,
    0x00,0x9c,0x8e,0x8f,0x28,	// 0x03d4
    0x40,0x96,0xb9,0xa3,0xff};

char mode_80x24x4[47]={
    0x0c,0x00,0x0f,0x08,0x00,	// 0x03c0
    0x67,			// 0x03c2
    0x03,0x00,0x03,0x00,0x02,	// 0x03c4
    0xff,0x00,			// 0x03c6,0x03da
    0x00,0x00,0x00,0x00,0x00,
    0x10,0x0e,0x00,0xff,	// 0x03ce
    0x5f,0x4f,0x50,0x82,0x55,
    0x81,0xbf,0x1f,0x00,0x4f,
    0x0d,0x0e,0x00,0x00,0x00,
    0x00,0x9c,0x8e,0x8f,0x28,	// 0x03d4
    0x0f,0x96,0xb9,0xa3,0xff};

const char DefPal[]=
    {	 0, 0, 0,	 0, 0,42,
	 0,42, 0,	 0,42,42,
	42, 0, 0,	42, 0,42,
	42,21, 0,	42,42,42,
	21,21,21,	21,21,63,
	21,63,21,	21,63,63,
	63,21,21,	63,21,63,
	63,63,21,	63,63,63	};

struct ModeDesc mode_lo={"320x200x256",
			(char*)&mode_320x200x8},
		mode_hi={"640x480x16",
			(char*)&mode_640x480x4},
		mode_txt={"80x24x16",
			(char*)&mode_80x24x4};

struct ModeDesc * Modes[]={&mode_txt,&mode_lo,&mode_hi};

typedef struct {
  unsigned char MiscOutReg;     /* */
  unsigned char CRTC[25];       /* Crtc Controller */
  unsigned char Sequencer[5];   /* Video Sequencer */
  unsigned char Graphics[9];    /* Video Graphics */
  unsigned char Attribute[21];  /* Video Atribute */
  unsigned char DAC[768];       /* Internal Colorlookuptable */
  char NoClock;                 /* number of selected clock */
} vgaHWRec, *vgaHWPtr;

vgaHWRec	mode;
vgaHWPtr	pmode=&mode;

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

int _vgaBlankScreen(int on)
{
  unsigned char scrn;

  outb(0x3C4,1);
  scrn = inb(0x3C5);

  if(on) {
    scrn &= 0xDF;			/* enable screen */
  }else {
    scrn |= 0x20;			/* blank screen */
  }

  outw(0x3C4, (scrn << 8) | 0x01); /* change mode */
  return 1;
}

/*
 * vgaSaveScreen -- blank the screen.
 */

int _vgaSaveScreen(int on)
{
    _vgaBlankScreen(on);
    return 1;
}

/* Defines for 640x480 @ 60Hz screen */

#define CrtcHTotal 	800
#define CrtcHDisplay	640
#define CrtcHSyncStart	664
#define CrtcHSyncEnd	760
#define	CrtcHSkew	0

#define CrtcVTotal 	525
#define CrtcVDisplay	480
#define CrtcVSyncStart	491
#define CrtcVSyncEnd	493

void SetMode()
{
    unsigned int       i;

    /*
     * initialize default colormap for monochrome
     */
    for (i=0; i<3;   i++) mode.DAC[i] = 0x00;
    for (i=3; i<768; i++) mode.DAC[i] = 0x3F;

    mode.NoClock=0;	/* Clock 25.175 MHz */

    /* Initialise overscan register */
    mode.Attribute[17] = 0xFF;

    /* Set HSync and VSync */
    mode.MiscOutReg = 0xE3;		/* -hsync -vsync */

    mode.MiscOutReg |= (mode.NoClock & 0x03) << 2;

    /*
    * Time Sequencer
    */
    mode.Sequencer[0] = 0x02;
    mode.Sequencer[1] = 0x01;
    mode.Sequencer[2] = 0x0F;
    mode.Sequencer[3] = 0x00;                             /* Font select */
    mode.Sequencer[4] = 0x06;                             /* Misc */

    /*
    * CRTC Controller
    */
    mode.CRTC[0]  = (CrtcHTotal >> 3) - 5;
    mode.CRTC[1]  = (CrtcHDisplay >> 3) - 1;
    mode.CRTC[2]  = (CrtcHSyncStart >> 3) -1;
    mode.CRTC[3]  = ((CrtcHSyncEnd >> 3) & 0x1F) | 0x80;
    i = (((CrtcHSkew) + 0x10) & ~0x1F);
    if (i < 0x80)
	mode.CRTC[3] |= i;
    mode.CRTC[4]  = (CrtcHSyncStart >> 3);
    mode.CRTC[5]  = (((CrtcHSyncEnd >> 3) & 0x20 ) << 2 )
	| (((CrtcHSyncEnd >> 3)) & 0x1F);
    mode.CRTC[6]  = (CrtcVTotal - 2) & 0xFF;
    mode.CRTC[7]  = (((CrtcVTotal -2) & 0x100) >> 8 )
	| (((CrtcVDisplay -1) & 0x100) >> 7 )
          | ((CrtcVSyncStart & 0x100) >> 6 )
	    | (((CrtcVSyncStart) & 0x100) >> 5 )
	      | 0x10
		| (((CrtcVTotal -2) & 0x200)   >> 4 )
	          | (((CrtcVDisplay -1) & 0x200) >> 3 )
		    | ((CrtcVSyncStart & 0x200) >> 2 );
    mode.CRTC[8]  = 0x00;
    mode.CRTC[9]  = ((CrtcVSyncStart & 0x200) >>4) | 0x40;
    mode.CRTC[10] = 0x00;
    mode.CRTC[11] = 0x00;
    mode.CRTC[12] = 0x00;
    mode.CRTC[13] = 0x00;
    mode.CRTC[14] = 0x00;
    mode.CRTC[15] = 0x00;
    mode.CRTC[16] = CrtcVSyncStart & 0xFF;
    mode.CRTC[17] = (CrtcVSyncEnd & 0x0F) | 0x20;
    mode.CRTC[18] = (CrtcVDisplay -1) & 0xFF;
    mode.CRTC[19] = 40;  /* just a guess */
    mode.CRTC[20] = 0x00;
    mode.CRTC[21] = CrtcVSyncStart & 0xFF; 
    mode.CRTC[22] = (CrtcVSyncEnd + 1) & 0xFF;
    mode.CRTC[23] = 0xE3;
    mode.CRTC[24] = 0xFF;

    /*
    * Graphics Display Controller
    */
    mode.Graphics[0] = 0x00;
    mode.Graphics[1] = 0x00;
    mode.Graphics[2] = 0x00;
    mode.Graphics[3] = 0x00;
    mode.Graphics[4] = 0x00;
    mode.Graphics[5] = 0x00;
    mode.Graphics[6] = 0x05;   /* only map 64k VGA memory !!!! */
    mode.Graphics[7] = 0x0F;
    mode.Graphics[8] = 0xFF;

    mode.Attribute[0]  = 0x00; /* standard colormap translation */
    mode.Attribute[1]  = 0x01;
    mode.Attribute[2]  = 0x02;
    mode.Attribute[3]  = 0x03;
    mode.Attribute[4]  = 0x04;
    mode.Attribute[5]  = 0x05;
    mode.Attribute[6]  = 0x06;
    mode.Attribute[7]  = 0x07;
    mode.Attribute[8]  = 0x08;
    mode.Attribute[9]  = 0x09;
    mode.Attribute[10] = 0x0A;
    mode.Attribute[11] = 0x0B;
    mode.Attribute[12] = 0x0C;
    mode.Attribute[13] = 0x0D;
    mode.Attribute[14] = 0x0E;
    mode.Attribute[15] = 0x0F;
    mode.Attribute[16] = 0x81; /* wrong for the ET4000 */
    mode.Attribute[17] = 0x00; /* GJA -- overscan. */
    /*
    * Attribute[17] is the overscan, and is initalised above only at startup
    * time, and not when mode switching.
    */
    mode.Attribute[18] = 0x0F;
    mode.Attribute[19] = 0x00;
    mode.Attribute[20] = 0x00;

/* Set this mode on. */

    {
	int i,tmp;

#define vgaIOBase	0x3d0

	tmp = inb(vgaIOBase + 0x0A);		/* Reset flip-flop */
	outb(0x3C0, 0x00);			/* Enables pallete access */

	_vgaSaveScreen(0);
	outw(0x3CE,0x0003); /* GJA - don't rotate, write unmodified */
	outw(0x3CE,0xFF08); /* GJA - write all bits in a byte */
	outw(0x3CE,0x0001); /* GJA - all planes come from CPU */
	_vgaSaveScreen(1);
	tmp = inb(vgaIOBase + 0x0A);			/* Reset flip-flop */
	outb(0x3C0, 0x00);				/* Enables pallete access */
	if (vgaIOBase == 0x3B0)
	    mode.MiscOutReg &= 0xFE;
	else
	    mode.MiscOutReg |= 0x01;

	outb(0x3C2, mode.MiscOutReg);

	for (i=1; i<5;  i++) outw(0x3C4, (mode.Sequencer[i] << 8) | i);
  
	/* Ensure CRTC registers 0-7 are unlocked by clearing bit 7 or CRTC[17] */

	outw(vgaIOBase + 4, ((mode.CRTC[17] & 0x7F) << 8) | 17);

	for (i=0; i<25; i++) outw(vgaIOBase + 4,(mode.CRTC[i] << 8) | i);

	for (i=0; i<9;  i++) outw(0x3CE, (mode.Graphics[i] << 8) | i);

	for (i=0; i<21; i++) {
	    tmp = inb(vgaIOBase + 0x0A);
	    outb(0x3C0,i); outb(0x3C0, mode.Attribute[i]);
	}

	outb(0x3C6,0xFF);
	outb(0x3C8,0x00);
	for (i=0; i<768; i++)
	{
	    outb(0x3C9, mode.DAC[i]);
	    DACDelay;
	}

	tmp = inb(vgaIOBase + 0x0A);
	outb(0x3C0, 0x20);
    }
}

void SetDefPal()
{
    SetVGAPal((char*)&DefPal,0,16);
}

void _SetMode(int num)
{
    SetVGAMode(Modes[num]->Parameters);
}

void AROS_InfoText(int num, char * text)
{
    while(*text)
    {
	xputc(*text++,num);
    }
}
