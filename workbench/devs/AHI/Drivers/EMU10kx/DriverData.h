/*
     emu10kx.audio - AHI driver for SoundBlaster Live! series
     Copyright (C) 2002-2005 Martin Blom <martin@blom.org>
     
     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.
     
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
     
     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef AHI_Drivers_EMU10kx_DriverData_h
#define AHI_Drivers_EMU10kx_DriverData_h

#include <exec/types.h>
#include <exec/interrupts.h>
#include <devices/ahi.h>

#include "emu10kx-ac97.h"
#include "emu10kx-camd.h"
#include "hwaccess.h"
#include "voicemgr.h"

/** Make the common library code initialize a global SysBase for us.
    It's required for hwaccess.c */

#define DRIVER_NEEDS_GLOBAL_EXECBASE
#include "DriverBase.h"

struct EMU10kxData;

struct EMU10kxBase
{
    /** Skeleton's variables *************************************************/

    struct DriverBase      driverbase;


    /** The driver's global data *********************************************/

    
    /** A sempahore used for locking */
    struct SignalSemaphore semaphore;

    /** The number of cards found */
    int                    cards_found;

    /** A EMU10kxData structure for each card found */
    struct EMU10kxData**   driverdatas;

    /** The public CAMD interface */
    struct EMU10kxCamd     camd;

    /** The public AC97 interface */
    struct EMU10kxAC97     ac97;

    /** TRUE if the DMA buffers have to be flushed before played etc. */
    BOOL                   flush_caches;
};

#define DRIVERBASE_SIZEOF (sizeof (struct EMU10kxBase))

#define RECORD_BUFFER_SAMPLES     4096
#define RECORD_BUFFER_SIZE_VALUE  ADCBS_BUFSIZE_16384
#ifndef __AMIGAOS4__
#define TIMER_INTERRUPT_FREQUENCY 1000
#else
#define TIMER_INTERRUPT_FREQUENCY 80
#endif


struct EMU10kxData
{
    /** Skeleton's variables *************************************************/

    struct DriverData   driverdata;

    /*** PCI/EMU10kx initialization progress *********************************/

    /** TRUE if bus mastering is activated */
    BOOL                pci_master_enabled;

    /** TRUE if the EMU10kx chip has been initialized */
    BOOL                emu10k1_initialized;

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
    
    /*** EMU10kx structures **************************************************/
    
    struct emu10k1_card card;
    struct emu_voice    voices[4];

    UWORD               voice_buffers_allocated;
    UWORD               voices_allocated;
    UWORD               voices_started;
    UWORD               pad;

    
    /*** Playback interrupt variables ****************************************/

    /** The mixing buffer (a cyclic buffer filled by AHI) */
    APTR                mix_buffer;

    /** The length of each playback buffer in sample frames */
    ULONG               current_length;
    
    /** The length of each playback buffer in sample bytes */
    ULONG               current_size;

    /** Where (inside the cyclic buffer) we're currently writing */
    APTR                current_buffers[4];

    /** The offset (inside the cyclic buffer) we're currently writing */
    ULONG               current_position;


    /*** Recording interrupt variables ***************************************/

    /** The recording buffer (simple double buffering is used */
    APTR                record_buffer;

    /** The DMA handle for the record buffer */
    dma_addr_t          record_dma_handle;

    /** Were (inside the recording buffer) the current data is */
    APTR                current_record_buffer;
    
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

#endif /* AHI_Drivers_EMU10kx_DriverData_h */
