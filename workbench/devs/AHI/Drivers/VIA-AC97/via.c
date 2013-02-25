/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
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

#include <string.h>

#include "library.h"
#include "regs.h"
#include "misc.h"
#include "pci_wrapper.h"

#ifdef __AROS__
#include <aros/debug.h>
#define DebugPrintF bug
#endif

/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

#define FREQUENCIES 11

static const ULONG Frequencies[ FREQUENCIES ] =
{
  5500,
  8000,     // µ- and A-Law
  9600,
  11025,    // CD/4
  16000,    // DAT/3
  19200,
  22050,    // CD/2
  32000,    // DAT/1.5
  38400,
  44100,    // CD
  48000     // DAT
};

#define INPUTS 8

static const STRPTR Inputs[ INPUTS ] =
{
  "Line in",
  "Mic",
  "CD",
  "Video",
  "Aux",
  "Mixer",
  "Mixer (mono)",
  "Phone"
};

/* Not static since it's used in misc.c too */
const UWORD InputBits[ INPUTS ] =
{  
  AC97_RECMUX_LINE,
  AC97_RECMUX_MIC,
  AC97_RECMUX_CD,
  AC97_RECMUX_VIDEO,
  AC97_RECMUX_AUX,
  AC97_RECMUX_STEREO_MIX,
  AC97_RECMUX_MONO_MIX,
  AC97_RECMUX_PHONE
};


#define OUTPUTS 1


BOOL ac97_wait_idle2(struct CardData *card);

static const STRPTR Outputs[ OUTPUTS ] =
{
  "Line",
};

inline unsigned int codec_xread(struct CardData *card)
{
    return pci_inl(VIA_REG_AC97, card);
}


inline void codec_xwrite(struct CardData *card, unsigned int val)
{
    pci_outl(val, VIA_REG_AC97, card);
}

int codec_ready(struct CardData *card)
{
        unsigned int timeout = 100;    /* 1ms */
        unsigned int val;
        
        while ( --timeout )
        {
                val = codec_xread(card);
                if (! (val & VIA_REG_AC97_BUSY))
                    return val & 0xffff;
				udelay( 1000 );
        }
        
        snd_printk("(codec_ready()) AC97 codec not ready!\n");
        return -1;
}

int codec_valid(struct CardData *card)
{
        unsigned int timeout = 1000;    /* 1ms */
        unsigned int val, val1;
        unsigned int stat = VIA_REG_AC97_PRIMARY_VALID;
        
        while (timeout-- > 0) {
                val = codec_xread(card);
                val1 = val & (VIA_REG_AC97_BUSY | stat);
                if (val1 == stat)
                        return val & 0xffff;
                udelay(1);
        }
        return -1;
}
 
void codec_wait(struct CardData *card)
{
        int err;
        err = codec_ready(card);
        udelay(500);
}

void codec_write(struct CardData *card,
                                    unsigned short reg,
                                    unsigned short val)
{
        unsigned int xval;
        
        xval = VIA_REG_AC97_CODEC_ID_PRIMARY;
        xval <<= VIA_REG_AC97_CODEC_ID_SHIFT;
        xval |= reg << VIA_REG_AC97_CMD_SHIFT;
        xval |= val << VIA_REG_AC97_DATA_SHIFT;
        codec_ready(card);
        codec_xwrite(card, xval);
        codec_ready(card);
}


BOOL ac97_wait_idle(struct CardData *card)
{
	unsigned long   tul = 0;
	int			    cnt = 10000;
	unsigned long	iobase = 0;
    struct PCIDevice *dev = card->pci_dev;

	while( --cnt )
	{
		if( !( ( tul = pci_inl(VIA_REG_AC97, card )) & VIA_REG_AC97_BUSY ) )
            return TRUE;
		MicroDelay(100);
	}

	DebugPrintF("Timed out waiting for AC97 controller!\n");
	return FALSE;
}

