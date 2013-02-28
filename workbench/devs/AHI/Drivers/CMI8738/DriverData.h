/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License");
you may not use this file except in compliance with the License. You may obtain a copy of the License at 
http://www.aros.org/license.html 
Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF 
ANY KIND, either express or implied. See the License for the specific language governing rights and 
limitations under the License. 

The Original Code is written by Davy Wentzler.
*/

#ifndef AHI_Drivers_CMI8738_DriverData_h
#define AHI_Drivers_CMI8738_DriverData_h

#include <exec/types.h>
#include <exec/interrupts.h>
#include <devices/ahi.h>

#define DRIVER "cmi8738.audio"
#define DRIVER_NEEDS_GLOBAL_EXECBASE

#include "DriverBase.h"

struct CMI8738_DATA;

struct tester
{
    unsigned long diff;
    int flip;
    int oldflip;
    int A;
    int Missed;
};

struct CMI8738Base
{
    /** Skeleton's variables *************************************************/

    struct DriverBase      driverbase;


    /** A sempahore used for locking */
    struct SignalSemaphore semaphore;

    /** The number of cards found */
    int                    cards_found;

    /** A CMI8738_DATA structure for each card found */
    struct CMI8738_DATA**   driverdatas;
};

#define DRIVERBASE_SIZEOF (sizeof (struct CMI8738Base))

#define RECORD_BUFFER_SAMPLES     1024


struct CMI8738_DATA
{
    /*** PCI/Card initialization progress *********************************/

    struct PCIDevice    *pci_dev;
    APTR		iobase;
    unsigned long       length;
    unsigned short      model;
    unsigned char       chiprev;
    unsigned char       chipvers;
    unsigned int	channels;
    unsigned int        irq;

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
    APTR                playback_buffer_nonaligned;
    APTR                playback_buffer_phys;

    /** The mixing buffer (a cyclic buffer filled by AHI) */
    APTR                mix_buffer;

    /** The length of each playback buffer in sample frames */
    ULONG               current_frames;
    
    /** The length of each playback buffer in sample bytes */
    ULONG               current_bytesize;

    /** Where (inside the cyclic buffer) we're currently writing */
    APTR                current_buffer;

    int                 flip;
    int                 oldflip;



    /*** Recording interrupt variables ***************************************/

    /** The recording buffer (simple double buffering is used */
    APTR                record_buffer;
    APTR                record_buffer_nonaligned;
    APTR                record_buffer_phys;

    /** Were (inside the recording buffer) the current data is */
    APTR                current_record_buffer;
    
    /** The length of each record buffer in sample bytes */
    ULONG               current_record_bytesize;
    
    int                 recflip;



    /** Analog mixer variables ***********************************************/
    
    unsigned char       mixerstate;
    

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

#endif /* AHI_Drivers_CMI8738_DriverData_h */
