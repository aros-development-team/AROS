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

#include <config.h>

#undef __USE_INLINE__
#include <proto/expansion.h>
extern struct UtilityIFace*        IUtility;
extern struct AHIsubIFace*         IAHIsub;
extern struct MMUIFace*            IMMU;

#include <devices/ahi.h>
#include <exec/memory.h>
#include <libraries/ahi_sub.h>

#include <proto/ahi_sub.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#include "library_card.h"
#include "regs.h"
#include "misc.h"
#include "DriverData.h"

extern void rate_set_dac2(struct CardData *card, unsigned long rate);
extern void rate_set_adc(struct CardData *card, unsigned long rate);

/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

#define FREQUENCIES 11

static const ULONG Frequencies[ FREQUENCIES ] =
{
  5500,
  8000,     /* µ- and A-Law */
  9600,
  11025,    /* CD/4 */
  16000,    /* DAT/3 */
  19200,
  22050,    /* CD/2 */
  32000,    /* DAT/1.5 */
  38400,
  44100,    /* CD */
  48000     /* DAT */
};

#define INPUTS 6

static const STRPTR Inputs[ INPUTS ] =
{
  "Line in",
  "Mic",
  "CD",
  "Aux",
  "Mixer",
  "Phone"
};

/* Not static since it's used in misc.c too */
const UWORD InputBits[ INPUTS ] =
{  
  AC97_RECMUX_LINE,
  AC97_RECMUX_MIC,
  AC97_RECMUX_CD,
  AC97_RECMUX_AUX,
  AC97_RECMUX_STEREO_MIX,
  AC97_RECMUX_PHONE
};


#define OUTPUTS 1

static const STRPTR Outputs[ OUTPUTS ] =
{
  "Front",
};


/******************************************************************************
** AHIsub_AllocAudio **********************************************************
******************************************************************************/

ULONG
_AHIsub_AllocAudio( struct TagItem*         taglist,
		    struct AHIAudioCtrlDrv* AudioCtrl,
		    struct DriverBase*      AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;

  int   card_num;
  ULONG ret;
  int   i, freq = 9;

  card_num = ( IUtility->GetTagData( AHIDB_AudioID, 0, taglist) & 0x0000f000 ) >> 12;
  
  if( card_num >= CardBase->cards_found ||
      CardBase->driverdatas[ card_num ] == NULL )
  {
    IExec->DebugPrintF("no data for card = %ld\n", card_num);
    Req( "No CardData for card %ld.", card_num );
    return AHISF_ERROR;
  }
  else
  {
    struct CardData* card;
    BOOL in_use;
    struct PCIDevice *dev;

    card  = CardBase->driverdatas[ card_num ];
    AudioCtrl->ahiac_DriverData = card;

    IExec->ObtainSemaphore( &CardBase->semaphore );
    in_use = ( card->audioctrl != NULL );
    if( !in_use )
    {
      card->audioctrl = AudioCtrl;
    }
    IExec->ReleaseSemaphore( &CardBase->semaphore );

    if( in_use )
    {
      return AHISF_ERROR;
    }
    
    dev = card->pci_dev;
    card->playback_interrupt_enabled = FALSE;
    card->record_interrupt_enabled = FALSE;
    /* Clears playback/record interrupts */
    dev->OutLong(card->iobase + SB128_SCON, (dev->InLong(card->iobase + SB128_SCON)) & SB128_IRQ_MASK);
   
   for( i = 1; i < FREQUENCIES; i++ )
   {
      if( (ULONG) Frequencies[ i ] > AudioCtrl->ahiac_MixFreq )
      {
         if ( ( AudioCtrl->ahiac_MixFreq - (LONG) Frequencies[ i - 1 ] ) < ( (LONG) Frequencies[ i ] - AudioCtrl->ahiac_MixFreq ) )
         {
            freq = i-1;
            break;
         }
         else
         {
            freq = i;
            break;
         }
      }
   }
   
  }

  ret = AHISF_KNOWHIFI | AHISF_KNOWSTEREO | AHISF_MIXING | AHISF_TIMING;


  for( i = 0; i < FREQUENCIES; ++i )
  {
    if( AudioCtrl->ahiac_MixFreq == Frequencies[ i ] )
    {
      ret |= AHISF_CANRECORD;
      break;
    }
  }

  return ret;
}