unsigned short codec_read(struct CardData *card, unsigned char reg)
{
        unsigned long xval;
        unsigned short data;
        int again = 0;

        xval = 0;
        xval |= VIA_REG_AC97_PRIMARY_VALID;
        xval |= VIA_REG_AC97_READ;
        xval |= (reg & 0x7f) << VIA_REG_AC97_CMD_SHIFT;
        
        ac97_wait_idle2(card);
        codec_xwrite(card, xval);
        
        ac97_wait_idle2(card);
        xval = codec_xread(card);
        
        data = (xval & 0xFFFF);
        
        if ( !( xval & VIA_REG_AC97_PRIMARY_VALID) )
        {
            DebugPrintF("Codec read failed!\n");
        }
        
        return data;
}


//IO base 0 registers
static const unsigned char VIA_AC97_RX						= 0x80;			//ac97 register
static const unsigned long		VIA_AC97_RX_PRIMARY_ID		= 0x00;			//primamry codec ID (RW)
static const unsigned long		VIA_AC97_RX_SECODARY_ID		= 0x40000000;	//secondary codec ID (RW)
static const unsigned long		VIA_AC97_RX_SECODARY_VALID	= 0x08000000;	//secondary valid data/status/index (RWC)
static const unsigned long		VIA_AC97_RX_PRIMARY_VALID	= 0x02000000;	//primary valid data etc. (RWC)
static const unsigned long      VIA_AC97_RX_BUSY			= 0x01000000;	//controller busy (R)
static const unsigned long		VIA_AC97_RX_READ			= 0x00800000;	//read/write select (RW)

static const unsigned char		VIA_AC97_RX_SHIFT			= 0x10;			//register shift
static const unsigned long		VIA_AC97_RX_DATA_MASK		= 0xffff;		//data mask

BOOL ac97_wait_idle2(struct CardData *card)
{
    struct PCIDevice *dev = card->pci_dev;
	unsigned long   tul = 0;
	int			    cnt = 26;   //..about half a second, for no good reason
	unsigned long	iobase = card->iobase;

	while( --cnt )
	{
		if( !( ( tul = pci_inl(VIA_AC97_RX, card) ) & VIA_AC97_RX_BUSY ) )
            return TRUE;
		MicroDelay(1000);
	}

	DebugPrintF("Timed out waiting for AC97 controller! VIA_AC97_RX = %lx\n", tul);
	return FALSE;
}


//note: this only reads the primary codec
BOOL ac97_read_reg(struct CardData *card, unsigned char reg, unsigned short *data )
{
    struct PCIDevice *dev = card->pci_dev;
	unsigned long iobase = card->iobase;
	//set up with required codec register, read mode, and clear the primary codec valid flag
	unsigned long tul = ( ( reg << VIA_AC97_RX_SHIFT ) | VIA_AC97_RX_READ | VIA_AC97_RX_PRIMARY_VALID );

	//wait for the controller to become free, and write the command
	ac97_wait_idle2(card);
	pci_outl(tul, VIA_AC97_RX, card);

	//wait for the controller to become free, and read the result
	ac97_wait_idle2(card);
	tul = pci_inl(VIA_AC97_RX, card);
	*data = ( tul & VIA_AC97_RX_DATA_MASK );

	//complain if the data/register etc. is invalid...
	if( !( tul & VIA_AC97_RX_PRIMARY_VALID ) )
	{
		DebugPrintF("Info: (ac97_read_reg) Primary codec operation failed!\n");
		return( FALSE );
	}

	return( TRUE );
}






