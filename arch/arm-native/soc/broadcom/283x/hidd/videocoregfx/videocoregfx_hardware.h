#ifndef _VIDEOCOREGFX_HARDWARE_H
#define _VIDEOCOREGFX_HARDWARE_H

#include <exec/libraries.h>
#include <hidd/pci.h>
#include <oop/oop.h>

extern IPTR __arm_periiobase;
#define ARM_PERIIOBASE __arm_periiobase
#include <hardware/bcm283x.h>
#include <hardware/videocore.h>

#include "videocoregfx_class.h"
#include "videocoregfx_bitmap.h"

#define VCMB_PROPCHAN     8

struct HWData  {
	ULONG void0;
};

#define VCDATA(x) ((struct HWData *)x->data)

#define FNAME_HW(x) VideoCoreGfx__HW__ ## x

BOOL FNAME_HW(InitGfxHW)(APTR param0);
void FNAME_HW(RefreshArea)(struct HWData *hwdata,
                            struct BitmapData *data,
                            LONG x1, LONG y1, LONG x2, LONG y2);

#endif /* _VIDEOCOREGFX_HARDWARE_H */