/******************************************************************************
** AHIsub_FreeAudio ***********************************************************
******************************************************************************/

void
_AHIsub_FreeAudio( struct AHIAudioCtrlDrv* AudioCtrl,
		   struct DriverBase*      AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;
  struct CardData* card = (struct CardData*) AudioCtrl->ahiac_DriverData;

  if( card != NULL )
  {
    IExec->ObtainSemaphore( &CardBase->semaphore );
    if( card->audioctrl == AudioCtrl )
    {
      /* Release it if we own it. */
      card->audioctrl = NULL;
    }
    IExec->ReleaseSemaphore( &CardBase->semaphore );

    AudioCtrl->ahiac_DriverData = NULL;
  }
}


/******************************************************************************
** AHIsub_Disable *************************************************************
******************************************************************************/

void
_AHIsub_Disable( struct AHIAudioCtrlDrv* AudioCtrl,
		 struct DriverBase*      AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;

  /* V6 drivers do not have to preserve all registers */

  IExec->Disable();
}


/******************************************************************************
** AHIsub_Enable **************************************************************
******************************************************************************/

void
_AHIsub_Enable( struct AHIAudioCtrlDrv* AudioCtrl,
		struct DriverBase*      AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;

  /* V6 drivers do not have to preserve all registers */

  IExec->Enable();
}


/******************************************************************************
** AHIsub_Start ***************************************************************
******************************************************************************/

