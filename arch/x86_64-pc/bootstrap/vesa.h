#ifndef VESA_H_
#define VESA_H_

#include "multiboot.h"

#ifndef _IMPLEMENTATION_

asm (".set getControllerInfo,0xf000");
asm (".set getModeInfo,0xf004");
asm (".set findMode,0xf008");
asm (".set setVbeMode,0xf00c");
asm (".set controllerinfo,0xf010");
asm (".set modeinfo,0xf014");

extern short (*getControllerInfo)(struct vbe_controller *info);
extern short (*getModeInfo)(long mode, struct vbe_mode *info);
extern short (*setVbeMode)(long mode);
extern short (*findMode)(int x, int y, int d);
extern struct vbe_controller *controllerinfo;
extern struct vbe_mode       *modeinfo;

#else

extern short getControllerInfo(struct vbe_controller *info);
extern short getModeInfo(long mode, struct vbe_mode *info);
extern short setVbeMode(long mode);
extern short findMode(int x, int y, int d);
extern struct vbe_controller controllerinfo;
extern struct vbe_mode       modeinfo;

#endif /*_IMPLEMENTATION_ */

#endif /*VESA_H_*/
