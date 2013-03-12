/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#include <config.h>
#include "pci_wrapper.h"

#ifdef __amigaos4__
#undef __USE_INLINE__
#include <proto/expansion.h>
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

#ifdef __amigaos4__
#include "library_card.h"
#elif __MORPHOS__
#include "library_mos.h"
#else
#include "library.h"
#endif
#include "regs.h"
#include "misc.h"
#include "DriverData.h"
#include "Revo51.h"


extern int z;

/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

#define FREQUENCIES 15

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
    96000,
	 176400,
    192000
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
    7,
    12,
    14 // 176.4 KHz: only when CCS_SYSTEM_CONFIG:6 = 1 or (MT_I2S_FORMAT:MT_CLOCK_128x = 1 & CCS_SYSTEM_CONFIG:6 = 0)
};


#define SPDIF_FREQUENCIES 7

static const ULONG SPDIF_Frequencies[ SPDIF_FREQUENCIES ] =
{
  	 32000,
    44100, // CD
    48000,
    88200,
    96000,
	 176400,
    192000
};

static const ULONG SPDIF_FrequencyBits[ SPDIF_FREQUENCIES ] =
{
    3,
    0,
    2,
    4,
    5,
    7,
    6
};



#define INPUTS 12

static const STRPTR Inputs[ INPUTS ] __attribute__((used)) =
{
  "CD in (16-bit)",
  "CD in (24-bit)",
  "Aux in (16-bit)",
  "Aux in (24-bit)",
  "Line in (16-bit)",
  "Line in (24-bit)",
  "Mic in (16-bit)",
  "Mic in (24-bit)",
  "Mix in (16-bit)",
  "Mix in (24-bit)",
  "Digital in (16-bit)",
  "Digital in (24-bit)"
};

#define INPUTS_PHASE28 4 // also for JULIA
static const STRPTR Inputs_Phase28[ INPUTS_PHASE28 ] __attribute__((used)) =
{
  "Line in (16-bit)",
  "Line in (24-bit)",
  "Digital in (16-bit)",
  "Digital in (24-bit)"
};


#define INPUTS_REVO71 2
static const STRPTR Inputs_Revo71[ INPUTS_REVO71 ] __attribute__((used)) =
{
  "Line in (16-bit)",
  "Line in (24-bit)"
};

#define INPUTS_REVO51 3
static const STRPTR Inputs_Revo51[ INPUTS_REVO51 ] __attribute__((used)) =
{
  "Mic in",
  "Line in",
  "CD in"
};


#define OUTPUTS 1

