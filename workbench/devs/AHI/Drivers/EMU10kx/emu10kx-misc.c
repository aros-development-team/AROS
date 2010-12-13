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

#include <config.h>

#include <exec/memory.h>
#include <proto/dos.h>

#include "library.h"
#include "8010.h"
#include "emu10kx-interrupt.h"
#include "emu10kx-misc.h"
#include "pci_wrapper.h"

/* Global in emu10kx.c */
extern const UWORD InputBits[];

/* Public functions in main.c */
int emu10k1_init(struct emu10k1_card *card);
void emu10k1_cleanup(struct emu10k1_card *card);

static void AddResetHandler(struct EMU10kxData* dd);

/******************************************************************************
** DriverData allocation ******************************************************
******************************************************************************/

// This code used to be in _AHIsub_AllocAudio(), but since we're now
// handling CAMD support too, it needs to be done at driver loading
// time.

INTGW( static, void,  playbackinterrupt, PlaybackInterrupt );
INTGW( static, void,  recordinterrupt,   RecordInterrupt );
INTGW( static, ULONG, emu10kxinterrupt,  EMU10kxInterrupt );


struct EMU10kxData*
AllocDriverData( APTR               dev,
		 struct DriverBase* AHIsubBase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;
  struct EMU10kxData* dd;
  UWORD               command_word;

  // FIXME: This should be non-cachable, DMA-able memory
  dd = AllocVec( sizeof( *dd ), MEMF_PUBLIC | MEMF_CLEAR );

  if( dd == NULL )
  {
    Req( "Unable to allocate driver structure." );
    return NULL;
  }


  dd->ahisubbase = AHIsubBase;

  dd->interrupt.is_Node.ln_Type = INTERRUPT_NODE_TYPE;
  dd->interrupt.is_Node.ln_Pri  = 0;
  dd->interrupt.is_Node.ln_Name = (STRPTR) LibName;
  dd->interrupt.is_Code         = (void(*)(void)) &emu10kxinterrupt;
  dd->interrupt.is_Data         = (APTR) dd;

  dd->playback_interrupt.is_Node.ln_Type = INTERRUPT_NODE_TYPE;
  dd->playback_interrupt.is_Node.ln_Pri  = 0;
  dd->playback_interrupt.is_Node.ln_Name = (STRPTR) LibName;
  dd->playback_interrupt.is_Code         = (void(*)(void)) &playbackinterrupt;
  dd->playback_interrupt.is_Data         = (APTR) dd;

  dd->record_interrupt.is_Node.ln_Type = INTERRUPT_NODE_TYPE;
  dd->record_interrupt.is_Node.ln_Pri  = 0;
  dd->record_interrupt.is_Node.ln_Name = (STRPTR) LibName;
  dd->record_interrupt.is_Code         = (void(*)(void)) &recordinterrupt;
  dd->record_interrupt.is_Data         = (APTR) dd;
  
  dd->card.pci_dev = dev;

//  if( pci_set_dma_mask(dd->card.pci_dev, EMU10K1_DMA_MASK) )
//  {
//    printf( "Unable to set DMA mask for card." );
//    goto error;
//  }

  command_word = ahi_pci_read_config_word( PCI_COMMAND, dd->card.pci_dev );
  command_word |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
  ahi_pci_write_config_word( PCI_COMMAND, command_word, dd->card.pci_dev );

  dd->pci_master_enabled = TRUE;

  // FIXME: How about latency/pcibios_set_master()??

  dd->card.iobase  = (unsigned long) ahi_pci_get_base_address(0, dev);
  dd->card.length  = ~( ahi_pci_get_base_size(0, dev) & PCI_BASE_ADDRESS_IO_MASK );
  dd->card.irq     = ahi_pci_get_irq(dev);
  dd->card.chiprev = ahi_pci_read_config_byte( PCI_REVISION_ID, dev );
  dd->card.model   = ahi_pci_read_config_word( PCI_SUBSYSTEM_ID, dev );
  dd->card.is_audigy = ( ahi_pci_read_config_word(PCI_DEVICE_ID, dev) == PCI_DEVICE_ID_CREATIVE_AUDIGY );
  dd->card.is_aps   = ( ahi_pci_read_config_long( PCI_SUBSYSTEM_VENDOR_ID, dev) == EMU_APS_SUBID );

  ahi_pci_add_intserver( &dd->interrupt, dev);

  dd->interrupt_added = TRUE;

  /* Initialize chip */
  if( emu10k1_init( &dd->card ) < 0 )
  {
    Req( "Unable to initialize EMU10kx subsystem.");
    return NULL;
  }

  dd->emu10k1_initialized = TRUE;

    
  /* Initialize mixer */

  emu10k1_writeac97( &dd->card, AC97_RESET, 0L);

  Delay( 1 );

  if (emu10k1_readac97( &dd->card, AC97_RESET ) & 0x8000) {
    Req( "ac97 codec not present.");
    return NULL;
  }


  dd->input          = 0;
  dd->output         = 0;
  dd->monitor_volume = Linear2MixerGain( 0, &dd->monitor_volume_bits );
  dd->input_gain     = Linear2RecordGain( 0x10000, &dd->input_gain_bits );
  dd->output_volume  = Linear2MixerGain( 0x10000, &dd->output_volume_bits );

  // No attenuation and natural tone for all outputs
  emu10k1_writeac97( &dd->card, AC97_MASTER_VOL_STEREO, 0x0000 );
  emu10k1_writeac97( &dd->card, AC97_MASTER_VOL_MONO,   0x0000 );
  emu10k1_writeac97( &dd->card, AC97_MASTER_TONE,       0x0f0f );

  emu10k1_writeac97( &dd->card, AC97_RECORD_GAIN,       0x0000 );
  emu10k1_writeac97( &dd->card, AC97_RECORD_SELECT,     InputBits[ 0 ] );

  emu10k1_writeac97( &dd->card, AC97_PCMOUT_VOL,        0x0808 );
  emu10k1_writeac97( &dd->card, AC97_PCBEEP_VOL,        0x0000 );

  emu10k1_writeac97( &dd->card, AC97_LINEIN_VOL,        0x0808 );
  emu10k1_writeac97( &dd->card, AC97_MIC_VOL,           AC97_MUTE | 0x0008 );
  emu10k1_writeac97( &dd->card, AC97_CD_VOL,            0x0808 );
  emu10k1_writeac97( &dd->card, AC97_AUX_VOL,           0x0808 );
  emu10k1_writeac97( &dd->card, AC97_PHONE_VOL,         0x0008 );
  emu10k1_writeac97( &dd->card, AC97_VIDEO_VOL,         0x0808 );


  if (emu10k1_readac97( &dd->card, AC97_EXTENDED_ID ) & 0x0080 )
  {
    sblive_writeptr( &dd->card, AC97SLOT, 0, AC97SLOT_CNTR | AC97SLOT_LFE);

    // Disable center/LFE to front speakers (Not headphone; it's actially surround mix.)
    emu10k1_writeac97( &dd->card, AC97_HEADPHONE_VOL, AC97_MUTE | 0x0808 ); 

    // No attenuation for center/LFE
    emu10k1_writeac97( &dd->card, AC97_SURROUND_MASTER, 0x0 );
  }

#ifdef __AMIGAOS4__
  AddResetHandler(dd);
#endif
  
  return dd;
}


