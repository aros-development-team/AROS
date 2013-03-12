/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#ifndef AHI_Drivers_Card_DriverData_h
#define AHI_Drivers_Card_DriverData_h

#include <exec/types.h>
#include <exec/interrupts.h>
#include <devices/ahi.h>
#include "ak_codec.h"
#include "I2C.h"

#define DRIVER "envy24ht.audio"
#define DRIVER_NEEDS_GLOBAL_EXECBASE
#include "DriverBase.h"

#define DATA_PORT               CCS_UART_DATA
#define COMMAND_PORT            CCS_UART_COMMAND
#define STATUS_PORT             CCS_UART_COMMAND

#define STATUSF_OUTPUT          0x40
#define STATUSF_INPUT           0x80

#define COMMAND_RESET           0xff
#define DATA_ACKNOWLEDGE        0xfe
#define COMMAND_UART_MODE       0x3f

#define MPU401_OUTPUT_READY()  ((INBYTE(card->iobase + STATUS_PORT) & STATUSF_OUTPUT) == 0)
#define MPU401_INPUT_READY()   ((INBYTE(card->iobase + STATUS_PORT) & STATUSF_INPUT) == 0)

#define MPU401_CMD(c)         OUTBYTE(card->iobase + COMMAND_PORT, c)
#define MPU401_STATUS()       INBYTE(card->iobase + STATUS_PORT)
#define MPU401_READ()         INBYTE(card->iobase + DATA_PORT )
#define MPU401_WRITE(v)       OUTBYTE(card->iobase + DATA_PORT,v )

enum Model {AUREON_SKY, AUREON_SPACE, PHASE28, REVO51, REVO71, JULIA, PHASE22};
extern unsigned long Dirs[];


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

   struct PCIDevice  *pci_dev;
	unsigned long     iobase;
   unsigned long     mtbase;
	unsigned short		model;
   unsigned char     chiprev;
	unsigned int      irq;
   BOOL              input_is_24bits;
   unsigned long    SavedDir;
   unsigned short   SavedMask;

    /** TRUE if bus mastering is activated */
    BOOL                pci_master_enabled;

    /** TRUE if the Card chip has been initialized */
    BOOL                card_initialized;
    enum Model      SubType;

    struct I2C_bit_ops  *bit_ops;
    unsigned int       gpio_dir;
    unsigned int       gpio_data;
    struct I2C        *i2c;



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
    
    // For revo71
    struct akm_codec    *RevoFrontCodec;
    struct akm_codec    *RevoSurroundCodec;
    struct akm_codec    *RevoRecCodec;
    
    struct akm_codec    *JuliaDAC;
    struct akm_codec    *JuliaRCV; // digital receiver
};

#endif /* AHI_Drivers_Card_DriverData_h */