ULONG
_AHIsub_Start( ULONG                   flags,
	       struct AHIAudioCtrlDrv* AudioCtrl,
	       struct DriverBase*      AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;
  struct CardData* card = (struct CardData*) AudioCtrl->ahiac_DriverData;
  struct PCIDevice *dev = card->pci_dev;
  unsigned long PlayCtrlFlags = 0, RecCtrlFlags = 0;
  ULONG dma_buffer_size = 0;
  int i, freqbit = 9;
  unsigned long scon = 0;
  APTR stack;

  for( i = 0; i < FREQUENCIES; ++i )
  {
    if( AudioCtrl->ahiac_MixFreq == Frequencies[ i ] )
    {
      freqbit = i;
      break;
    }
  }

  if( flags & AHISF_PLAY )
  {
    
    ULONG dma_sample_frame_size;
    int i;
    short *a;
    unsigned short cod, ChannelsFlag;

    ChannelsFlag = 0;
    
    /* Update cached/syncronized variables */

    IAHIsub->AHIsub_Update( AHISF_PLAY, AudioCtrl );

    /* Allocate a new mixing buffer. Note: The buffer must be cleared, since
       it might not be filled by the mixer software interrupt because of
       pretimer/posttimer! */

    card->mix_buffer = IExec->AllocVec( AudioCtrl->ahiac_BuffSize,
			       MEMF_PUBLIC | MEMF_CLEAR );

    if( card->mix_buffer == NULL )
    {
      Req( "Unable to allocate %ld bytes for mixing buffer.",
	   AudioCtrl->ahiac_BuffSize );
      return AHIE_NOMEM;
    }

    /* Allocate a buffer large enough for 16-bit double-buffered
       playback (mono or stereo) */

    if( AudioCtrl->ahiac_Flags & AHIACF_STEREO )
    {
      dma_sample_frame_size = 4;
      dma_buffer_size = AudioCtrl->ahiac_MaxBuffSamples * dma_sample_frame_size;
      ChannelsFlag = SB128_STEREO;
    }
    else
    {
      dma_sample_frame_size = 2;
      dma_buffer_size = AudioCtrl->ahiac_MaxBuffSamples * dma_sample_frame_size;
    }

    card->playback_buffer = pci_alloc_consistent(dma_buffer_size * 2, &card->playback_buffer_nonaligned);

    if (!card->playback_buffer)
    {
      Req( "Unable to allocate playback buffer." );
      return AHIE_NOMEM;
    }

    /* Enable Playback interrupt */
    dev->OutLong(card->iobase + SB128_SCON, (dev->InLong(card->iobase + SB128_SCON) | SB128_DAC2_INTEN));

    card->current_bytesize = dma_buffer_size;
    card->current_frames = AudioCtrl->ahiac_MaxBuffSamples;
    card->current_buffer   = card->playback_buffer + card->current_bytesize;
    card->playback_interrupt_enabled = TRUE;
   
    card->flip = 0; 

    /* Select the DAC2 Memory Page */
    dev->OutLong(card->iobase + SB128_MEMPAGE, SB128_PAGE_DAC);
  
    /* Buffer address and length (in longwords) is set */
    stack = IExec->SuperState();
    card->playback_buffer_phys = IMMU->GetPhysicalAddress(card->playback_buffer);
    IExec->UserState(stack);
    
    dev->OutLong(card->iobase + SB128_DAC2_FRAME, (unsigned long)(card->playback_buffer_phys));
    dev->OutLong(card->iobase + SB128_DAC2_COUNT, (((dma_buffer_size * 2) >> 2) - 1) & 0xFFFF);

    /* Playback format is always 16 Bit, Stereo, but checks exist in case of a Mono mode (not possible). */
    PlayCtrlFlags = (SB128_16BIT | ChannelsFlag) << 2;
 
    /* Set frequency */
    if (card->currentPlayFreq != freqbit)
    {
      rate_set_dac2(card, Frequencies[freqbit]);
      card->currentPlayFreq = freqbit; 
    }
    card->is_playing = TRUE;
  }

  if( flags & AHISF_RECORD )
  {
    UWORD mask;

    card->current_record_bytesize = RECORD_BUFFER_SAMPLES * 4;

    /* Allocate a new recording buffer (page aligned!) */
    card->record_buffer = pci_alloc_consistent(card->current_record_bytesize * 2, &card->record_buffer_nonaligned);

    if( card->record_buffer == NULL )
    {
      Req( "Unable to allocate %ld bytes for the recording buffer.", card->current_record_bytesize);
      return AHIE_NOMEM;
    }

    SaveMixerState( card );
    UpdateMonitorMixer( card );

    /* Enable record interrupt */
    dev->OutLong(card->iobase + SB128_SCON, (dev->InLong(card->iobase + SB128_SCON) | SB128_ADC_INTEN));
    
    card->record_interrupt_enabled = TRUE;

    card->recflip = 0;
   
    /* Select the ADC Memory Page */
    dev->OutLong(card->iobase + SB128_MEMPAGE, SB128_PAGE_ADC);

    /* Buffer address and length (in longwords) is set */
    stack = IExec->SuperState();
    card->record_buffer_phys = IMMU->GetPhysicalAddress(card->record_buffer);
    IExec->UserState(stack);

    dev->OutLong(card->iobase + SB128_ADC_FRAME, (unsigned long) card->record_buffer_phys);
    dev->OutLong(card->iobase + SB128_ADC_COUNT, (((card->current_record_bytesize * 2) >> 2) - 1) & 0xFFFF);

    card->is_recording = TRUE;
    card->current_record_buffer = card->record_buffer + card->current_record_bytesize;
    
    /* Record format is always 16 Bit, Stereo */
    RecCtrlFlags = (SB128_16BIT | SB128_STEREO) << 4;

    /* Set frequency */
    if (card->currentRecFreq != freqbit)
    {
      rate_set_adc(card, Frequencies[freqbit]);
      card->currentRecFreq = freqbit; 
    }
  }

   if( flags & AHISF_PLAY )
   {
     /* Set Sample Count per Interrupt */
     if (PlayCtrlFlags & 0x04)
       dev->OutLong(card->iobase + SB128_DAC2_SCOUNT, ((dma_buffer_size >> 2) - 1) & 0xFFFF);
     else
       dev->OutLong(card->iobase + SB128_DAC2_SCOUNT, ((dma_buffer_size >> 1) - 1) & 0xFFFF);
     /* Set format, ENDINC set to 2 */
     dev->OutLong(card->iobase + SB128_SCON, (dev->InLong(card->iobase + SB128_SCON) | PlayCtrlFlags | 2 << 19));
     /* Start playback! */
     dev->OutLong(card->iobase + SB128_CONTROL, (dev->InLong(card->iobase + SB128_CONTROL) | CTRL_DAC2_EN));
   }

   if( flags & AHISF_RECORD )
   {
     /* Set Sample Count per Interrupt */
     dev->OutLong(card->iobase + SB128_ADC_SCOUNT, ((card->current_record_bytesize >> 2) - 1) & 0xFFFF);
     /* Set format */
     dev->OutLong(card->iobase + SB128_SCON, (dev->InLong(card->iobase + SB128_SCON) | RecCtrlFlags));
     /* Start recording! */
     dev->OutLong(card->iobase + SB128_CONTROL, (dev->InLong(card->iobase + SB128_CONTROL) | CTRL_ADC_EN));
   }
  
   return AHIE_OK;
}


