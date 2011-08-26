/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License");
you may not use this file except in compliance with the License. You may obtain a copy of the License at 
http://www.aros.org/license.html 
Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF 
ANY KIND, either express or implied. See the License for the specific language governing rights and 
limitations under the License. 

The Original Code is written by Davy Wentzler.
*/

//#include <config.h>

#if !defined(__AROS__)
#undef __USE_INLINE__
#include <proto/expansion.h>
#endif

#include <devices/ahi.h>
#include <exec/memory.h>
#include <libraries/ahi_sub.h>

#include <proto/ahi_sub.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#ifdef __AROS__
#define DEBUG 1
#include <aros/debug.h>
#define DebugPrintF bug
#endif
#include <math.h>
#include <string.h>

#include "library.h"
#include "regs.h"
#include "misc.h"
#include "pci_wrapper.h"

extern int z;

/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

#define FREQUENCIES 8

static const ULONG Frequencies[ FREQUENCIES ] =
{
    5512,
    8000,     // µ- and A-Law
    11025,    // CD/4
    16000,    // DAT/3
    22050,    // CD/2
    32000,    // DAT/1.5
    44100,    // CD
    48000     // DAT
};

static const ULONG FrequencyBits[ FREQUENCIES ] =
{
    0,
    4,
    1,
    5,
    2,
    6,
    3,
    7
};

#define INPUTS 5

static const STRPTR Inputs[ INPUTS ] =
{
    "Line in",
    "Mic",
    "CD",
    "Aux",
    "S/PDIF"
};

#if 0
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
#endif

#define OUTPUTS 2

static const STRPTR Outputs[ OUTPUTS ] =
{
    "Line Out",
    "Digital Out"
};

/******************************************************************************
** AHIsub_AllocAudio **********************************************************
******************************************************************************/

ULONG
_AHIsub_AllocAudio( struct TagItem*         taglist,
		    struct AHIAudioCtrlDrv* AudioCtrl,
		    struct DriverBase*      AHIsubBase )
{
    struct CMI8738Base* CMI8738Base = (struct CMI8738Base*) AHIsubBase;

    int   card_num;
    ULONG ret;
    int   i, freq = 6;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

    card_num = ( GetTagData( AHIDB_AudioID, 0, taglist) & 0x0000f000 ) >> 12;

