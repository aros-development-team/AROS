/*
    Copyright � 2004-2014, Davy Wentzler. All rights reserved.
    $Id$
*/

#include <config.h>

#undef __USE_INLINE__
#include <proto/expansion.h>
#ifdef __amigaos4__
extern struct UtilityIFace*        IUtility;
extern struct AHIsubIFace*         IAHIsub;
extern struct MMUIFace*            IMMU;
#endif

#include <devices/ahi.h>
#include <exec/memory.h>
#include <libraries/ahi_sub.h>

#include <proto/ahi_sub.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <math.h>
#include <string.h>

#include "library.h"
#include "regs.h"
#include "misc.h"
#include "pci_wrapper.h"
#include "DriverData.h"


/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

#define FREQUENCIES 13

static const ULONG Frequencies[ FREQUENCIES ] =
{
    8000,
    9600,
    11025,
    12000,
    16000,
    22050,
    24000,
    32000,
    44100, // CD
    48000,
    64000,
    88200,
    96000
};

static const ULONG FrequencyBits[ FREQUENCIES ] =
{
    6,
    3,
    10,
    2,
    5,
    9,
    1,
    4,
    8,
    0,
    15,
    11,
    7
};





#define INPUTS 5

static const STRPTR Inputs[ INPUTS ] =
{
  "Mic/Line 1 - 2",
  "Line 3 - 4",
  "Line 5 - 6",
  "Line 7 - 8",
  "S/PDIF",
};


#define INPUTS_2496 2

static const STRPTR Inputs_2496[ INPUTS ] =
{
  "Line",
  "S/PDIF",
};

#define OUTPUTS 5

static const STRPTR Outputs[ OUTPUTS ] =
{
  "Line 1 - 2",
  "Line 3 - 4",
  "Line 5 - 6",
  "Line 7 - 8",
  "S/PDIF",
};

#define OUTPUTS_2496 2

static const STRPTR Outputs_2496[ OUTPUTS ] =
{
  "Line",
  "S/PDIF",
};

#define INPUTS_DELTA44 2

static const STRPTR Inputs_Delta44[ INPUTS_DELTA44 ] =
{
  "Line 1 - 2",
  "Line 3 - 4"
};

#define OUTPUTS_DELTA44 1

static const STRPTR Outputs_Delta44[ OUTPUTS_DELTA44 ] =
{
  "Line 1 - 2"
//  "Line 3 - 4"
};


#define INPUTS_DELTA66 3

static const STRPTR Inputs_Delta66[ INPUTS_DELTA66 ] =
{
  "Line 1 - 2",
  "Line 3 - 4",
  "S/PDIF"
};

#define OUTPUTS_DELTA66 1