/******************************************************************************
** AHIsub_Update **************************************************************
******************************************************************************/

void
_AHIsub_Update( ULONG                   flags,
		struct AHIAudioCtrlDrv* AudioCtrl,
		struct DriverBase*      AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;
  struct CardData* card = (struct CardData*) AudioCtrl->ahiac_DriverData;

  card->current_frames = AudioCtrl->ahiac_BuffSamples;

  if( AudioCtrl->ahiac_Flags & AHIACF_STEREO )
  {
    card->current_bytesize = card->current_frames * 4;
  }
  else
  {
    card->current_bytesize = card->current_frames * 2;
  }
}


/******************************************************************************
** AHIsub_Stop ****************************************************************
******************************************************************************/

void
_AHIsub_Stop( ULONG                   flags,
	      struct AHIAudioCtrlDrv* AudioCtrl,
	      struct DriverBase*      AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;
  struct CardData* card = (struct CardData*) AudioCtrl->ahiac_DriverData;
  struct PCIDevice *dev = card->pci_dev;


  if( flags & AHISF_PLAY )
  {
    unsigned long play_ctl;
    card->is_playing= FALSE;

    play_ctl = dev->InLong(card->iobase + SB128_CONTROL);
    play_ctl &= ~(CTRL_DAC2_EN);
    /* Stop */
    dev->OutLong(card->iobase + SB128_CONTROL, play_ctl); 

    /* Clear and mask interrupts */
    dev->OutLong(card->iobase + SB128_SCON, (dev->InLong(card->iobase + SB128_SCON)) & SB128_IRQ_MASK);

    if (card->current_bytesize > 0)
       pci_free_consistent(card->playback_buffer_nonaligned);

    card->current_bytesize     = 0;
    card->current_frames = 0;
    card->current_buffer   = NULL;

    if ( card->mix_buffer)
       IExec->FreeVec( card->mix_buffer );
    card->mix_buffer = NULL;
    card->playback_interrupt_enabled = FALSE;
    card->current_bytesize = 0;
  }

  if( flags & AHISF_RECORD )
  {
    unsigned long rec_ctl, val;

    rec_ctl = dev->InLong(card->iobase + SB128_CONTROL);
    rec_ctl &= ~(CTRL_ADC_EN);
    /* Stop */
    dev->OutLong(card->iobase + SB128_CONTROL, rec_ctl); 

    /* Clear and mask interrupts */
    dev->OutLong(card->iobase + SB128_SCON, (dev->InLong(card->iobase + SB128_SCON)) & SB128_IRQ_MASK);

    if( card->is_recording )
    {
      /* Do not restore mixer unless they have been saved */
      RestoreMixerState( card );
    }

    if( card->record_buffer != NULL )
    {
      pci_free_consistent( card->record_buffer_nonaligned);
    }

    card->record_buffer = NULL;
    card->current_record_bytesize = 0;

    card->is_recording = FALSE;
    card->record_interrupt_enabled = FALSE;
  }
  
  
}


/******************************************************************************
** AHIsub_GetAttr *************************************************************
******************************************************************************/