    if( card_num >= CMI8738Base->cards_found ||
      CMI8738Base->driverdatas[ card_num ] == NULL )
    {
	DebugPrintF("no date for card = %ld\n", card_num);
	Req( "No CMI8738_DATA for card %ld.", card_num );
	return AHISF_ERROR;
    }
    else
    {
	struct CMI8738_DATA* card;
	BOOL in_use;
	struct PCIDevice *dev;

	card  = CMI8738Base->driverdatas[ card_num ];
	AudioCtrl->ahiac_DriverData = card;

	ObtainSemaphore( &CMI8738Base->semaphore );
	in_use = ( card->audioctrl != NULL );
	if( !in_use )
	{
	    card->audioctrl = AudioCtrl;
	}
	ReleaseSemaphore( &CMI8738Base->semaphore );

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
    struct CMI8738Base* CMI8738Base = (struct CMI8738Base*) AHIsubBase;
    struct CMI8738_DATA* card = (struct CMI8738_DATA*) AudioCtrl->ahiac_DriverData;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

    if( card != NULL )
    {
	ObtainSemaphore( &CMI8738Base->semaphore );
	if( card->audioctrl == AudioCtrl )
	{
	    // Release it if we own it.
	    card->audioctrl = NULL;
	}
	ReleaseSemaphore( &CMI8738Base->semaphore );

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
    struct CMI8738Base* CMI8738Base = (struct CMI8738Base*) AHIsubBase;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

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
    struct CMI8738Base* CMI8738Base = (struct CMI8738Base*) AHIsubBase;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

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
    struct CMI8738Base* CMI8738Base = (struct CMI8738Base*) AHIsubBase;
    struct CMI8738_DATA* card = (struct CMI8738_DATA*) AudioCtrl->ahiac_DriverData;
    struct PCIDevice *dev = card->pci_dev;
    UWORD PlayCtrlFlags = 0, RecCtrlFlags = 0;
    ULONG dma_buffer_size = 0;
    int i, freqbit = 6;
    unsigned long phys_addr;
    APTR stack;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

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

    card->mixerstate = cmimix_rd(dev, card, CMPCI_SB16_MIXER_OUTMIX);

    if( flags & AHISF_PLAY )
    {
	ULONG dma_sample_frame_size;
	int i;
	short *a;
	unsigned short cod, ChannelsFlag = CMPCI_REG_FORMAT_16BIT;

	//WriteMask(dev, card, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_RESET | CMPCI_REG_CH1_RESET);
	//ClearMask(dev, card, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_RESET | CMPCI_REG_CH1_RESET);
    
	/* Allocate a new mixing buffer. Note: The buffer must be cleared, since
	   it might not be filled by the mixer software interrupt because of
	   pretimer/posttimer! */

	card->mix_buffer = AllocVec( AudioCtrl->ahiac_BuffSize, MEMF_PUBLIC | MEMF_CLEAR );

	if( card->mix_buffer == NULL )
	{
	    Req( "Unable to allocate %ld bytes for mixing buffer.", AudioCtrl->ahiac_BuffSize );
	    return AHIE_NOMEM;
	}

	/* Allocate a buffer large enough for 16-bit double-buffered
	   playback (mono or stereo) */

	if( AudioCtrl->ahiac_Flags & AHIACF_STEREO )
	{
	    dma_sample_frame_size = 4;
	    dma_buffer_size = AudioCtrl->ahiac_MaxBuffSamples * dma_sample_frame_size;
	    ChannelsFlag |= CMPCI_REG_FORMAT_STEREO;
	}
	else
	{
	    dma_sample_frame_size = 2;
	    dma_buffer_size = AudioCtrl->ahiac_MaxBuffSamples * dma_sample_frame_size;
	}

	//DebugPrintF("dma_buffer_size = %ld, AudioCtrl->ahiac_BuffSize = %ld, AudioCtrl->ahiac_MaxBuffSamples = %ld\nAudioCtrl->ahiac_BuffSamples = %ld", dma_buffer_size, AudioCtrl->ahiac_BuffSize, AudioCtrl->ahiac_MaxBuffSamples, AudioCtrl->ahiac_BuffSamples);
 
	card->playback_buffer = pci_alloc_consistent(dma_buffer_size * 2, &card->playback_buffer_nonaligned, 128);

	if (!card->playback_buffer)
	{
	    Req( "Unable to allocate playback buffer." );
	    return AHIE_NOMEM;
	}

	card->current_bytesize = dma_buffer_size;
	card->current_frames = AudioCtrl->ahiac_MaxBuffSamples;
	card->current_buffer   = card->playback_buffer + card->current_bytesize;
	card->playback_interrupt_enabled = TRUE;
   
	card->flip = 0;
	card->oldflip = 0;

	WritePartialMask(dev, card, CMPCI_REG_FUNC_1, CMPCI_REG_DAC_FS_SHIFT, CMPCI_REG_DAC_FS_MASK, FrequencyBits[freqbit]);
	WritePartialMask(dev, card, CMPCI_REG_CHANNEL_FORMAT, CMPCI_REG_CH0_FORMAT_SHIFT, CMPCI_REG_CH0_FORMAT_MASK, ChannelsFlag);
	WriteMask(dev, card, CMPCI_REG_CHANNEL_FORMAT, (13 << 1));

#if !defined(__AROS__)
	if (IFakeDMA == NULL)
	{
	    stack = SuperState();
	    card->playback_buffer_phys = IMMU->GetPhysicalAddress(card->playback_buffer);
	    UserState(stack);
	}
	else
#endif
	    card->playback_buffer_phys = card->playback_buffer;

	bug("[CMI8738] %s: Playback buffer @ 0x%p\n", __PRETTY_FUNCTION__, card->playback_buffer);
    
	pci_outl(card->playback_buffer_phys, CMPCI_REG_DMA0_BASE, card);
	pci_outw((dma_buffer_size / dma_sample_frame_size) * 2 - 1, CMPCI_REG_DMA0_LENGTH, card);
	pci_outw((dma_buffer_size / dma_sample_frame_size) - 1, CMPCI_REG_DMA0_INTLEN, card);
    
	card->is_playing = TRUE;
    }

    if( flags & AHISF_RECORD )
    {
	UWORD mask;
	ULONG ChannelsFlag = CMPCI_REG_FORMAT_16BIT;
	unsigned char byte;

	card->current_record_bytesize = RECORD_BUFFER_SAMPLES * 4;

	/* Allocate a new recording buffer (page aligned!) */
	card->record_buffer = pci_alloc_consistent(card->current_record_bytesize * 2, &card->record_buffer_nonaligned, 128);

	if( card->record_buffer == NULL )
	{
	    Req( "Unable to allocate %ld bytes for the recording buffer.", card->current_record_bytesize, 128);
	    return AHIE_NOMEM;
	}

	SaveMixerState( card );
	UpdateMonitorMixer( card );

	switch (card->input)
	{
	case 0: // line
	    cmimix_wr(dev, card, CMPCI_SB16_MIXER_OUTMIX, card->mixerstate | CMPCI_SB16_SW_LINE);
	    break;
         
	case 1: // mic
	    cmimix_wr(dev, card, CMPCI_SB16_MIXER_OUTMIX, card->mixerstate | CMPCI_SB16_SW_MIC);
	    break;
     
	case 2: // CD
	    cmimix_wr(dev, card, CMPCI_SB16_MIXER_OUTMIX, card->mixerstate | CMPCI_SB16_SW_CD);
	    break;
         
	case 3: // Aux
	    byte = pci_inb(CMPCI_REG_MIXER25, card);
	    byte |= 0x30;
	    pci_outb(byte, CMPCI_REG_MIXER25, card); // unmute Aux
	    break;
     
	default:
	    break;
	}
    
	card->record_interrupt_enabled = TRUE;

	card->recflip = 0;
    
	WritePartialMask(dev, card, CMPCI_REG_FUNC_1, CMPCI_REG_ADC_FS_SHIFT, CMPCI_REG_ADC_FS_MASK, FrequencyBits[freqbit]);
	WritePartialMask(dev, card, CMPCI_REG_CHANNEL_FORMAT, CMPCI_REG_CH1_FORMAT_SHIFT, CMPCI_REG_CH1_FORMAT_MASK, CMPCI_REG_FORMAT_16BIT | CMPCI_REG_FORMAT_STEREO);
    
#if !defined(__AROS__)
	if (IFakeDMA == NULL)
	{
	    stack = SuperState();
	    card->record_buffer_phys = IMMU->GetPhysicalAddress(card->record_buffer);
	    UserState(stack);
	}
	else
#endif
        card->record_buffer_phys = card->record_buffer;

	pci_outl(card->record_buffer_phys, CMPCI_REG_DMA1_BASE, card);
	udelay(1);
	pci_outw((card->current_record_bytesize / 4) * 2 - 1, CMPCI_REG_DMA1_LENGTH, card);
	udelay(1);
	pci_outw((card->current_record_bytesize / 4) - 1, CMPCI_REG_DMA1_INTLEN, card);
	udelay(1);
	card->current_record_buffer = card->record_buffer + card->current_record_bytesize;
	card->is_recording = TRUE;
    }

    if( flags & AHISF_PLAY )
    {
	z = 0;
	WriteMask(dev, card, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_ENABLE);
	WriteMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH0_INTR_ENABLE);
    }

    if( flags & AHISF_RECORD )
    {
	WriteMask(dev, card, CMPCI_REG_FUNC_0, CMPCI_REG_CH1_ENABLE);
	WriteMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH1_INTR_ENABLE);
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
    struct CMI8738Base* CMI8738Base = (struct CMI8738Base*) AHIsubBase;
    struct CMI8738_DATA* card = (struct CMI8738_DATA*) AudioCtrl->ahiac_DriverData;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

#if 0
    card->current_frames = AudioCtrl->ahiac_BuffSamples;

    if( AudioCtrl->ahiac_Flags & AHIACF_STEREO )
    {
	card->current_bytesize = card->current_frames * 4;
    }
    else
    {
	card->current_bytesize = card->current_frames * 2;
    }
#endif
}


/******************************************************************************
** AHIsub_Stop ****************************************************************
******************************************************************************/

void
_AHIsub_Stop( ULONG                   flags,
	      struct AHIAudioCtrlDrv* AudioCtrl,
	      struct DriverBase*      AHIsubBase )
{
    struct CMI8738Base* CMI8738Base = (struct CMI8738Base*) AHIsubBase;
    struct CMI8738_DATA* card = (struct CMI8738_DATA*) AudioCtrl->ahiac_DriverData;
    struct PCIDevice *dev = card->pci_dev;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