static const STRPTR Outputs_Delta66[ OUTPUTS_DELTA66 ] =
{
  "Line 1 - 2"
//  "Line 3 - 4",
//  "S/PDIF"
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

  card_num = ( GetTagData( AHIDB_AudioID, 0, taglist) & 0x0000f000 ) >> 12;
  
  if( card_num >= CardBase->cards_found ||
      CardBase->driverdatas[ card_num ] == NULL )
  {
    DebugPrintF("no data for card = %ld\n", card_num);
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

    ObtainSemaphore( &CardBase->semaphore );
    in_use = ( card->audioctrl != NULL );
    if( !in_use )
    {
      card->audioctrl = AudioCtrl;
    }
    ReleaseSemaphore( &CardBase->semaphore );

    if( in_use )
    {
      return AHISF_ERROR;
    }
    
    dev = card->pci_dev;
    card->playback_interrupt_enabled = FALSE;
    card->record_interrupt_enabled = FALSE;
    
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
    ObtainSemaphore( &CardBase->semaphore );
    if( card->audioctrl == AudioCtrl )
    {
      // Release it if we own it.
      card->audioctrl = NULL;
    }
    ReleaseSemaphore( &CardBase->semaphore );

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

  // V6 drivers do not have to preserve all registers

  Disable();
}


/******************************************************************************
** AHIsub_Enable **************************************************************
******************************************************************************/

void
_AHIsub_Enable( struct AHIAudioCtrlDrv* AudioCtrl,
		struct DriverBase*      AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;

  // V6 drivers do not have to preserve all registers

  Enable();
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
  UWORD PlayCtrlFlags = 0, RecCtrlFlags = 0;
  ULONG dma_buffer_size = 0;
  int i, freqbit = 9;
  APTR stack;

  /* Stop playback/recording, free old buffers (if any) */
  //IAHIsub->AHIsub_Stop( flags, AudioCtrl );

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
    unsigned short cod, ChannelsFlag = 0;

    /* Update cached/syncronized variables */

    _AHIsub_Update( AHISF_PLAY, AudioCtrl, AHIsubBase);

    /* Allocate a new mixing buffer. Note: The buffer must be cleared, since
       it might not be filled by the mixer software interrupt because of
       pretimer/posttimer! */

    card->mix_buffer = AllocVec( AudioCtrl->ahiac_BuffSize, MEMF_PUBLIC | MEMF_CLEAR );

    if( card->mix_buffer == NULL )
    {
        Req( "Unable to allocate %ld bytes for mixing buffer.", AudioCtrl->ahiac_BuffSize );
        return AHIE_NOMEM;
    }

    /* Allocate a buffer large enough for 16-bit double-buffered playback (only stereo) */

    dma_sample_frame_size = 40; // 5 stereo 32-bit streams
    dma_buffer_size = AudioCtrl->ahiac_MaxBuffSamples * dma_sample_frame_size;

    //DebugPrintF("dma_buffer_size = %ld, %lx (hex)\n", dma_buffer_size, dma_buffer_size);
 
    card->playback_buffer = pci_alloc_consistent(dma_buffer_size * 2, &card->playback_buffer_nonaligned, AHIsubBase);

    card->playback_buffer_phys = ahi_pci_logic_to_physic_addr(card->playback_buffer, card->pci_dev);

    if (!card->playback_buffer)
    {
        Req( "Unable to allocate playback buffer." );
        return AHIE_NOMEM;
    }
   
    ClearMask8(card, MT_DMA_CONTROL, MT_PLAY_START); // stop first

    pci_outb_mt(FrequencyBits[freqbit] & MT_RATE_MASK, MT_SAMPLERATE, card);
    //pci_outb_mt(0, MT_I2S_FORMAT, card);
    ClearMask8(card, MT_INTR_MASK_STATUS, MT_PLAY_MASK);

    card->current_bytesize = dma_buffer_size;
    card->current_frames = AudioCtrl->ahiac_MaxBuffSamples;
    card->current_buffer   = card->playback_buffer + card->current_bytesize;
    card->playback_interrupt_enabled = TRUE;

    card->flip = 0;
    
    pci_outl_mt((long) card->playback_buffer_phys, MT_DMA_PB_ADDRESS, card);
    pci_outw_mt((dma_buffer_size * 2) / 4 - 1, MT_DMA_PB_LENGTH, card);
    pci_outw_mt((dma_buffer_size * 1) / 4 - 1, MT_DMA_PB_INTLEN, card);
    
    card->is_playing = TRUE;

    //DebugPrintF("PLAY STARTED\n");
  }

  if( flags & AHISF_RECORD )
  {
    if ((card->SubType == PHASE88 && card->input >= 4) ||
        (card->SubType == MAUDIO_1010LT && card->input >= 4) ||
        (card->SubType == MAUDIO_2496 && card->input >= 1) ||
        (card->SubType == MAUDIO_DELTA66 && card->input >= 3) ) { // digital in
      
      WriteMask8(card, MT_SAMPLERATE, MT_SPDIF_MASTER);   // tbd!
      //DebugPrintF("REC: SPDIF is master\n");
      }
    
    WriteMask8(card, MT_SAMPLERATE, FrequencyBits[freqbit] & MT_RATE_MASK);
    card->current_record_bytesize_target = RECORD_BUFFER_SAMPLES * 4;
    card->current_record_bytesize_32bit = RECORD_BUFFER_SAMPLES * 48; // 12 tracks 32-bit

    //DebugPrintF("REC: current_record_bytesize = %ld\n", card->current_record_bytesize_32bit);
    
    /* Allocate a new recording buffer (page aligned!) */
    card->record_buffer = pci_alloc_consistent(card->current_record_bytesize_target * 2, &card->record_buffer_nonaligned, AHIsubBase);
    card->record_buffer_32bit = pci_alloc_consistent(card->current_record_bytesize_32bit * 2, &card->record_buffer_nonaligned_32bit, AHIsubBase);

    if( card->record_buffer == NULL || card->record_buffer_32bit == NULL)
    {
      Req( "Unable to allocate %ld bytes for the recording buffer.", card->current_record_bytesize_32bit);
      return AHIE_NOMEM;
    }

    ClearMask8(card, MT_DMA_CONTROL, MT_REC_START); // stop
    pci_outb_mt(FrequencyBits[freqbit] & MT_RATE_MASK, MT_SAMPLERATE, card);
    ClearMask8(card, MT_INTR_MASK_STATUS, MT_REC_MASK);
    
    
    card->current_record_buffer = card->record_buffer + card->current_record_bytesize_target; 
    card->current_record_buffer_32bit = card->record_buffer_32bit + card->current_record_bytesize_32bit;

    card->record_buffer_32bit_phys = ahi_pci_logic_to_physic_addr(card->record_buffer_32bit, card->pci_dev);

    pci_outl_mt((long) card->record_buffer_32bit_phys, MT_DMA_REC_ADDRESS, card);
    pci_outw_mt((card->current_record_bytesize_32bit * 2) / 4 - 1, MT_DMA_REC_LENGTH, card);
    pci_outw_mt((card->current_record_bytesize_32bit * 1) / 4 - 1, MT_DMA_REC_INTLEN, card);

    card->record_interrupt_enabled = TRUE;
    card->recflip = 0;
    
    card->is_recording = TRUE;
  }

  for (i = 0x60; i < 0x64; i++)
        DebugPrintF("config %lx = %x\n", i, inb_config(i, card->pci_dev));

    for (i = 0x0; i < 0x1E; i++)
        DebugPrintF("CCS %lx = %x\n", i, pci_inb(i, card));

    for (i = 0x0; i < 0x31; i++)
        DebugPrintF("CCI %lx = %x\n", i, ReadCCI(card, i));

    for (i = 0x0; i < 0x34; i++)
        DebugPrintF("MT %lx = %x\n", i, pci_inb_mt(i, card));

   if( flags & AHISF_PLAY )
   {
      WriteMask8(card, MT_DMA_CONTROL, MT_PLAY_START);
   }

   if( flags & AHISF_RECORD )
   {
      //DebugPrintF("REC STARTED, freqbit = %ld\n", freqbit);
      WriteMask8(card, MT_DMA_CONTROL, MT_REC_START);
   }

   return AHIE_OK;
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

  if( (flags & AHISF_PLAY) && card->is_playing == TRUE)
  {
    card->is_playing= FALSE;

    //DebugPrintF("STOPPING\n");
    ClearMask8(card, MT_DMA_CONTROL, MT_PLAY_START);
    WriteMask8(card, MT_INTR_MASK_STATUS, MT_PLAY_MASK);

    if (card->playback_buffer) {
       pci_free_consistent(card->playback_buffer_nonaligned, AHIsubBase);
       }

    card->current_bytesize     = 0;
    card->current_frames = 0;
    card->current_buffer   = NULL;

    if ( card->mix_buffer)
       FreeVec( card->mix_buffer );
    card->mix_buffer = NULL;
    card->playback_interrupt_enabled = FALSE;
    card->current_bytesize = 0;
  }
  
  if( (flags & AHISF_RECORD) && card->is_recording == TRUE)
  {
    unsigned short rec_ctl, val;

    card->is_recording = FALSE;
    
    //DebugPrintF("STOPPING REC\n");
    ClearMask8(card, MT_DMA_CONTROL, MT_REC_START);
    WriteMask8(card, MT_INTR_MASK_STATUS, MT_REC_MASK);
    
    if( card->record_buffer != NULL )
    {
      pci_free_consistent( card->record_buffer_nonaligned, AHIsubBase);
      pci_free_consistent( card->record_buffer_nonaligned_32bit, AHIsubBase);
    }

    card->record_buffer = NULL;
    card->current_record_bytesize_32bit = 0;
    card->record_buffer_32bit = NULL;
    card->record_interrupt_enabled = FALSE;
    
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
  struct CardData* card = CardBase->driverdatas[0];
  int i;

  switch( attribute )
  {
    case AHIDB_Bits:
      return 16;

    case AHIDB_Frequencies:
      return FREQUENCIES;

    case AHIDB_Frequency: // Index->Frequency
      return (LONG) Frequencies[ argument ];

    case AHIDB_Index: // Frequency->Index
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

      return 0;  // Will not happen

    case AHIDB_Author:
      return (LONG) "Davy Wentzler";

    case AHIDB_Copyright:
      return (LONG) "(C) Davy Wentzler";

    case AHIDB_Version:
      return (LONG) LibIDString;

    case AHIDB_Annotation:
      return (LONG)
   	"VIA Envy24 driver";

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
      return 0x00000;

    case AHIDB_MaxMonitorVolume:
      return 0x00000;

    case AHIDB_MinInputGain:
      return 0x10000; // 0.0 dB gain

    case AHIDB_MaxInputGain:
      if (card->SubType == MAUDIO_1010LT || card->SubType == MAUDIO_DELTA44 || card->SubType == MAUDIO_DELTA66)
          return 0x7F17A;
      else
          return 0x10000; // 0 dB gain

    case AHIDB_MinOutputVolume:
      return 0x00010; // -72 dB / mute

    case AHIDB_MaxOutputVolume:
      return 0x10000; // 0 dB

    case AHIDB_Inputs:
      if (card->SubType == PHASE88 || card->SubType == MAUDIO_1010LT)
        return INPUTS;
      else if ( card->SubType == MAUDIO_2496)
        return INPUTS_2496;
      else if (card->SubType == MAUDIO_DELTA44)
        return INPUTS_DELTA44;
      else if (card->SubType == MAUDIO_DELTA66)
        return INPUTS_DELTA66;

    case AHIDB_Input:
      if (card->SubType == PHASE88 || card->SubType == MAUDIO_1010LT)
      {
        if (argument < 0 || argument > (INPUTS - 1))
            argument = 0;
        return (LONG) Inputs[ argument ];
      }
      else if ( card->SubType == MAUDIO_2496)
      {
        if (argument < 0 || argument > (INPUTS_2496 - 1))
            argument = 0;
        return (LONG) Inputs_2496[ argument ];
      }
      else if (card->SubType == MAUDIO_DELTA44)
      {
        if (argument < 0 || argument > (INPUTS_DELTA44 - 1))
            argument = 0;
        return (LONG) Inputs_Delta44[ argument ];
      }
      else if (card->SubType == MAUDIO_DELTA66)
      {
          if (argument < 0 || argument > (INPUTS_DELTA66 - 1))
            argument = 0;
        return (LONG) Inputs_Delta66[ argument ];
      }


    case AHIDB_Outputs:
      if (card->SubType == PHASE88 || card->SubType == MAUDIO_1010LT)
        return OUTPUTS;
      else if ( card->SubType == MAUDIO_2496)
        return OUTPUTS_2496;
      else if (card->SubType == MAUDIO_DELTA44)
        return OUTPUTS_DELTA44;
      else if (card->SubType == MAUDIO_DELTA66)
        return OUTPUTS_DELTA66;

    case AHIDB_Output:
      if (card->SubType == PHASE88 || card->SubType == MAUDIO_1010LT)
          return (LONG) Outputs[ argument ];
      else if ( card->SubType == MAUDIO_2496)
          return (LONG) Outputs_2496[ argument ];
      else if (card->SubType == MAUDIO_DELTA44)
          return (LONG) Outputs_Delta44[ argument ];
      else if (card->SubType == MAUDIO_DELTA66)
          return (LONG) Outputs_Delta66[ argument ];

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
#if 0
  switch( attribute )
  {
    case AHIC_MonitorVolume:
      card->monitor_volume = argument;
      //DebugPrintF("card->monitor_volume = %lu, %lx\n", card->monitor_volume, card->monitor_volume);
      if( card->is_recording )
      {
         //UpdateMonitorMixer( card );
      }
      return TRUE;

    case AHIC_MonitorVolume_Query:
      return card->monitor_volume;

    case AHIC_InputGain:
    {
      double dB;
      unsigned char val;

      card->input_gain = argument;

      dB = 20.0 * log10((Fixed) argument / 65536.0);
      val = 128 + (int) (dB * 2);
      
      if (card->SubType == MAUDIO_DELTA44 || card->SubType == MAUDIO_DELTA66)
      {
        akm4xxx_write(card, &card->codec[0], 0, 0x04, val);
        akm4xxx_write(card, &card->codec[0], 0, 0x05, val);
      }
      else if (card->SubType == MAUDIO_1010LT)
      {
         int chip;

         for (chip = 0; chip < 4; chip++)
         {
            akm4xxx_write(card, &card->codec[chip], chip, 0x04, val);
            akm4xxx_write(card, &card->codec[chip], chip, 0x05, val);
         }
      }

      return TRUE;
    }

    case AHIC_InputGain_Query:
      return card->input_gain;

    case AHIC_OutputVolume:
    {
      double dB = 20.0 * log10((Fixed) argument / 65536.0);
      unsigned int val = 127 - (unsigned int) (dB * 127.0 / -72.0);

      card->output_volume = argument;
         
      //DebugPrintF("val = %x\n", val);
      
      if (card->SubType == MAUDIO_DELTA44 || card->SubType == MAUDIO_DELTA66)
      {
        akm4xxx_write(card, &card->codec[0], 0, 0x06, val);
        akm4xxx_write(card, &card->codec[0], 0, 0x07, val);
      }
      else if (card->SubType == MAUDIO_2496)
      {
        akm4xxx_write(card, &card->codec[0], 0, 0x06, val);
        akm4xxx_write(card, &card->codec[0], 0, 0x07, val);
      }
      else if (card->SubType == MAUDIO_1010LT)
      {
         int chip;

         for (chip = 0; chip < 4; chip++)
         {
            akm4xxx_write(card, &card->codec[chip], chip, 0x06, val);
            akm4xxx_write(card, &card->codec[chip], chip, 0x07, val);
         }
      }
      
      return TRUE;
    }

    case AHIC_OutputVolume_Query:
      return card->output_volume;

    case AHIC_Input:
      card->input = argument;

      if( card->is_recording )
      {
         //UpdateMonitorMixer( card );
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
#else
  return FALSE;
#endif
}
