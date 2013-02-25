/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AHI_Drivers_VIAAC97_DriverData_h
#define AHI_Drivers_VIAAC97_DriverData_h

#include <exec/types.h>
#include <exec/interrupts.h>
#include <devices/ahi.h>

#include "hwaccess.h"

/** Make the common library code initialize a global SysBase for us.
    It's required for hwaccess.c */

#define DRIVER_NEEDS_GLOBAL_EXECBASE
#include "DriverBase.h"

struct CardData;

struct CardBase
{
    /** Skeleton's variables *************************************************/

    struct DriverBase      driverbase;


    /** The driver's global data *********************************************/

    
    /** A sempahore used for locking */
    struct SignalSemaphore semaphore;

    /** The number of cards found */
    int                    cards_found;

    /** A CardData structure for each card found */
    struct CardData**   driverdatas;
};

#define DRIVERBASE_SIZEOF (sizeof (struct CardBase))

#define RECORD_BUFFER_SAMPLES     4096
#define RECORD_BUFFER_SIZE_VALUE  ADCBS_BUFSIZE_16384


struct snd_dma_device {
	int type;			/* SNDRV_DMA_TYPE_XXX */
//	struct device *dev;		/* generic device */
};

struct snd_dma_buffer {
	struct snd_dma_device dev;	/* device type */
	unsigned char *area;	/* virtual pointer */
	void *addr;	/* physical address */
	int bytes;		/* buffer size in bytes */
	void *private_data;	/* private for allocator; don't touch */
};

struct snd_via_sg_table { // scatter/gather format. Oh joy, the docs talk about EOL, base counts etc...
        APTR offset;
        unsigned int size;
};

struct CardData
{

    struct PCIDevice			*pci_dev;
	unsigned long           iobase;
	unsigned long		length;
	unsigned short		model;
	unsigned int irq; 
    int flip;
    int recflip;
    UBYTE chiprev;
    
    struct snd_dma_buffer table;
    struct snd_via_sg_table *play_idx_table;
    struct snd_via_sg_table *rec_idx_table;
    
    APTR play_idx_table_nonaligned;
    APTR rec_idx_table_nonaligned;
    
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

    /** TRUE if the hardware interrupt may Cause() playback_interrupt */
    BOOL                playback_interrupt_enabled;

    /** The recording software interrupt */
    struct Interrupt    record_interrupt;

    /** TRUE if the hardware interrupt may Cause() playback_interrupt */
    BOOL                record_interrupt_enabled;

    /** The reset handler */
    struct Interrupt    reset_handler;

    /** TRUE if the reset handler has been added to the system */
    BOOL                reset_handler_added;

    /*** CAMD support functions **********************************************/

    /** CAMD transmitter function wrapped as a Hook */
    struct Hook*        camd_transmitfunc;

    /** CAMD receiver function wrapped as a Hook */
    struct Hook*        camd_receivefunc;

    /** True if CMAD V40 mode */
    ULONG               camd_v40;
    
    /*** Card structures **************************************************/
    
    APTR                playback_buffer1;
    APTR                playback_buffer2;
    APTR                playback_buffer1_nonaligned;
    APTR                playback_buffer2_nonaligned;

    
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

#endif /* AHI_Drivers_VIAAC97_DriverData_h */
