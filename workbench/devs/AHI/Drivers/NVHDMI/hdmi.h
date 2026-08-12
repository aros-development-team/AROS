#ifndef AHI_Drivers_NVHDMI_hdmi_h
#define AHI_Drivers_NVHDMI_hdmi_h

#include <config.h>

#include <devices/ahi.h>
#include <DriverData.h>
#include <stddef.h>


#define udelay micro_delay


struct NVHDMIChip *AllocDriverData(APTR controller, struct DriverBase *AHIsubBase);
void FreeDriverData(struct NVHDMIChip *dd, struct DriverBase *AHIsubBase);

void micro_delay(unsigned int val);

ULONG send_command_12(UBYTE codec, UBYTE node, UWORD verb, UBYTE payload, struct NVHDMIChip *card);
ULONG send_command_4(UBYTE codec, UBYTE node, UBYTE verb, UWORD payload, struct NVHDMIChip *card);
ULONG get_parameter(UBYTE node, UBYTE parameter, struct NVHDMIChip *card);

UWORD get_hda_format(struct NVHDMIChip *card);

#endif /* AHI_Drivers_NVHDMI_hdmi_h */
