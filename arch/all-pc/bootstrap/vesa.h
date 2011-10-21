/*
    Copyright © 2007-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VBE-related definitions.
    Lang: english
*/

#ifndef VESA_H_
#define VESA_H_

#include <aros/multiboot.h>
#include <exec/types.h>

/* Our trampoline code is linked at this address */
#define VESA_START (void *)0x1000

#ifndef _IMPLEMENTATION_

asm (".set getControllerInfo,0x1000");
asm (".set getModeInfo,0x1004");
asm (".set findMode,0x1008");
asm (".set setVbeMode,0x100c");
asm (".set paletteWidth,0x1010");
asm (".set controllerinfo,0x1014");
asm (".set modeinfo,0x1018");

extern short (*getControllerInfo)(void);
extern short (*getModeInfo)(long mode);
extern short (*setVbeMode)(long mode, BOOL set_refresh);
extern short (*paletteWidth)(long req, unsigned char *width);
extern short (*findMode)(int x, int y, int d, int vfreq, BOOL prioritise_depth);
extern struct vbe_controller *controllerinfo;
extern struct vbe_mode       *modeinfo;

extern void *_binary_vesa_size, *_binary_vesa_start;

#else

struct vesa11Info
{
    unsigned short x_resolution;
    unsigned short y_resolution;
    unsigned char bits_per_pixel;
    unsigned char memory_model;
};

struct CRTCInfoBlock
{
    UWORD h_total;
    UWORD h_sync_start;
    UWORD h_sync_end;
    UWORD v_total;
    UWORD v_sync_start;
    UWORD v_sync_end;
    UBYTE flags;
    ULONG pixel_clock;
    UWORD refresh_rate;
    UBYTE reserved[40];
} __attribute__((packed));

extern short getControllerInfo(void);
extern short getModeInfo(long mode);
extern short setVbeMode(long mode, BOOL set_refresh);
extern short paletteWidth(long req, unsigned char *width);
extern short findMode(int x, int y, int d, int vfreq, BOOL prioritise_depth);
extern struct vbe_controller controllerinfo;
extern struct vbe_mode       modeinfo;
extern struct CRTCInfoBlock  timings;

#endif /*_IMPLEMENTATION_ */


#endif /*VESA_H_*/
