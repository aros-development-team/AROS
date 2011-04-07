#ifndef VESA_H_
#define VESA_H_

#include <hardware/vbe.h>

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
extern short (*setVbeMode)(long mode);
extern short (*paletteWidth)(long req, unsigned char *width);
extern short (*findMode)(int x, int y, int d);
extern struct vbe_controller *controllerinfo;
extern struct vbe_mode       *modeinfo;

#else

struct vesa11Info
{
    unsigned short x_resolution;
    unsigned short y_resolution;
    unsigned char bits_per_pixel;
    unsigned char memory_model;
};

extern short getControllerInfo(void);
extern short getModeInfo(long mode);
extern short setVbeMode(long mode);
extern short paletteWidth(long req, unsigned char *width);
extern short findMode(int x, int y, int d);
extern struct vbe_controller controllerinfo;
extern struct vbe_mode       modeinfo;

#endif /*_IMPLEMENTATION_ */


#endif /*VESA_H_*/