static void set_table_ptr(struct CardData *card, BOOL Play)
{
    struct PCIDevice *dev = card->pci_dev;
    ULONG phys_addr;
#ifdef __amigaos4__
    APTR stack;
#endif
    
    codec_ready(card);
    if (Play)
    {
#ifdef __amigaos4__
        stack = IExec->SuperState();
        phys_addr = IMMU->GetPhysicalAddress(card->play_idx_table);
        IExec->UserState(stack);
#else
        phys_addr = (ULONG)card->play_idx_table;
#endif
        
        pci_outl(phys_addr, 4, card);
    }
    else
    {
#ifdef __amigaos4__
        stack = IExec->SuperState();
        phys_addr = IMMU->GetPhysicalAddress(card->rec_idx_table);
        IExec->UserState(stack);
#else
        phys_addr = (ULONG)card->rec_idx_table;
#endif

        pci_outl(phys_addr, 4 + RECORD, card);
    }
    
    udelay(20);
    codec_ready(card);
}



#ifdef __amigaos4__
#define tolittle(a) ((((ULONG) (a) & 0xFF000000) >> 24) | \
                     (((ULONG) (a) & 0x00FF0000) >> 8)  | \
                     (((ULONG) (a) & 0x0000FF00) << 8)  | \
                     (((ULONG) (a) & 0x000000FF) << 24))
#else
#define tolittle(a) AROS_LONG2LE(a)
#endif

