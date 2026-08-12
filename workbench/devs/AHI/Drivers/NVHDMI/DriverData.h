#ifndef AHI_Drivers_NVHDMI_DriverData_h
#define AHI_Drivers_NVHDMI_DriverData_h

#include <exec/types.h>
#include <exec/interrupts.h>
#include <devices/ahi.h>

#include <hidd/hda.h>

/** Make the common library code initialize a global SysBase for us. */

#define DRIVER "nvhdmi.audio"
#define DRIVER_NEEDS_GLOBAL_EXECBASE

#include "DriverBase.h"

struct NVHDMIChip;

struct NVHDMIBase {
    struct DriverBase      driverbase;

    /** A sempahore used for locking */
    struct SignalSemaphore semaphore;

    /** The number of cards found */
    int                    cards_found;

    /** A NVHDMIChip structure for each card found */
    struct NVHDMIChip    **driverdatas;
};

#define DRIVERBASE_SIZEOF (sizeof (struct NVHDMIBase))


// Verb - Set Converter Format (Verb ID=2h)
struct Freq {
    ULONG frequency;
    UBYTE base44100; // 1 if base= 44.1kHz, 0 if base=48kHz
    UBYTE mult; // multiplier 3 bits
    UBYTE div; // divisor 3 bits
};


struct NVHDMIChip {
    /** The hdaudio.hidd controller driving this card */
    APTR hda_ctrl;

    int flip;

    UWORD codecbits;
    UWORD codecnr;

    /** Stream in use while playing */
    APTR output_stream;
    struct HDA_StreamInfo output_info;

    // important node IDs
    UBYTE function_group;
    UBYTE dac_nid; // digital converter feeding the chosen pin
    UBYTE pin_nid; // HDMI/DP pin the monitor is on
    BOOL  pin_is_dp;

    // sample rate
    struct Freq *frequencies;
    ULONG nr_of_frequencies;
    ULONG selected_freq_index;

    // sample bitsize
    ULONG *bitsizes;
    ULONG nr_of_bitsizes;
    ULONG selected_bitsize_index;

    /** TRUE if the codec has been initialized */
    BOOL                card_initialized;

    /*** The driverbase ******************************************************/

    struct DriverBase  *ahisubbase;

    /*** The AudioCtrl currently using this DriverData structure *************/

    struct AHIAudioCtrlDrv *audioctrl;

    /*** Playback interrupt **************************************************/

    /** TRUE when playback is enabled */
    BOOL                is_playing;

    /** The playback software interrupt, caused by the hidd on
        buffer completion */
    struct Interrupt    playback_interrupt;

    /*** Playback interrupt variables ****************************************/

    APTR playback_buffer1;
    APTR playback_buffer2;

    /** The mixing buffer (a cyclic buffer filled by AHI) */
    APTR mix_buffer;

    /** The length of each playback buffer in sample frames */
    ULONG               current_frames;

    /** The length of each playback buffer in sample bytes */
    ULONG               current_bytesize;

    /** Where (inside the cyclic buffer) we're currently writing */
    APTR current_buffer;

    /** The current (playback) output volume (fixed, not adjustable) */
    Fixed               output_volume;
};

#endif /* AHI_Drivers_NVHDMI_DriverData_h */
