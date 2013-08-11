#ifndef AHI_Drivers_Card_DriverData_h
#define AHI_Drivers_Card_DriverData_h

#include <exec/types.h>
#include <exec/interrupts.h>
#include <devices/ahi.h>


/** Make the common library code initialize a global SysBase for us.
    It's required for hwaccess.c */

#define DRIVER "hdaudio.audio"
#define DRIVER_NEEDS_GLOBAL_EXECBASE
#define INPUTS 5

#include "DriverBase.h"

struct HDAudioChip;

struct HDAudioBase
{
    struct DriverBase      driverbase;

    
    /** A sempahore used for locking */
    struct SignalSemaphore semaphore;

    /** The number of cards found */
    int                    cards_found;

    /** A HDAudioChip structure for each card found */
    struct HDAudioChip**   driverdatas;
};

#define DRIVERBASE_SIZEOF (sizeof (struct HDAudioBase))

#define RECORD_BUFFER_SAMPLES     1024
#define RECORD_BUFFER_SIZE_VALUE  ADCBS_BUFSIZE_4096



struct BDLE // Buffer Descriptor List (3.6.2)
{
    ULONG lower_address; // address for 32-bit systems
    ULONG upper_address; // only for use with 64-bit systems
    ULONG length; // in bytes
    ULONG reserved_ioc; // bit 0 is Interrupt on Completion
};

struct Stream
{
    struct BDLE *bdl;
    APTR bdl_nonaligned;
    APTR *bdl_nonaligned_addresses;
    
    ULONG sd_reg_offset; // 3.3.35 offset 0x80 + (ISS) * 0x20
    ULONG index;
    ULONG tag; // index + 1
    UWORD fifo_size;
};


// Verb - Set Converter Format (Verb ID=2h)
struct Freq
{
    ULONG frequency;
    UBYTE base44100; // 1 if base= 44.1kHz, 0 if base=48kHz
    UBYTE mult; // multiplier 3 bits
    UBYTE div; // divisor 3 bits
};



struct HDAudioChip
{
    struct PCIDevice *pci_dev;
    APTR iobase;
    unsigned long length;
    unsigned short model;
    unsigned int irq; 
    int flip;
    int recflip;
    unsigned char chiprev;
    
    UWORD codecbits;
    UWORD codecnr;
    
    ULONG *corb;
    ULONG corb_entries;
    ULONG *rirb;
    ULONG rirb_entries;
    ULONG rirb_rp; // software read pointer
    LONG rirb_irq;
    
    APTR dma_position_buffer;
    
    struct Stream *streams;
    UBYTE nr_of_streams;
    UBYTE nr_of_input_streams;
    UBYTE nr_of_output_streams;
    
    // important node ID's
    UBYTE function_group;
    UBYTE dac_nid; // front L&R
    UBYTE adc_nid;
    UBYTE adc_mixer_nid;
    BOOL adc_mixer_is_mux;
    UBYTE dac_volume_nid;
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
    
    UBYTE eapd_gpio_mask;

    /*** PCI/Card initialization progress *********************************/

    /** TRUE if bus mastering is activated */
    BOOL                pci_master_enabled;

    /** TRUE if the Card chip has been initialized */
    BOOL                card_initialized;

    /*** The driverbase ******************************************************/

    /** This field is also used as a lock and access to is is
     * semaphore protected. */
    struct DriverBase*  ahisubbase;

    /*** The AudioCtrl currently using this DriverData structure *************/

    struct AHIAudioCtrlDrv* audioctrl;

    /*** Playback/recording interrupts ***************************************/
    
    /** TRUE when playback is enabled */
    BOOL                is_playing;

    /** TRUE when recording is enabled */
    BOOL                is_recording;

    /** The main (hardware) interrupt */
    struct Interrupt    interrupt;

    /** TRUE if the hardware interrupt has been added to the PCI subsystem */
    BOOL                interrupt_added;

    /** The playback software interrupt */
    struct Interrupt    playback_interrupt;

    /** The recording software interrupt */
    struct Interrupt    record_interrupt;
    
    /*** Card structures **************************************************/
    
    APTR                playback_buffer1;
    APTR                playback_buffer2;
    
    /*** Playback interrupt variables ****************************************/

    /** The mixing buffer (a cyclic buffer filled by AHI) */
    APTR                mix_buffer;

    /** The length of each playback buffer in sample frames */
    ULONG               current_frames;
    
    /** The length of each playback buffer in sample bytes */
    ULONG               current_bytesize;

    /** Where (inside the cyclic buffer) we're currently writing */
    APTR                current_buffer;



    /*** Recording interrupt variables ***************************************/

    /** The recording buffer (simple double buffering is used */
    APTR                record_buffer1;
    APTR                record_buffer2;
    APTR                record_buffer1_nonaligned;
    APTR                record_buffer2_nonaligned;

    /** Were (inside the recording buffer) the current data is */
    APTR                current_record_buffer;
    
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
