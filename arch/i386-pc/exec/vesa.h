#ifndef VESA_H_
#define VESA_H_

#include <aros/multiboot.h>

#ifndef _IMPLEMENTATION_

asm (".set getControllerInfo,0x1000");
asm (".set getModeInfo,0x1004");
asm (".set findMode,0x1008");
asm (".set setVbeMode,0x100c");
asm (".set paletteWidth,0x1010");
asm (".set paletteSetup,0x1014");
asm (".set controllerinfo,0x1018");
asm (".set modeinfo,0x101C");

extern short (*getControllerInfo)(void);
extern short (*getModeInfo)(long mode);
extern short (*setVbeMode)(long mode);
extern short (*paletteWidth)(long req, unsigned char *width);
extern short (*paletteSetup)(unsigned char bits);
extern short (*findMode)(int x, int y, int d);
extern struct vbe_controller *controllerinfo;
extern struct vbe_mode       *modeinfo;

extern void *_binary_vesa_size, *_binary_vesa_start;

#else

struct palette_data
{
    unsigned char B;
    unsigned char G;
    unsigned char R;
    unsigned char Z;
};

extern short getControllerInfo(void);
extern short getModeInfo(long mode);
extern short setVbeMode(long mode);
extern short paletteWidth(long req, unsigned char *width);
extern short paletteSetup(unsigned char bits);
extern short findMode(int x, int y, int d);
extern struct vbe_controller controllerinfo;
extern struct vbe_mode       modeinfo;

#endif /*_IMPLEMENTATION_ */


#endif /*VESA_H_*/