static const STRPTR Outputs[ OUTPUTS ] =
{
  "Line & Digital",
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

  card_num = ( GETTAGDATA( AHIDB_AudioID, 0, taglist) & 0x0000f000 ) >> 12;
  
  if( card_num >= CardBase->cards_found ||
      CardBase->driverdatas[ card_num ] == NULL )
  {
    DEBUGPRINTF("no data for card = %ld\n", card_num);
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

    OBTAINSEMAPHORE( &CardBase->semaphore );
    in_use = ( card->audioctrl != NULL );
    if( !in_use )
    {
      card->audioctrl = AudioCtrl;
    }
    RELEASESEMAPHORE( &CardBase->semaphore );

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
    OBTAINSEMAPHORE( &CardBase->semaphore );
    if( card->audioctrl == AudioCtrl )
    {
      // Release it if we own it.
      card->audioctrl = NULL;
    }
    RELEASESEMAPHORE( &CardBase->semaphore );

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

#ifdef __amigaos4__
  IExec->Disable();
#else
  Disable();
#endif
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

#ifdef __amigaos4__
  IExec->Enable();
#else
  Enable();
#endif
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
  int i, freqbit = 9, SPDIF_freqbit = -1;
  BOOL SPDIF = FALSE;
  unsigned char RMASK = MT_RDMA0_MASK;
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
  
  for( i = 0; i < SPDIF_FREQUENCIES; ++i )
  {
    if( AudioCtrl->ahiac_MixFreq == SPDIF_Frequencies[ i ] )
    {
      SPDIF_freqbit = i;
      SPDIF = TRUE;
      break;
    }
  }

  OUTBYTE(card->mtbase + MT_SAMPLERATE, FrequencyBits[freqbit]);
  OUTBYTE(card->mtbase + MT_DMAI_BURSTSIZE, 2); // set to stereo pair mode


  if( flags & AHISF_PLAY )
  {
    
    ULONG dma_sample_frame_size;
    int i;
    short *a;
    unsigned short cod, ChannelsFlag = 0;
    

    /* Allocate a new mixing buffer. Note: The buffer must be cleared, since
       it might not be filled by the mixer software interrupt because of
       pretimer/posttimer! */

    card->mix_buffer = ALLOCVEC( AudioCtrl->ahiac_BuffSize, MEMF_PUBLIC | MEMF_CLEAR );

    if( card->mix_buffer == NULL )
    {
        Req( "Unable to allocate %ld bytes for mixing buffer.", AudioCtrl->ahiac_BuffSize );
        return AHIE_NOMEM;
    }

    /* Allocate a buffer large enough for 32-bit double-buffered playback (only stereo) */

    dma_sample_frame_size = 16; // we need 4-channel, otherwise the length can't be divided by 8
    dma_buffer_size = AudioCtrl->ahiac_MaxBuffSamples * dma_sample_frame_size;

    //DEBUGPRINTF("dma_buffer_size = %lu\n", dma_buffer_size);

    /*if (dma_buffer_size != card->current_bytesize)
    {
        if (card->playback_buffer)
        {
            DEBUGPRINTF("Freeing prev buffer\n");
            pci_free_consistent(card->playback_buffer_nonaligned);
        }*/

    card->playback_buffer = pci_alloc_consistent(dma_buffer_size * 2, &card->playback_buffer_nonaligned, 4096);
    card->spdif_out_buffer = pci_alloc_consistent(dma_buffer_size, &card->spdif_out_buffer_nonaligned, 4096); // only stereo

    if (!card->playback_buffer || !card->spdif_out_buffer)
    {
        Req( "Unable to allocate playback buffer." );
        return AHIE_NOMEM;
    }

    ClearMask8(card, card->mtbase, MT_DMA_CONTROL, MT_PDMA0_START | MT_PDMA4_START); // stop
    ClearMask8(card, card->mtbase, MT_INTR_MASK, MT_PDMA0_MASK); // allow PDMA0 interrupts

    card->current_bytesize = dma_buffer_size;
    card->current_frames = AudioCtrl->ahiac_MaxBuffSamples;
    card->current_buffer   = card->playback_buffer + card->current_bytesize;
    card->spdif_out_current_buffer = card->spdif_out_buffer + card->current_bytesize / 2;
    card->playback_interrupt_enabled = TRUE;
    
#ifdef __amigaos4__
    stack = IExec->SuperState();
    card->playback_buffer_phys = IMMU->GetPhysicalAddress(card->playback_buffer);
    card->spdif_out_buffer_phys = IMMU->GetPhysicalAddress(card->spdif_out_buffer);
    IExec->UserState(stack);
#elif __MORPHOS__
    card->playback_buffer_phys = ahi_pci_logic_to_physic_addr(card->playback_buffer, dev);
    card->spdif_out_buffer_phys = ahi_pci_logic_to_physic_addr(card->spdif_out_buffer, dev);
#else
    card->playback_buffer_phys = card->playback_buffer;
    card->spdif_out_buffer_phys = card->spdif_out_buffer; // if SPDIF were false, this pointer wouldn't get initialized, which might mean trouble in the IRQ
#endif

    OUTLONG(card->mtbase + MT_DMAI_PB_ADDRESS, card->playback_buffer_phys);
    
    //kprintf("card->playback_buffer_phys = %lx, virt = %lx\n", card->playback_buffer_phys, card->playback_buffer);
    //DEBUGPRINTF("addr = %lx, %lx\n", card->playback_buffer_phys, INLONG(card->mtbase + MT_DMAI_PB_ADDRESS));

    //DEBUGPRINTF("dmai length = %lu\n", (dma_buffer_size * 2) / 4 - 1);
    OUTWORD(card->mtbase + MT_DMAI_PB_LENGTH, (dma_buffer_size * 2) / 4 - 1);
    OUTBYTE(card->mtbase + MT_DMAI_PB_LENGTH + 2, 0);
    OUTWORD(card->mtbase + MT_DMAI_INTLEN, (dma_buffer_size * 1) / 4 - 1);

    card->flip = 0;
    
    // enable S/PDIF out
    if (SPDIF)
    {
       ClearMask8(card, card->iobase, CCS_SPDIF_CONFIG, CCS_SPDIF_INTEGRATED); // disable: see doc: 4-27
    
       OUTWORD(card->mtbase + MT_SPDIF_TRANSMIT, 0x04 | 1 << 5 | (FrequencyBits[freqbit] << 12)); // dig/dig converter
    
       WriteMask8(card, card->iobase, CCS_SPDIF_CONFIG, CCS_SPDIF_INTEGRATED); // enable
    
       OUTLONG(card->mtbase + MT_PDMA4_ADDRESS, card->spdif_out_buffer_phys);
       OUTWORD(card->mtbase + MT_PDMA4_LENGTH, (dma_buffer_size ) / 4 - 1);
       OUTWORD(card->mtbase + MT_PDMA4_INTLEN, (dma_buffer_size / 2) / 4 - 1);
       
    }

    WriteMask8(card, card->mtbase, MT_INTR_STATUS, MT_DMA_FIFO | MT_PDMA0 | MT_PDMA4); // clear possibly pending interrupts
  }

  if( flags & AHISF_RECORD )
  {
    if ((card->SubType == PHASE28 && card->input >= 2) ||
        (card->SubType == JULIA && card->input >= 2) ||
        (card->SubType == PHASE22 && card->input >= 2) ||
        (card->SubType != AUREON_SKY && card->input >= 10)) { // digital in
      //unsigned long GPIO;
      
      RMASK = MT_RDMA1_MASK;
      //GPIO = GetGPIOData(card, card->iobase);
      //DEBUGPRINTF("%d %d %d\n", (GPIO & PHASE28_FREQ2) >> 16, (GPIO & PHASE28_FREQ1) >> 21, (GPIO & PHASE28_FREQ0) >> 22);
      
      WriteMask8(card, card->mtbase, MT_SAMPLERATE, MT_SPDIF_MASTER);   // tbd!
      }
       
    card->current_record_bytesize_32bit = RECORD_BUFFER_SAMPLES * 4 * 2; // 32-bit stereo

    //DEBUGPRINTF("REC: current_record_bytesize = %ld\n", card->current_record_bytesize_32bit);
    
    /* Allocate a new recording buffer (page aligned!) */
    card->record_buffer = pci_alloc_consistent(card->current_record_bytesize_32bit, &card->record_buffer_nonaligned, 4096);
    card->record_buffer_32bit = pci_alloc_consistent(card->current_record_bytesize_32bit * 2, &card->record_buffer_nonaligned_32bit, 4096);

    if( card->record_buffer == NULL || card->record_buffer_32bit == NULL)
    {
      Req( "Unable to allocate %ld bytes for the recording buffer.", card->current_record_bytesize_32bit);
      return AHIE_NOMEM;
    }

    SaveMixerState( card );
    UpdateMonitorMixer( card );
    
    
    ClearMask8(card, card->mtbase, MT_DMA_CONTROL, RMASK); // stop
    ClearMask8(card, card->mtbase, MT_INTR_MASK, RMASK);
    
    card->current_record_buffer = card->record_buffer + card->current_record_bytesize_32bit / 2; 
    card->current_record_buffer_32bit = card->record_buffer_32bit + card->current_record_bytesize_32bit;
    
#ifdef __amigaos4__
    stack = IExec->SuperState();
    card->record_buffer_32bit_phys = IMMU->GetPhysicalAddress(card->record_buffer_32bit);
    IExec->UserState(stack);
#elif __MORPHOS__
    card->record_buffer_32bit_phys = ahi_pci_logic_to_physic_addr(card->record_buffer_32bit, dev);
#else
    card->record_buffer_32bit_phys = card->record_buffer_32bit;
#endif

    if (RMASK == MT_RDMA0_MASK)
    {
        OUTLONG(card->mtbase + MT_RDMA0_ADDRESS, (long) card->record_buffer_32bit_phys);
        OUTWORD(card->mtbase + MT_RDMA0_LENGTH, (card->current_record_bytesize_32bit * 2) / 4 - 1);
        OUTWORD(card->mtbase + MT_RDMA0_INTLEN, (card->current_record_bytesize_32bit * 1) / 4 - 1);
    }
    
    else
    {
        OUTLONG(card->mtbase + MT_RDMA1_ADDRESS, (long) card->record_buffer_32bit_phys);
        OUTWORD(card->mtbase + MT_RDMA1_LENGTH, (card->current_record_bytesize_32bit * 2) / 4 - 1);
        OUTWORD(card->mtbase + MT_RDMA1_INTLEN, (card->current_record_bytesize_32bit * 1) / 4 - 1);
    }

    card->record_interrupt_enabled = TRUE;
    card->recflip = 0;
    
    if (card->SubType == AUREON_SKY || card->SubType == AUREON_SPACE || card->SubType == PHASE28)
       wm_put(card, card->iobase, 0x1C, 0x0D); // turn on zero-latency monitoring and PCM, disable AC97 mix

    
    WriteMask8(card, card->mtbase, MT_INTR_STATUS, RMASK);
  }

   if( flags & AHISF_PLAY )
   {
      unsigned char start = MT_PDMA0_START;
      int i;
      
      if (SPDIF)
      {
         start |= MT_PDMA4_START;
      }
      
      card->is_playing = TRUE;
      
      //DEBUGPRINTF("START\n");

      //DEBUGPRINTF("Config 4-5 = %x\n", READCONFIGWORD( PCI_COMMAND ));
//		DEBUGPRINTF("BAR0 = %lx\n", dev->ReadConfigLong(0x10));
//		DEBUGPRINTF("BAR1 = %lx\n", dev->ReadConfigLong(0x14));

      /*for (i = 0; i <= 0x1F; i++)
      {
         DEBUGPRINTF("CCS %02d (%02x) = %x\n", i, i, INBYTE(card->iobase + i));
      }

      for (i = 0; i <= 0x76; i+=4)
      {
         DEBUGPRINTF("MT %02d (%02x) = %lx\n", i, i, INLONG(card->mtbase + i));
      }*/

      WriteMask8(card, card->mtbase, MT_DMA_CONTROL, start);
   }

   if( flags & AHISF_RECORD )
   {
      card->is_recording = TRUE;
      WriteMask8(card, card->mtbase, MT_DMA_CONTROL, RMASK);
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

  /*card->current_frames = AudioCtrl->ahiac_BuffSamples;

  if( AudioCtrl->ahiac_Flags & AHIACF_STEREO )
  {
    card->current_bytesize = card->current_frames * 4;
  }
  else
  {
    card->current_bytesize = card->current_frames * 2;
  }*/
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
  unsigned char RMASK = MT_RDMA0_MASK;

  if( flags & AHISF_PLAY && card->is_playing)
  {
    unsigned short play_ctl, j;
    card->is_playing= FALSE;
    
    ClearMask8(card, card->mtbase, MT_DMA_CONTROL, MT_PDMA0_START | MT_PDMA4_START);
    WriteMask8(card, card->mtbase, MT_INTR_MASK, MT_DMA_FIFO_MASK | MT_PDMA0_MASK);
    
    if (card->current_bytesize > 0) {
       pci_free_consistent(card->playback_buffer_nonaligned);
       pci_free_consistent(card->spdif_out_buffer_nonaligned);
       }

    card->current_bytesize = 0;
    card->current_frames = 0;
    card->current_buffer   = NULL;

    if (card->mix_buffer) {
       FREEVEC( card->mix_buffer );
       }
    card->mix_buffer = NULL;
    card->playback_interrupt_enabled = FALSE;
  }
  
  if( flags & AHISF_RECORD && card->is_recording)
  {
    unsigned short rec_ctl, val;

    card->is_recording = FALSE;
    if ((card->SubType == PHASE28 && card->input >= 2) ||
        (card->SubType == JULIA && card->input >= 2) ||
        (card->SubType != PHASE28 && card->input >= 10))
        RMASK = MT_RDMA1_MASK;
    
    ClearMask8(card, card->mtbase, MT_DMA_CONTROL, RMASK);
    WriteMask8(card, card->mtbase, MT_INTR_MASK, RMASK);
    
    if (card->SubType == AUREON_SKY || card->SubType == AUREON_SPACE || card->SubType == PHASE28)
    {
       wm_put(card, card->iobase, 0x1C, 0x0B); // turn off zero-latency monitoring: enable PCM and AC97 mix
    }

    DELAY(1);

    if( card->record_buffer != NULL )
    {
      pci_free_consistent( card->record_buffer_nonaligned );
      pci_free_consistent( card->record_buffer_nonaligned_32bit );
    }

    card->record_buffer = NULL;
    card->current_record_bytesize_32bit = 0;
    card->record_buffer_32bit = NULL;
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
  struct CardData* card = CardBase->driverdatas[0]; // warning, don't take from AudioCtrl, since it is not always
                                                    // initialized before a call to GetAttr()!!!
  int i;

  switch( attribute )
  {
    case AHIDB_Bits:
      return 16;

    case AHIDB_Frequencies:
      if (card->SubType == PHASE22)
        return FREQUENCIES - 2;
      else
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
   	"Terratec Aureon, Phase22/28, M-Audio Revolution 5.1/7.1 and ESI Juli@ driver";

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
      if (card->SubType == REVO51)
         return 7; // -79dB
      else
         return 0x00000;

    case AHIDB_MaxMonitorVolume:
      if (card->SubType == REVO51)
         return 0x10000;
      else
         return 0x00000;

    case AHIDB_MinInputGain:
      if (card->SubType == REVO71 || card->SubType == JULIA || card->SubType == PHASE22)
         return 0x10000;
      else if (card->SubType == REVO51)
         return 0x10000;
      else
         return 0x404D; // -12 dB gain

    case AHIDB_MaxInputGain:
      if (card->SubType == REVO71 || card->SubType == JULIA)
         return 0x10000;
      else if (card->SubType == REVO51)
         return 0x3FB27; // 12 dB
      else if (card->SubType == PHASE22)
         return 0x7F17A;
      else
         return 0x8E99A; // 19 dB gain

    case AHIDB_MinOutputVolume:
      if (card->SubType == PHASE22)
        return 0x10000;
      else
        return 0x00000; // -100 dB / mute

    case AHIDB_MaxOutputVolume:
      return 0x10000; // 0 dB

    case AHIDB_Inputs:
      if (card->SubType == PHASE28 || card->SubType == JULIA || card->SubType == PHASE22)
        return INPUTS_PHASE28;
      else if (card->SubType == REVO71)
        return INPUTS_REVO71;
      else if (card->SubType == REVO51)
        return INPUTS_REVO51;
      else
        return INPUTS;

    case AHIDB_Input:
      if (card->SubType == PHASE28 || card->SubType == JULIA || card->SubType == PHASE22)
        return (LONG) Inputs_Phase28[ argument ];
      else if (card->SubType == REVO71)
        return (LONG) Inputs_Revo71[ argument ];
      else if (card->SubType == REVO51)
        return (LONG) Inputs_Revo51[ argument ];
      else
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
      card->monitor_volume = 0;

      if (card->SubType == REVO51)
      {
         double dB = 20.0 * log10((Fixed) argument / 65536.0);
         int val = (int) dB;
         int offset = card->input * 4; // left/right (10dB/1dB)
         unsigned char bytes[4];

         int dB10 = -val / 10;
         int dB1 = -val % 10;
         bytes[0] = pt2258_db_codes[offset + 0] | dB10;
         bytes[1] = pt2258_db_codes[offset + 1] | dB1;
         bytes[2] = pt2258_db_codes[offset + 2] | dB10;
         bytes[3] = pt2258_db_codes[offset + 3] | dB1;
         //DEBUGPRINTF("MON offset = %d, %x %x\n", offset, bytes[0], bytes[1]);
         WriteBytesI2C(card, card->i2c, bytes, 4);
      }
      return TRUE;

    case AHIC_MonitorVolume_Query:
      return card->monitor_volume;

    case AHIC_InputGain:
      card->input_gain = Linear2RecordGain( (Fixed) argument, &card->input_gain_bits );
      //DEBUGPRINTF("gain = %lx, bits = %x, argument = %lx\n", card->input_gain, card->input_gain_bits, argument);
      
      if (card->SubType != REVO71 && card->SubType != REVO51 && card->SubType != JULIA && card->SubType != PHASE22)
         wm_put(card, card->iobase, 0x19, card->input_gain_bits | 0x40); // | LRBOTH
      
      else if (card->SubType == PHASE22)
      {
        double dB = 20.0 * log10((Fixed) argument / 65536.0);
        unsigned char val = 128 + (int) (dB * 2);
        
        akm4xxx_write(card, card->RevoFrontCodec, 0, 0x04, val);
        akm4xxx_write(card, card->RevoFrontCodec, 0, 0x05, val);
      }
      else if (card->SubType == REVO51)
      {
         double dB = 20.0 * log10((Fixed) argument / 65536.0);
         unsigned char val = 0x80 + (int) (dB * 2);

         akm4xxx_write_new(card, card->RevoRecCodec, 0, 0x04, val);
         akm4xxx_write_new(card, card->RevoRecCodec, 0, 0x05, val);
      }
      return TRUE;

    case AHIC_InputGain_Query:
      return card->input_gain;

    case AHIC_OutputVolume:
      if (card->SubType != REVO71 && card->SubType != REVO51 && card->SubType != JULIA && card->SubType != PHASE22) {
         card->output_volume = Linear2MixerGain( (Fixed) argument, &card->output_volume_bits );
         
         //DEBUGPRINTF("AHI: output %x\n", card->output_volume_bits);
         
         wm_put(card, card->iobase, 9, card->output_volume_bits | 0x100); // | update on all channels
         wm_put(card, card->iobase, 10, card->output_volume_bits | 0x100);
         }
      else if (card->SubType == REVO71 || card->SubType == REVO51) {
         double dB = 20.0 * log10((Fixed) argument / 65536.0);
         unsigned int val = (unsigned int) (255.0 * pow(10.0, dB / 20.0));

         card->output_volume = Linear2AKMGain( (Fixed) argument, &card->output_volume_bits );

         if (card->SubType == REVO71)
         {
             akm4xxx_write(card, card->RevoFrontCodec, 0, 0x03, val);
             akm4xxx_write(card, card->RevoFrontCodec, 0, 0x04, val);

             akm4xxx_write(card, card->RevoSurroundCodec, 0, 0x04, card->output_volume_bits);
             akm4xxx_write(card, card->RevoSurroundCodec, 0, 0x05, card->output_volume_bits);
         }
         else // REVO51
         {
             double dB;
             unsigned int val;

             if (argument == 0) // prevent overflow
                 argument = 1;
             dB = -20.0 * log10((Fixed) argument / 65536.0);

             if (dB > 63.0)
                val = 0x80;
             else
                val = 0x80 | (0x7F - (unsigned int)(dB * 2));

             //DEBUGPRINTF("arg = %ld, val = %lu, dB = %d\n", argument, val, (int) dB);

             akm4xxx_write_new(card, card->RevoFrontCodec, 0, 0x04, val);
             akm4xxx_write_new(card, card->RevoFrontCodec, 0, 0x05, val);
         }
         }
      
      else if (card->SubType == JULIA) {
         double dB = -20.0 * log10((Fixed) argument / 65536.0);
         unsigned int val = 0x80 | (0x7F - (unsigned int)(dB * 2));
         
         //DEBUGPRINTF("val = %x, dB = %ld.%ld\n", val, (int)(dB), (int) ((dB - (int)(dB)) * 1000)  );
         
         WriteI2C(card->pci_dev, card, AK4358_ADDR, 0x08, val); // dig out is conn. to the DAC as analogue out
         WriteI2C(card->pci_dev, card, AK4358_ADDR, 0x09, val);
         
         /*WriteI2C(card->pci_dev, card, AK4358_ADDR, 0x06, val);
         WriteI2C(card->pci_dev, card, AK4358_ADDR, 0x07, val);
         
         WriteI2C(card->pci_dev, card, AK4358_ADDR, 0x08, val);
         WriteI2C(card->pci_dev, card, AK4358_ADDR, 0x09, val);
         
         WriteI2C(card->pci_dev, card, AK4358_ADDR, 0x0B, val);
         WriteI2C(card->pci_dev, card, AK4358_ADDR, 0x0C, val);*/
         }
      else if (card->SubType == PHASE22) {
         }

      //DEBUGPRINTF("card->output_volume = %lx, bits = %lx\n", card->output_volume, card->output_volume_bits);
      return TRUE;

    case AHIC_OutputVolume_Query:
      return card->output_volume;

    case AHIC_Input:
      card->input = argument;
      
      if (card->SubType == REVO51)
         card->input_is_24bits = FALSE;
      else
      {
        if (card->input % 2 == 0)
           card->input_is_24bits = FALSE;
        else
           card->input_is_24bits = TRUE;
      }
      
      if (card->SubType != PHASE28 && card->input < 10 && card->SubType != REVO71 && card->SubType != REVO51 && card->SubType != JULIA && card->SubType != PHASE22) // analogue
         wm_put(card, card->iobase, 0x1B, (card->input / 2) + 0x10 * (card->input / 2) );
      else if (card->SubType == REVO51)
      {
         akm4xxx_write_new(card, card->RevoRecCodec, 0, 0x01, card->input);
      }

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