static int build_via_table(struct CardData *card, APTR sgbuf1, APTR sgbuf2, int OneBufferSize, struct snd_via_sg_table **idx, 
                           APTR *idx_nonaligned)
{
    struct PCIDevice *dev = card->pci_dev;
    int i;
    ULONG phys_addr;
#ifdef __amigaos4__
    APTR stack;
#endif
    
    *idx = pci_alloc_consistent(sizeof(**idx) * 4, idx_nonaligned);
    
#ifdef __amigaos4__
    stack = IExec->SuperState();
    phys_addr = IMMU->GetPhysicalAddress(sgbuf1);
    IExec->UserState(stack);
#else
    phys_addr = (ULONG)sgbuf1;
#endif
    
    (*idx)[0].offset = (APTR) tolittle(phys_addr);
    (*idx)[0].size = tolittle( (OneBufferSize) | VIA_TBL_BIT_FLAG);
    
#ifdef __amigaos4__
    stack = IExec->SuperState();
    phys_addr = IMMU->GetPhysicalAddress(sgbuf2);
    IExec->UserState(stack);
#else
    phys_addr = (ULONG)sgbuf2;
#endif
    
    (*idx)[1].offset = (APTR) tolittle(phys_addr);
    (*idx)[1].size = tolittle( (OneBufferSize) | VIA_TBL_BIT_EOL );
    
    CacheClearE(*idx, sizeof(**idx) * 4, CACRF_ClearD);
    //DebugPrintF("===> play_idx_table at %lx, %lx\n", *idx, *idx_nonaligned);
    
    return 0;
}


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

    

  card_num = (GetTagData( AHIDB_AudioID, 0, taglist) & 0x0000f000 ) >> 12;
  
  if( card_num >= CardBase->cards_found ||
      CardBase->driverdatas[ card_num ] == NULL )
  {
    DebugPrintF("no date for card = %ld\n", card_num);
    Req( "No CardData for card %ld.", card_num );
    return AHISF_ERROR;
  }
  else
  {
    BOOL                in_use;
    struct PCIDevice *dev;
    struct CardData *card;
    unsigned short uval;

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
** AHIsub_Start ***************************************************************
******************************************************************************/

ULONG
_AHIsub_Start( ULONG                   flags,
	       struct AHIAudioCtrlDrv* AudioCtrl,
	       struct DriverBase*      AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;
  struct CardData* card = (struct CardData*) AudioCtrl->ahiac_DriverData;
  UWORD PlayCtrlFlags = 0, RecCtrlFlags = 0;
  ULONG dma_buffer_size = 0;
  int i, freqbit = 9;
  unsigned short uval;

  //channel_reset(card);
  codec_write(card, 0x2, 0);
  codec_write(card, 0x08, 0x0F0F);
  codec_write(card, 0x0A, 0x8000);
  codec_write(card, 0x18, 0x0808);

  uval = codec_read(card, 0x2A);
  codec_write(card, 0x2A, uval | 0x1); // enable VRA
  codec_write(card, 0x2C, AudioCtrl->ahiac_MixFreq);
  

  if( flags & AHISF_PLAY )
  {
    
    ULONG dma_sample_frame_size;
    int i;
    short *a;
    unsigned short cod, ChannelsFlag = 0;

    card->mix_buffer = AllocVec( AudioCtrl->ahiac_BuffSize, MEMF_PUBLIC | MEMF_CLEAR );

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
    }
    else
    {
      dma_sample_frame_size = 2;
      dma_buffer_size = AudioCtrl->ahiac_MaxBuffSamples * dma_sample_frame_size;
    }

    //DebugPrintF("dma_buffer_size = %ld, %lx\n", dma_buffer_size, dma_buffer_size);
 
    card->playback_buffer1 = pci_alloc_consistent(dma_buffer_size, &card->playback_buffer1_nonaligned);
    card->playback_buffer2 = pci_alloc_consistent(dma_buffer_size, &card->playback_buffer2_nonaligned);

    if (!card->playback_buffer1)
    {
      Req( "Unable to allocate playback buffer." );
      return AHIE_NOMEM;
    }

    CacheClearE(card->playback_buffer1, dma_buffer_size, CACRF_ClearD);
    CacheClearE(card->playback_buffer2, dma_buffer_size, CACRF_ClearD);

    card->current_bytesize = dma_buffer_size;
    card->current_frames = AudioCtrl->ahiac_MaxBuffSamples;
    card->current_buffer   = card->playback_buffer1;
    card->playback_interrupt_enabled = TRUE;
    
    build_via_table(card, card->playback_buffer1, card->playback_buffer2, dma_buffer_size, &card->play_idx_table, &card->play_idx_table_nonaligned);
    set_table_ptr(card, TRUE);
  
    pci_outb(VIA_REG_TYPE_AUTOSTART | 0x40 |
             VIA_REG_TYPE_16BIT |
             VIA_REG_TYPE_STEREO |
             VIA_REG_TYPE_INT_LSAMPLE |
             VIA_REG_TYPE_INT_EOL |
             VIA_REG_TYPE_INT_FLAG,
             VIA_REG_OFFSET_TYPE, card);
   
    card->flip = 0;

    card->is_playing = TRUE;
  }

  if( flags & AHISF_RECORD )
  {
    UWORD mask;

    card->current_record_bytesize = RECORD_BUFFER_SAMPLES * 4;

    /* Allocate a new recording buffer (page aligned!) */
    card->record_buffer1 = pci_alloc_consistent(card->current_record_bytesize, &card->record_buffer1_nonaligned);
    card->record_buffer2 = pci_alloc_consistent(card->current_record_bytesize, &card->record_buffer2_nonaligned);

    if( card->record_buffer1 == NULL )
    {
      Req( "Unable to allocate %ld bytes for the recording buffer.", card->current_record_bytesize);
      return AHIE_NOMEM;
    }

    SaveMixerState( card );
    UpdateMonitorMixer( card );
    
    card->record_interrupt_enabled = TRUE;

    card->recflip = 0;
    
    build_via_table(card, card->record_buffer1, card->record_buffer2, card->current_record_bytesize, &card->rec_idx_table, &card->rec_idx_table_nonaligned);
    set_table_ptr(card, FALSE);
    
    pci_outb(VIA_REG_TYPE_AUTOSTART | 0x40 |
             VIA_REG_TYPE_16BIT |
             VIA_REG_TYPE_STEREO |
             VIA_REG_TYPE_INT_EOL |
             VIA_REG_TYPE_INT_FLAG,
             VIA_REG_OFFSET_TYPE + RECORD,
             card);
    
    card->is_recording = TRUE;
    card->current_record_buffer = card->record_buffer1;

  }

   if( flags & AHISF_PLAY )
   {
      unsigned char val;
      
      val = VIA_REG_CTRL_START;
      pci_outb(val, VIA_REG_OFFSET_CONTROL, card);
   }

   if( flags & AHISF_RECORD )
   {
      unsigned char val;
      
      val = VIA_REG_CTRL_START;
      pci_outb(val, VIA_REG_OFFSET_CONTROL + RECORD, card);
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

  unsigned char val;
      
  val = VIA_REG_CTRL_TERMINATE;

  if( flags & AHISF_PLAY && card->is_playing)
  {
    pci_outb(val, VIA_REG_OFFSET_CONTROL, card);
    card->is_playing= FALSE;

    if (card->current_bytesize > 0)
    {
       pci_free_consistent(card->playback_buffer1);
       pci_free_consistent(card->playback_buffer2);
    }

    card->current_bytesize     = 0;
    card->current_frames = 0;
    card->current_buffer   = NULL;

    if ( card->mix_buffer)
       FreeVec( card->mix_buffer );
    card->mix_buffer = NULL;
    card->playback_interrupt_enabled = FALSE;
    
    if (card->play_idx_table_nonaligned)
    {
        FreeVec(card->play_idx_table_nonaligned);
    }
    card->play_idx_table = NULL;
  }

  if( flags & AHISF_RECORD && card->is_recording)
  {
    unsigned short rec_ctl;

    pci_outb(val, VIA_REG_OFFSET_CONTROL + RECORD, card);
    if( card->is_recording )
    {
      // Do not restore mixer unless they have been saved
      RestoreMixerState( card );
    }

    if( card->record_buffer1 != NULL )
    {
      pci_free_consistent( card->record_buffer1);
      pci_free_consistent( card->record_buffer2);
    }

    card->record_buffer1 = NULL;
    card->record_buffer2 = NULL;
    card->current_record_bytesize = 0;

    card->is_recording = FALSE;
    card->record_interrupt_enabled = FALSE;
    
    pci_free_consistent(card->rec_idx_table_nonaligned);
    card->rec_idx_table = NULL;

  }
  
  card->current_bytesize = 0;
}


/******************************************************************************
** AHIsub_GetAttr *************************************************************
******************************************************************************/

SIPTR
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

    case AHIDB_Frequency: // Index->Frequency
      return (SIPTR) Frequencies[ argument ];

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
      return (SIPTR) "Davy Wentzler";

    case AHIDB_Copyright:
      return (SIPTR) "(C) Davy Wentzler";

    case AHIDB_Version:
      return (SIPTR) LibIDString;

    case AHIDB_Annotation:
      return (SIPTR)
   	"VIA VT82C686 AC97 driver";

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
      return 0x40000;

    case AHIDB_MinInputGain:
      return 0x10000; // 0.0 dB gain

    case AHIDB_MaxInputGain:
      return 0xD55D0; // 22.5 dB gain

    case AHIDB_MinOutputVolume:
      return 0x00000; // -96 dB

    case AHIDB_MaxOutputVolume:
      return 0x10000; // 0 dB

    case AHIDB_Inputs:
      return INPUTS;

    case AHIDB_Input:
      return (SIPTR) Inputs[ argument ];

    case AHIDB_Outputs:
      return OUTPUTS;

    case AHIDB_Output:
      return (SIPTR) Outputs[ argument ];

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
      codec_write(card, AC97_RECORD_GAIN, card->input_gain_bits );
      return TRUE;

    case AHIC_InputGain_Query:
      return card->input_gain;

    case AHIC_OutputVolume:
      card->output_volume = Linear2MixerGain( (Fixed) argument, &card->output_volume_bits );
      codec_write(card, AC97_PCMOUT_VOL, card->output_volume_bits );
      return TRUE;

    case AHIC_OutputVolume_Query:
      return card->output_volume;

    case AHIC_Input:
      card->input = argument;
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

      return TRUE;

    case AHIC_Output_Query:
      return card->output;

    default:
      return FALSE;
  }
}