LONG
_AHIsub_GetAttr( ULONG                   attribute,
		 LONG                    argument,
		 LONG                    def,
		 struct TagItem*         taglist,
		 struct AHIAudioCtrlDrv* AudioCtrl,
		 struct DriverBase*      AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;
  int i;


  switch( attribute )
  {
    case AHIDB_Bits:
      return 16;

    case AHIDB_Frequencies:
      return FREQUENCIES;

    case AHIDB_Frequency: /* Index->Frequency */
      return (LONG) Frequencies[ argument ];

    case AHIDB_Index: /* Frequency->Index */
      if( argument <= (LONG) Frequencies[ 0 ] )
      {
        return 0;
      }

      if( argument >= (LONG) Frequencies[ FREQUENCIES - 1 ] )
      {
        return FREQUENCIES-1;
      }

      for( i = 1; i < FREQUENCIES; i++ )
      {
        if( (LONG) Frequencies[ i ] > argument )
        {
          if( ( argument - (LONG) Frequencies[ i - 1 ] ) < ( (LONG) Frequencies[ i ] - argument ) )
          {
            return i-1;
          }
          else
          {
            return i;
          }
        }
      }

      return 0;  /* Will not happen */

    case AHIDB_Author:
      return (LONG) "Ross Vumbaca";

    case AHIDB_Copyright:
      return (LONG) "(C) Ross Vumbaca";

    case AHIDB_Version:
      return (LONG) LibIDString;

    case AHIDB_Annotation:
      return (LONG)
   	"OS4 PPC native driver";

    case AHIDB_Record:
      return TRUE;

    case AHIDB_FullDuplex:
      return TRUE;

    case AHIDB_Realtime:
      return TRUE;

    case AHIDB_MaxRecordSamples:
      return RECORD_BUFFER_SAMPLES;

    /* formula's:
    #include <math.h>
    
    unsigned long res = (unsigned long) (0x10000 * pow (10.0, dB / 20.0));
    double dB = 20.0 * log10(0xVALUE / 65536.0);
   
    printf("dB = %f, res = %lx\n", dB, res);*/

    case AHIDB_MinMonitorVolume:
      return 0x004d2;

    case AHIDB_MaxMonitorVolume:
      return 0x3fb27;

    case AHIDB_MinInputGain:
      return 0x10000; /* 0.0 dB gain */

    case AHIDB_MaxInputGain:
      return 0xD55D0; /* 22.5 dB gain */

    case AHIDB_MinOutputVolume:
      return 0x004d2; /* -34.5 dB / mute */

    case AHIDB_MaxOutputVolume:
      return 0x3fb27; /* 12 dB */

    case AHIDB_Inputs:
      return INPUTS;

    case AHIDB_Input:
      return (LONG) Inputs[ argument ];

    case AHIDB_Outputs:
      return OUTPUTS;

    case AHIDB_Output:
      return (LONG) Outputs[ argument ];

    default:
      return def;
  }
}


/******************************************************************************
** AHIsub_HardwareControl *****************************************************
******************************************************************************/

ULONG
_AHIsub_HardwareControl( ULONG                   attribute,
			 LONG                    argument,
			 struct AHIAudioCtrlDrv* AudioCtrl,
			 struct DriverBase*      AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;
  struct CardData* card = (struct CardData*) AudioCtrl->ahiac_DriverData;

  switch( attribute )
  {
    case AHIC_MonitorVolume:
      card->monitor_volume = Linear2MixerGain( (Fixed) argument, &card->monitor_volume_bits );
      if( card->is_recording )
      {
         UpdateMonitorMixer( card );
      }
      return TRUE;

    case AHIC_MonitorVolume_Query:
      return card->monitor_volume;

    case AHIC_InputGain:
      card->input_gain = Linear2RecordGain( (Fixed) argument, &card->input_gain_bits );
      if(card->es1370)
      {
        /* Not supported on ES1370 */
      }
      else
        codec_write(card, AC97_RECORD_GAIN, card->input_gain_bits );
      return TRUE;

    case AHIC_InputGain_Query:
      return card->input_gain;

    case AHIC_OutputVolume:
      card->output_volume = Linear2MixerGain( (Fixed) argument, &card->output_volume_bits );
      if(card->es1370)
      {
        ak4531_ac97_write(card, AC97_PCMOUT_VOL, card->output_volume_bits );
      }
      else
        codec_write(card, AC97_PCMOUT_VOL, card->output_volume_bits );
      return TRUE;

    case AHIC_OutputVolume_Query:
      return card->output_volume;

    case AHIC_Input:
      card->input = argument;
      if(card->es1370)
      {
        ak4531_ac97_write(card, AC97_RECORD_SELECT, InputBits[ card->input ] );
      }
      else
        codec_write(card, AC97_RECORD_SELECT, InputBits[ card->input ] );

      if( card->is_recording )
      {
         UpdateMonitorMixer( card );
      }
      return TRUE;

    case AHIC_Input_Query:
      return card->input;

    case AHIC_Output:
      card->output = argument;

      if( card->output == 0 )
      {
      }
      else
      {
      }
      return TRUE;

    case AHIC_Output_Query:
      return card->output;

    default:
      return FALSE;
  }
}