/******************************************************************************
** DriverData deallocation ****************************************************
******************************************************************************/

// And this code used to be in _AHIsub_FreeAudio().

void
FreeDriverData( struct EMU10kxData* dd,
		struct DriverBase*  AHIsubBase )
{
  if( dd != NULL )
  {
    if( dd->card.pci_dev != NULL )
    {
      if( dd->emu10k1_initialized )
      {
        emu10k1_cleanup( &dd->card );
      }

      if( dd->pci_master_enabled )
      {
        UWORD cmd;

        cmd = ahi_pci_read_config_word( PCI_COMMAND, dd->card.pci_dev );
        cmd &= ~( PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER );
        ahi_pci_write_config_word( PCI_COMMAND, cmd, dd->card.pci_dev );
      }
    }

    if( dd->interrupt_added )
    {
      ahi_pci_rem_intserver( &dd->interrupt, dd->card.pci_dev );
    }

    FreeVec( dd );
  }
}

/******************************************************************************
** Misc. **********************************************************************
******************************************************************************/

void
SaveMixerState( struct EMU10kxData* dd )
{
  dd->ac97_mic    = emu10k1_readac97( &dd->card, AC97_MIC_VOL );
  dd->ac97_cd     = emu10k1_readac97( &dd->card, AC97_CD_VOL );
  dd->ac97_video  = emu10k1_readac97( &dd->card, AC97_VIDEO_VOL );
  dd->ac97_aux    = emu10k1_readac97( &dd->card, AC97_AUX_VOL );
  dd->ac97_linein = emu10k1_readac97( &dd->card, AC97_LINEIN_VOL );
  dd->ac97_phone  = emu10k1_readac97( &dd->card, AC97_PHONE_VOL );
}


void
RestoreMixerState( struct EMU10kxData* dd )
{
  emu10k1_writeac97( &dd->card, AC97_MIC_VOL,    dd->ac97_mic );
  emu10k1_writeac97( &dd->card, AC97_CD_VOL,     dd->ac97_cd );
  emu10k1_writeac97( &dd->card, AC97_VIDEO_VOL,  dd->ac97_video );
  emu10k1_writeac97( &dd->card, AC97_AUX_VOL,    dd->ac97_aux );
  emu10k1_writeac97( &dd->card, AC97_LINEIN_VOL, dd->ac97_linein );
  emu10k1_writeac97( &dd->card, AC97_PHONE_VOL,  dd->ac97_phone );
}

