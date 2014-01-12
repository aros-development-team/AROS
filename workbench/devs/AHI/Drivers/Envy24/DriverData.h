/*
    Copyright � 2004-2014, Davy Wentzler. All rights reserved.
    $Id$
*/

#ifndef AHI_Drivers_Card_DriverData_h
#define AHI_Drivers_Card_DriverData_h

#include <exec/types.h>
#include <exec/interrupts.h>
#include <devices/ahi.h>

#define DRIVER "envy24.audio"
#define DRIVER_NEEDS_GLOBAL_EXECBASE

enum Model {PHASE88, MAUDIO_2496, MAUDIO_1010LT, MAUDIO_DELTA44, MAUDIO_DELTA66};

#include "DriverBase.h"
#include "I2C.h"
#include "ak_codec.h"



   
struct CardData;

struct CardBase
{
    /** Skeleton's variables *************************************************/

    struct DriverBase      driverbase;


    /** A sempahore used for locking */
    struct SignalSemaphore semaphore;

    /** The number of cards found */
    int                    cards_found;

    /** A CardData structure for each card found */
    struct CardData**   driverdatas;
};

#define DRIVERBASE_SIZEOF (sizeof (struct CardBase))
#define RECORD_BUFFER_SAMPLES     1764


struct CardData
{
    /*** PCI/Card initialization progress *********************************/

   struct PCIDevice    *pci_dev;
	unsigned long       iobase;
   unsigned long       mtbase;
	unsigned short		model;
   unsigned char       chiprev;
   unsigned char       gpio_dir;
   unsigned char       gpio_data;
   struct I2C_bit_ops  *bit_ops;
   struct I2C          *i2c_cs8404;
   struct I2C          *i2c_in_addr;
   struct I2C          *i2c_out_addr;
   enum akm_types      akm_type;
   struct akm_codec    codec[4];
   
   enum Model          SubType;
   

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

    /** TRUE if the hardware interrupt may Cause() playback_interrupt */
    BOOL                playback_interrupt_enabled;

    /** The recording software interrupt */
    struct Interrupt    record_interrupt;

    /** TRUE if the hardware interrupt may Cause() playback_interrupt */
    BOOL                record_interrupt_enabled;



    /*** CAMD support functions **********************************************/

    /** CAMD transmitter function wrapped as a Hook */
    struct Hook*        camd_transmitfunc;

    /** CAMD receiver function wrapped as a Hook */
    struct Hook*        camd_receivefunc;

    /** True if CMAD V40 mode */
    ULONG               camd_v40;

    

    /*** Playback interrupt variables ****************************************/

    APTR                playback_buffer;
    APTR                spdif_out_buffer;
    APTR                playback_buffer_nonaligned;
    APTR                spdif_out_buffer_nonaligned;
    APTR                playback_buffer_phys;
    APTR                spdif_out_buffer_phys;


    /** The mixing buffer (a cyclic buffer filled by AHI) */
    APTR                mix_buffer;

    /** The length of each playback buffer in sample frames */
    ULONG               current_frames;
    
    /** The length of each playback buffer in sample bytes */
    ULONG               current_bytesize;

    /** Where (inside the cyclic buffer) we're currently writing */
    APTR                current_buffer;
    APTR                spdif_out_current_buffer;

    int                 flip;



    /*** Recording interrupt variables ***************************************/

    /** The recording buffer (simple double buffering is used */
    APTR                record_buffer;
    APTR                record_buffer_32bit;
    APTR                record_buffer_nonaligned;
    APTR                record_buffer_nonaligned_32bit;
    APTR                record_buffer_phys;
    APTR                record_buffer_32bit_phys;

    /** Were (inside the recording buffer) the current data is */
    APTR                current_record_buffer;
    APTR                current_record_buffer_32bit;
    
    /** The length of each record buffer in sample bytes */
    ULONG               current_record_bytesize_32bit;
    ULONG               current_record_bytesize_target;
    
    int                 recflip;



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

    /** The hardware register value corresponding to monitor_volume */
    UWORD               monitor_volume_bits;

    /** The hardware register value corresponding to input_gain */
    UWORD               input_gain_bits;

    /** The hardware register value corresponding to output_volume */
    UWORD               output_volume_bits;

    /** Saved state for AC97 mike */
    UWORD               ac97_mic;

    /** Saved state for AC97 cd */
    UWORD               ac97_cd;
    
    /** Saved state for AC97 vide */
    UWORD               ac97_video;
    
    /** Saved state for AC97 aux */
    UWORD               ac97_aux;
    
    /** Saved state for AC97 line in */
    UWORD               ac97_linein;
    
    /** Saved state for AC97 phone */
    UWORD               ac97_phone;
};

#endif /* AHI_Drivers_Card_DriverData_h */
