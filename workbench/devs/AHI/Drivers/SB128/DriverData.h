/*

The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

The Original Code is (C) Copyright 2004-2011 Ross Vumbaca.

The Initial Developer of the Original Code is Ross Vumbaca.

All Rights Reserved.

*/

#ifndef AHI_Drivers_SB128_DriverData_h
#define AHI_Drivers_SB128_DriverData_h

#include <exec/types.h>
#include <exec/interrupts.h>
#include <devices/ahi.h>

#define DRIVER_NEEDS_GLOBAL_EXECBASE

#include "DriverBase.h"

struct SB128_DATA;

struct SB128Base
{
    /** Skeleton's variables *************************************************/

    struct DriverBase      driverbase;


    /** A sempahore used for locking */
    struct SignalSemaphore semaphore;

    /** The number of cards found */
    int                    cards_found;

    /** A SB128_DATA structure for each card found */
    struct SB128_DATA**   driverdatas;
};

#define DRIVERBASE_SIZEOF (sizeof (struct SB128Base))

#define RECORD_BUFFER_SAMPLES     1024


struct SB128_DATA
{
    /*** PCI/Card initialization progress *********************************/

  struct PCIDevice    *pci_dev;
  APTR       iobase;
  unsigned long		    length;
  unsigned short		  model;
  unsigned char       chiprev;
  unsigned int        irq;

    /** TRUE if bus mastering is activated */
    BOOL                pci_master_enabled;

    /** TRUE if the Card chip has been initialized */
    BOOL                card_initialized;

    /** TRUE if the card is an ES1370 */
    BOOL                es1370;

    /** A semaphore used for hardware access serialisation *******************/
    struct SignalSemaphore sb128_semaphore;


    /*** The driverbase ******************************************************/

    /** This field is also used as a lock and access to it is
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
    
    /** Saved state for AC97 video */
    UWORD               ac97_video;
    
    /** Saved state for AC97 aux */
    UWORD               ac97_aux;
    
    /** Saved state for AC97 line in */
    UWORD               ac97_linein;
    
    /** Saved state for AC97 phone */
    UWORD               ac97_phone;

    /** Saved state of AK4531 Output Register 1 */
    char                ak4531_output_1;

    /** Saved state of AK4531 Output Register 1 */
    char                ak4531_output_2;
    
    /** The current Playback Frequency */
    int                 currentPlayFreq;
    
    /** The current Record Frequency */
    int                 currentRecFreq;
};

#endif /* AHI_Drivers_SB128_DriverData_h */