void
UpdateMonitorMixer( struct EMU10kxData* dd )
{
  int   i  = InputBits[ dd->input ];
  UWORD m  = dd->monitor_volume_bits & 0x801f;
  UWORD s  = dd->monitor_volume_bits;
  UWORD mm = AC97_MUTE | 0x0008;
  UWORD sm = AC97_MUTE | 0x0808;

  if( i == AC97_RECMUX_STEREO_MIX ||
      i == AC97_RECMUX_MONO_MIX )
  {
    // Use the original mixer settings
    RestoreMixerState( dd );
  }
  else
  {
    emu10k1_writeac97( &dd->card, AC97_MIC_VOL,
		       i == AC97_RECMUX_MIC ? m : mm );

    emu10k1_writeac97( &dd->card, AC97_CD_VOL,
		       i == AC97_RECMUX_CD ? s : sm );

    emu10k1_writeac97( &dd->card, AC97_VIDEO_VOL,
		       i == AC97_RECMUX_VIDEO ? s : sm );

    emu10k1_writeac97( &dd->card, AC97_AUX_VOL,
		       i == AC97_RECMUX_AUX ? s : sm );

    emu10k1_writeac97( &dd->card, AC97_LINEIN_VOL,
		       i == AC97_RECMUX_LINE ? s : sm );

    emu10k1_writeac97( &dd->card, AC97_PHONE_VOL,
		       i == AC97_RECMUX_PHONE ? m : mm );
  }
}


Fixed
Linear2MixerGain( Fixed  linear,
		  UWORD* bits )
{
  static const Fixed gain[ 33 ] =
  {
    260904, // +12.0 dB
    219523, // +10.5 dB
    184706, //  +9.0 dB
    155410, //  +7.5 dB
    130762, //  +6.0 dB
    110022, //  +4.5 dB
    92572,  //  +3.0 dB
    77890,  //  +1.5 dB
    65536,  //  ±0.0 dB
    55142,  //  -1.5 dB
    46396,  //  -3.0 dB
    39037,  //  -4.5 dB
    32846,  //  -6.0 dB
    27636,  //  -7.5 dB
    23253,  //  -9.0 dB
    19565,  // -10.5 dB
    16462,  // -12.0 dB
    13851,  // -13.5 dB
    11654,  // -15.0 dB
    9806,   // -16.5 dB
    8250,   // -18.0 dB
    6942,   // -19.5 dB
    5841,   // -21.0 dB
    4915,   // -22.5 dB
    4135,   // -24.0 dB
    3479,   // -25.5 dB
    2927,   // -27.0 dB
    2463,   // -28.5 dB
    2072,   // -30.0 dB
    1744,   // -31.5 dB
    1467,   // -33.0 dB
    1234,   // -34.5 dB
    0       //   -oo dB
  };

  int v = 0;

  while( linear < gain[ v ] )
  {
    ++v;
  }

  if( v == 32 )
  {
    *bits = 0x8000; // Mute
  }
  else
  {
    *bits = ( v << 8 ) | v;
  }

//  KPrintF( "l2mg %08lx -> %08lx (%04lx)\n", linear, gain[ v ], *bits );
  return gain[ v ];
}

Fixed
Linear2RecordGain( Fixed  linear,
		   UWORD* bits )
{
  static const Fixed gain[ 17 ] =
  {
    873937, // +22.5 dB
    735326, // +21.0 dB
    618700, // +19.5 dB
    520571, // +18.0 dB
    438006, // +16.5 dB
    368536, // +15.0 dB
    310084, // +13.5 dB
    260904, // +12.0 dB
    219523, // +10.5 dB
    184706, //  +9.0 dB
    155410, //  +7.5 dB
    130762, //  +6.0 dB
    110022, //  +4.5 dB
    92572,  //  +3.0 dB
    77890,  //  +1.5 dB
    65536,  //  ±0.0 dB
    0       //  -oo dB
  };

  int v = 0;

  while( linear < gain[ v ] )
  {
    ++v;
  }

  if( v == 16 )
  {
    *bits = 0x8000; // Mute
  }
  else
  {
    *bits = ( ( 15 - v ) << 8 ) | ( 15 - v );
  }

  return gain[ v ];
}


ULONG
SamplerateToLinearPitch( ULONG samplingrate )
{
  samplingrate = (samplingrate << 8) / 375;
  return (samplingrate >> 1) + (samplingrate & 1);
}


#ifdef __AMIGAOS4__
static ULONG ResetHandler(struct ExceptionContext *ctx, struct ExecBase *pExecBase, struct EMU10kxData* dd)
{
    emu10k1_irq_disable( &dd->card, INTE_INTERVALTIMERENB );
    emu10k1_voices_stop( dd->voices, dd->voices_started );

    return 0UL;
}


void AddResetHandler(struct EMU10kxData* dd)
{
    static struct Interrupt interrupt;

    interrupt.is_Code = (void (*)())ResetHandler;
    interrupt.is_Data = (APTR) dd;
    interrupt.is_Node.ln_Pri  = 0;
    interrupt.is_Node.ln_Type = NT_EXTINTERRUPT;
    interrupt.is_Node.ln_Name = "reset handler";

    AddResetCallback( &interrupt );
}
#endif
