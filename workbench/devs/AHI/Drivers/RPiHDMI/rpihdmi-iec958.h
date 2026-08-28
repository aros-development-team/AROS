#ifndef AHI_Drivers_RPiHDMI_iec958_h
#define AHI_Drivers_RPiHDMI_iec958_h

#include <exec/types.h>

/* IEC958/SPDIF channel status setup (separate L/R per IEC 60958-3) */
void spdif_setup_channel_status(UBYTE *cs_left, UBYTE *cs_right, ULONG samplerate);

/* IEC958 sample conversion */
void convert_mix_to_iec958(WORD *src, ULONG *dst, ULONG frames, UBYTE *cs_left, UBYTE *cs_right, ULONG *frame_counter);

#endif
