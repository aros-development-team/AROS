#ifndef HARDWARE_CUSTOM_H
#define HARDWARE_CUSTOM_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Custom Amiga chips
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

struct Custom
{
    UWORD bltddat;
    UWORD dmaconr;
    UWORD vposr;
    UWORD vhposr;
    UWORD dskdatr;
    UWORD joy0dat;
    UWORD joy1dat;
    UWORD clxdat;
    UWORD adkconr;
    UWORD pot0dat;
    UWORD pot1dat;
    UWORD potinp;
    UWORD serdatr;
    UWORD dskbytr;
    UWORD intenar;
    UWORD intreqr;
    APTR  dskpt;
    UWORD dsklen;
    UWORD dskdat;
    UWORD refptr;
    UWORD vposw;
    UWORD vhposw;
    UWORD copcon;
    UWORD serdat;
    UWORD serper;
    UWORD potgo;
    UWORD joytest;
    UWORD strequ;
    UWORD strvbl;
    UWORD strhor;
    UWORD strlong;

    UWORD bltcon0;
    UWORD bltcon1;
    UWORD bltafwm;
    UWORD bltalwm;
    APTR  bltcpt;
    APTR  bltbpt;
    APTR  bltapt;
    APTR  bltdpt;
    UWORD bltsize;
    UBYTE pad2d;
    UBYTE bltcon0l; /* WRITE-ONLY */
    UWORD bltsizv;
    UWORD bltsizh;
    UWORD bltcmod;
    UWORD bltbmod;
    UWORD bltamod;
    UWORD bltdmod;
    UWORD pad34[4];
    UWORD bltcdat;
    UWORD bltbdat;
    UWORD bltadat;

    UWORD pad3b[3];
    UWORD deniseid;
    UWORD dsksync;
    ULONG cop1lc;
    ULONG cop2lc;
    UWORD copjmp1;
    UWORD copjmp2;
    UWORD copins;
    UWORD diwstrt;
    UWORD diwstop;
    UWORD ddfstrt;
    UWORD ddfstop;
    UWORD dmacon;
    UWORD clxcon;
    UWORD intena;
    UWORD intreq;
    UWORD adkcon;

    /* chip audio channel */
    struct AudChannel
    {
        UWORD * ac_ptr;    /* waveform data */
        UWORD   ac_len;    /* waveform length (in words) */
        UWORD   ac_per;    /* sample period */
        UWORD   ac_vol;    /* volume */
        UWORD   ac_dat;
        UWORD   ac_pad[2];
    } aud[4];

    APTR  bplpt[8];
    UWORD bplcon0;
    UWORD bplcon1;
    UWORD bplcon2;
    UWORD bplcon3;
    UWORD bpl1mod;
    UWORD bpl2mod;
    UWORD bplcon4;
    UWORD clxcon2;
    UWORD bpldat[8];
    APTR  sprpt[8];

    struct SpriteDef
    {
        UWORD pos;
        UWORD ctl;
        UWORD dataa;
        UWORD datab;
    } spr[8];

    UWORD color[32];
    UWORD htotal;
    UWORD hsstop;
    UWORD hbstrt;
    UWORD hbstop;
    UWORD vtotal;
    UWORD vsstop;
    UWORD vbstrt;
    UWORD vbstop;
    UWORD sprhstrt;
    UWORD sprhstop;
    UWORD bplhstrt;
    UWORD bplhstop;
    UWORD hhposw;
    UWORD hhposr;
    UWORD beamcon0;
    UWORD hsstrt;
    UWORD vsstrt;
    UWORD hcenter;
    UWORD diwhigh;
    UWORD padf3[11];
    UWORD fmode;
};

#ifdef ECS_SPECIFIC

#define HSYNCTRUE   0x0001
#define VSYNCTRUE   0x0002
#define CSYNCTRUE   0x0004
#define CSBLANK     0x0008
#define VARCSYNC    0x0010
#define DISPLAYPAL  0x0020
#define DISPLAYDUAL 0x0040
#define VARBEAM     0x0080
#define VARHSYNC    0x0100
#define VARVSYNC    0x0200
#define CSCBLANKEN  0x0400
#define LOLDIS      0x0800
#define VARVBLANK   0x1000

/* bplcon0 */
#define USE_BPLCON3 1

/* bplcon2 */
#define BPLCON2_ZDCTEN   (1<<10)
#define BPLCON2_ZDBPEN   (1<<11)
#define BPLCON2_ZDBPSEL0 (1<<12)
#define BPLCON2_ZDBPSEL1 (1<<13)
#define BPLCON2_ZDBPSEL2 (1<<14)

/* bplcon3 */
#define BPLCON3_EXTBLNKEN (1<<0)
#define BPLCON3_EXTBLKZD  (1<<1)
#define BPLCON3_ZDCLKEN   (1<<2)
#define BPLCON3_BRDNTRAN  (1<<4)
#define BPLCON3_BRDNBLNK  (1<<5)

#endif /* ECS_SPECIFIC */

#endif /* HARDWARE_CUSTOM_H */