    if( flags & AHISF_PLAY )
    {
	unsigned short play_ctl;

	card->is_playing= FALSE;

	ClearMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH0_INTR_ENABLE);
	ClearMask(dev, card, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_ENABLE);

	if (card->current_bytesize > 0)
	   pci_free_consistent(card->playback_buffer_nonaligned);

	card->current_bytesize     = 0;
	card->current_frames = 0;
	card->current_buffer   = NULL;

	if ( card->mix_buffer)
	   FreeVec( card->mix_buffer );

	card->mix_buffer = NULL;
	card->playback_interrupt_enabled = FALSE;
	card->current_bytesize = 0;
	//DebugPrintF("#IRQ's = %ld\n", z);
    }

    if( flags & AHISF_RECORD && card->is_recording)
    {
	unsigned short rec_ctl, val;
	unsigned char byte;

	ClearMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH1_INTR_ENABLE);
	ClearMask(dev, card, CMPCI_REG_FUNC_0, CMPCI_REG_CH1_ENABLE);

	switch (card->input)
	{
	case 0: // line
	    cmimix_wr(dev, card, CMPCI_SB16_MIXER_OUTMIX, card->mixerstate);
	    break;
	     
	case 1: // mic
	    cmimix_wr(dev, card, CMPCI_SB16_MIXER_OUTMIX, card->mixerstate);
	    break;
	 
	case 2: // CD
	    cmimix_wr(dev, card, CMPCI_SB16_MIXER_OUTMIX, card->mixerstate);
	    break;
	     
	case 3: // Aux
	    byte = pci_inb(CMPCI_REG_MIXER25, card);
	    pci_outb(byte & ~0x30, CMPCI_REG_MIXER25, card); // mute Aux
	    break;
	 
	default:
	    break;
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
    struct CMI8738Base* CMI8738Base = (struct CMI8738Base*) AHIsubBase;
    int i;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

    struct CMI8738_DATA* card = (struct CMI8738_DATA*) AudioCtrl->ahiac_DriverData;
    if (card == NULL)
    {
	int card_num = ( GetTagData( AHIDB_AudioID, 0, taglist) & 0x0000f000 ) >> 12;
	  
	if( card_num <= CMI8738Base->cards_found ||
	      CMI8738Base->driverdatas[ card_num ] != NULL )
	    card = CMI8738Base->driverdatas[ card_num ];
    }
    bug("[CMI8738] %s: card data @ 0x%p\n", __PRETTY_FUNCTION__, card);

    switch( attribute )
    {
    case AHIDB_Bits:
	return 16;

    case AHIDB_MaxChannels:
	if (card)
	    return card->channels;
	return 2;

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
	return (LONG) "(C) 2011 The AROS Dev Team";

    case AHIDB_Version:
	return (LONG) LibIDString;

    case AHIDB_Annotation:
	return (LONG) "AROS CMI8738 Audio driver";

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
	return 0x10000; // 0 dB gain

    case AHIDB_MinOutputVolume:
	return 0x34; // -62 dB

    case AHIDB_MaxOutputVolume:
	return 0x10000; // 0 dB

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
    struct CMI8738Base* CMI8738Base = (struct CMI8738Base*) AHIsubBase;
    struct CMI8738_DATA* card = (struct CMI8738_DATA*) AudioCtrl->ahiac_DriverData;
    struct PCIDevice *dev = card->pci_dev;
    unsigned char byte;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

    switch( attribute )
    {
    case AHIC_MonitorVolume:
	card->monitor_volume = Linear2MixerGain( (Fixed) argument, &card->monitor_volume_bits );
	//DebugPrintF("card->monitor_volume = %lu, %lx\n", card->monitor_volume, card->monitor_volume);
	if( card->is_recording )
	{
	    UpdateMonitorMixer( card );
	}
	return TRUE;

    case AHIC_MonitorVolume_Query:
	return card->monitor_volume;

    case AHIC_InputGain:
	card->input_gain = Linear2RecordGain( (Fixed) argument, &card->input_gain_bits );
	//codec_write(card, AC97_RECORD_GAIN, card->input_gain_bits );
	return TRUE;

    case AHIC_InputGain_Query:
	return card->input_gain;

    case AHIC_OutputVolume:
    {
	double dB = 20.0 * log10((Fixed) argument / 65536.0);
	unsigned int val = 0xFF - ( ((unsigned int)(-dB/2)) << 3);
	cmimix_wr(dev, card, 0x30, val);
	cmimix_wr(dev, card, 0x31, val);
	return TRUE;
    }
    case AHIC_OutputVolume_Query:
	return card->output_volume;

    case AHIC_Input:
	card->input = argument;
      
	switch (card->input)
	{
	case 0: // line
	    cmimix_wr(dev, card, CMPCI_SB16_MIXER_ADCMIX_L, (CMPCI_SB16_MIXER_LINE_SRC_R << 1) );
	    cmimix_wr(dev, card, CMPCI_SB16_MIXER_ADCMIX_R, CMPCI_SB16_MIXER_LINE_SRC_R );
	    break;
	    
	case 1: // mic
	    cmimix_wr(dev, card, CMPCI_SB16_MIXER_ADCMIX_L, CMPCI_SB16_MIXER_MIC_SRC);
	    cmimix_wr(dev, card, CMPCI_SB16_MIXER_ADCMIX_R, CMPCI_SB16_MIXER_MIC_SRC);
	    break;
	
	case 2: // CD
	    cmimix_wr(dev, card, CMPCI_SB16_MIXER_ADCMIX_L, (CMPCI_SB16_MIXER_CD_SRC_R << 1) );
	    cmimix_wr(dev, card, CMPCI_SB16_MIXER_ADCMIX_R, CMPCI_SB16_MIXER_CD_SRC_R );
	    break;
	    
	case 3: // Aux
	    byte = pci_inb(CMPCI_REG_MIXER25, card);
	    pci_outb(byte | 0xC0, CMPCI_REG_MIXER25, card); // rec source Aux
	    break;
	
	case 4: // SPDIF
	    
	    break;
	
	default:
	    break;
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
	    ClearMask(dev, card, CMPCI_REG_FUNC_1, CMPCI_REG_SPDIFOUT_DAC | CMPCI_REG_SPDIF0_ENABLE);
	    ClearMask(dev, card, CMPCI_REG_LEGACY_CTRL, CMPCI_REG_XSPDIF_ENABLE);
	}
	else
	{
	    WriteMask(dev, card, CMPCI_REG_FUNC_1, CMPCI_REG_SPDIFOUT_DAC | CMPCI_REG_SPDIF0_ENABLE);
	    WriteMask(dev, card, CMPCI_REG_LEGACY_CTRL, CMPCI_REG_XSPDIF_ENABLE);
	}
	return TRUE;

    case AHIC_Output_Query:
	return card->output;

    default:
	return FALSE;
    }
}
