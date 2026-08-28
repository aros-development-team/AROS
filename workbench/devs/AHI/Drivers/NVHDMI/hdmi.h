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

/* Pick the HDMI/DP pin that has a sink on it and route a digital converter
   to it. Run at init AND at every playback start: the pins do not report
   presence or a valid ELD until the display link is up, and the HDA link
   reset this driver performs when it initialises clears what the display
   driver had programmed. Selecting once at init picks the wrong pin. */
BOOL setup_hdmi_codec(struct NVHDMIChip *card);

#endif /* AHI_Drivers_NVHDMI_hdmi_h */
