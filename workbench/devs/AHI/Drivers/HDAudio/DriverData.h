#ifndef AHI_Drivers_Card_DriverData_h
#define AHI_Drivers_Card_DriverData_h

#include <exec/types.h>
#include <exec/interrupts.h>
#include <devices/ahi.h>

#include <hidd/hda.h>

/** Make the common library code initialize a global SysBase for us.
    It's required for hwaccess.c */

#define DRIVER "hdaudio.audio"
#define DRIVER_NEEDS_GLOBAL_EXECBASE
#define INPUTS 5

#include "DriverBase.h"

struct HDAudioChip;

struct HDAudioBase {
    struct DriverBase      driverbase;


    /** A sempahore used for locking */
    struct SignalSemaphore semaphore;

    /** The number of cards found */
    int                    cards_found;

    /** A HDAudioChip structure for each card found */
    struct HDAudioChip   **driverdatas;
};

#define DRIVERBASE_SIZEOF (sizeof (struct HDAudioBase))

#define RECORD_BUFFER_SAMPLES     1024


// Verb - Set Converter Format (Verb ID=2h)
struct Freq {
    ULONG frequency;
    UBYTE base44100; // 1 if base= 44.1kHz, 0 if base=48kHz
    UBYTE mult; // multiplier 3 bits
    UBYTE div; // divisor 3 bits
};



struct HDAudioChip {
    /** The hdaudio.hidd controller driving this card */
    APTR hda_ctrl;

    int flip;
    int recflip;

    UWORD codecbits;
    UWORD codecnr;

    /** Streams in use while playing/recording */
    APTR output_stream;
    APTR input_stream;
    struct HDA_StreamInfo output_info;
    struct HDA_StreamInfo input_info;

    // important node IDs
    UBYTE function_group;
    UBYTE dac_nid; // front L&R
    UBYTE adc_nid;
    UBYTE adc_mixer_nid;
    BOOL adc_mixer_is_mux;
    UBYTE dac_volume_nids[10];
    UBYTE dac_volume_count;
    UBYTE speaker_nid;
    UBYTE headphone_nid;

    BOOL speaker_active;

    UBYTE line_in_nid;
    UBYTE mic1_nid;
    UBYTE mic2_nid;
    UBYTE cd_nid;

    UBYTE adc_mixer_indices[5]; //0 = Line in, 1 = Mic in 1, 2 = Mic in 2, 3 = CD, 4 = Monitor Mixer

    float adc_min_gain;
    float adc_max_gain;
    float adc_step_gain;
    float dac_min_gain;
    float dac_max_gain;
    float dac_step_gain;

    // sample rate
    struct Freq *frequencies;
    ULONG nr_of_frequencies;
    ULONG selected_freq_index;

    // sample bitsize
    ULONG *bitsizes;
    ULONG nr_of_bitsizes;
    ULONG selected_bitsize_index;

    UBYTE eapd_gpio_mask;

    /*** Card initialization progress *****************************************/

    /** TRUE if the Card chip has been initialized */
    BOOL                card_initialized;

    /*** The driverbase ******************************************************/

    /** This field is also used as a lock and access to is is
     * semaphore protected. */
    struct DriverBase  *ahisubbase;

    /*** The AudioCtrl currently using this DriverData structure *************/

    struct AHIAudioCtrlDrv *audioctrl;

    /*** Playback/recording interrupts ***************************************/

    /** TRUE when playback is enabled */
    BOOL                is_playing;

    /** TRUE when recording is enabled */
    BOOL                is_recording;

    /** The playback software interrupt, caused by the hidd on
        buffer completion */
    struct Interrupt    playback_interrupt;

    /** The recording software interrupt */
    struct Interrupt    record_interrupt;

    /*** Card structures **************************************************/
    APTR playback_buffer1;
    APTR playback_buffer2;

    /*** Playback interrupt variables ****************************************/

    /** The mixing buffer (a cyclic buffer filled by AHI) */
    APTR mix_buffer;

    /** The length of each playback buffer in sample frames */
    ULONG               current_frames;

    /** The length of each playback buffer in sample bytes */
    ULONG               current_bytesize;

    /** Where (inside the cyclic buffer) we're currently writing */
    APTR current_buffer;

    /*** Recording interrupt variables ***************************************/

    /** The recording buffer (simple double buffering is used */
    APTR record_buffer1;
    APTR record_buffer2;

    /** Were (inside the recording buffer) the current data is */
    APTR current_record_buffer;

    /** The length of each record buffer in sample bytes */
    ULONG               current_record_bytesize;

    /** Analog mixer variables ***********************************************/

    /** The currently selected input */
    UWORD               input;

    /** The currently selected output */
    UWORD               output;

    /** The current (recording) monitor volume */
    Fixed               monitor_volume;

    /** The current (recording) input gain */
    Fixed               input_gain;

    /** The current (playback) output volume */
    Fixed               output_volume;
};

#endif /* AHI_Drivers_Card_DriverData_h */
