#ifndef _VIDEOCOREGFX_HARDWARE_H
#define _VIDEOCOREGFX_HARDWARE_H

#include <exec/libraries.h>
#include <hidd/pci.h>
#include <oop/oop.h>

extern IPTR __arm_periiobase;
#define ARM_PERIIOBASE __arm_periiobase
#include <hardware/bcm2708.h>
#include <hardware/videocore.h>

#include "vcgfx_hidd.h"
#include "vcgfx_bitmap.h"

/* BCM2712 moved the mailbox to bus 0x7c013880; the peripheral base is
 * the legacy block either way, so only the offset differs. */
#undef VCMB_BASE
#define VCMB_OFFSET_BCM2712 0x013880
#define VCMB_BASE (ARM_PERIIOBASE + ((ARM_PERIIOBASE == BCM2712_PERIIOBASE) \
                                     ? VCMB_OFFSET_BCM2712 : VCMB_OFFSET))

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
