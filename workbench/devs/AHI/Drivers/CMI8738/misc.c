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

#include <exec/memory.h>

#ifdef __AROS__
#define DEBUG 1
#include <aros/debug.h>
#endif

#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

#include "library.h"
#include "regs.h"
#include "interrupt.h"
#include "misc.h"

#include "pci_wrapper.h"

#define CACHELINE_SIZE 32

#ifdef INTGW
#define DebugPrintF bug
INTGW(static, void,   playbackinterrupt, PlaybackInterrupt);
INTGW(static, void,   recordinterrupt,   RecordInterrupt);
INTGW(static, ULONG, cardinterrupt,  CardInterrupt);
#endif

/* Global in Card.c */
extern const UWORD InputBits[];

/* Public functions in main.c */
int card_init(struct CMI8738_DATA *card);
void card_cleanup(struct CMI8738_DATA *card);

#if !defined(__AROS__)
void AddResetHandler(struct CMI8738_DATA *card);
#endif

void micro_delay(unsigned int val)
{
  struct Device*              TimerBase = NULL;
  struct timerequest*         TimerIO = NULL;
  struct MsgPort *            replymp;

    replymp = (struct MsgPort *) CreateMsgPort();
    if (!replymp)
    {
      bug("Could not create the reply port!\n");
      return;
    }
    
    TimerIO = (struct timerequest *) CreateIORequest(replymp, sizeof(struct timerequest));

    if (TimerIO == NULL)
    {
        DebugPrintF("Out of memory.\n");
        return;
    }
    
    if (OpenDevice((CONST_STRPTR) "timer.device", UNIT_MICROHZ, (struct IORequest *) TimerIO, 0) != 0)
    {
        DebugPrintF("Unable to open 'timer.device'.\n");
        return; 
    }
    else
    {
        TimerBase = (struct Device *) TimerIO->tr_node.io_Device;
    }
    
    TimerIO->tr_node.io_Command = TR_ADDREQUEST; /* Add a request.   */
    TimerIO->tr_time.tv_secs = 0;                /* 0 seconds.      */
    TimerIO->tr_time.tv_micro = val;             /* 'val' micro seconds. */
    DoIO((struct IORequest *) TimerIO);
    CloseDevice((struct IORequest *) TimerIO);
    DeleteIORequest((struct IORequest *) TimerIO);
    TimerIO = NULL;
    
    if (replymp)
    {
        DeleteMsgPort(replymp);
    }
}

void WritePartialMask(struct PCIDevice *dev, struct CMI8738_DATA* card, unsigned long reg, unsigned long shift, unsigned long mask, unsigned long val)
{
    ULONG tmp;

    tmp = pci_inl(reg, card);
    tmp &= ~(mask << shift);
    tmp |= val << shift;
    pci_outl(tmp, reg, card);
}


void ClearMask(struct PCIDevice *dev, struct CMI8738_DATA* card, unsigned long reg, unsigned long mask)
{
    ULONG tmp;

    tmp = pci_inl(reg, card);
    tmp &= ~mask;
    pci_outl(tmp, reg, card);
}


void WriteMask(struct PCIDevice *dev, struct CMI8738_DATA* card, unsigned long reg, unsigned long mask)
{
    ULONG tmp;

    tmp = pci_inl(reg, card);
    tmp |= mask;
    pci_outl(tmp, reg, card);
}

void cmimix_wr(struct PCIDevice *dev, struct CMI8738_DATA* card, unsigned char port, unsigned char val)
{
    pci_outb(port, CMPCI_REG_SBADDR, card);
    pci_outb(val, CMPCI_REG_SBDATA, card);
}

unsigned char cmimix_rd(struct PCIDevice *dev, struct CMI8738_DATA* card, unsigned char port)
{
    pci_outb(port, CMPCI_REG_SBADDR, card);
    return (unsigned char) pci_inb(CMPCI_REG_SBDATA, card);
}


/******************************************************************************
** DriverData allocation ******************************************************
******************************************************************************/

// This code used to be in _AHIsub_AllocAudio(), but since we're now
// handling CAMD support too, it needs to be done at driver loading
// time.

struct CMI8738_DATA*
AllocDriverData( struct PCIDevice *dev,
		 struct DriverBase* AHIsubBase )
{
    struct CMI8738Base* CMI8738Base = (struct CMI8738Base*) AHIsubBase;
    struct CMI8738_DATA* card;
    UWORD command_word;
    ULONG  chipvers;
    int i, v;
    unsigned char byte;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

    // FIXME: This should be non-cachable, DMA-able memory
    card = AllocVec( sizeof( *card ), MEMF_PUBLIC | MEMF_CLEAR );

    if( card == NULL )
    {
	Req( "Unable to allocate driver structure." );
	return NULL;
    }

    card->ahisubbase = AHIsubBase;

    card->interrupt.is_Node.ln_Type = IRQTYPE;
    card->interrupt.is_Node.ln_Pri  = 0;
    card->interrupt.is_Node.ln_Name = (STRPTR) LibName;
#ifdef INTGW
    card->interrupt.is_Code         = (VOID_FUNC)cardinterrupt;
#else
    card->interrupt.is_Code         = (void(*)(void))CardInterrupt;
#endif
    card->interrupt.is_Data         = (APTR) card;

    card->playback_interrupt.is_Node.ln_Type = IRQTYPE;
    card->playback_interrupt.is_Node.ln_Pri  = 0;
    card->playback_interrupt.is_Node.ln_Name = (STRPTR) LibName;
#ifdef INTGW
    card->playback_interrupt.is_Code         = (VOID_FUNC)playbackinterrupt;
#else
    card->playback_interrupt.is_Code         = (void(*)(void))PlaybackInterrupt;
#endif
    card->playback_interrupt.is_Data         = (APTR) card;

    card->record_interrupt.is_Node.ln_Type = IRQTYPE;
    card->record_interrupt.is_Node.ln_Pri  = 0;
    card->record_interrupt.is_Node.ln_Name = (STRPTR) LibName;
#ifdef INTGW
    card->record_interrupt.is_Code         = (VOID_FUNC)recordinterrupt;
#else
    card->record_interrupt.is_Code         = (void(*)(void))RecordInterrupt;
#endif
    card->record_interrupt.is_Data         = (APTR) card;

    card->pci_dev = dev;

    command_word = inw_config(PCI_COMMAND,  dev);  
    command_word |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
    outw_config( PCI_COMMAND, command_word, dev);

    card->pci_master_enabled = TRUE;

    /*for (i = 0; i < 6; i++)
    {
	if (dev->GetResourceRange(i))
	    DebugPrintF("BAR[%ld] = %lx\n", i, dev->GetResourceRange(i)->BaseAddress);
    }*/

    card->iobase  =  ahi_pci_get_base_address(0, dev);
    card->length  = ahi_pci_get_base_size(0, dev);
    card->irq     = ahi_pci_get_irq(dev);
    card->chiprev = inb_config(PCI_REVISION_ID, dev);
    card->model   = inw_config(PCI_SUBSYSTEM_ID, dev);

    bug("[CMI8738]: %s: iobase = 0x%p, len = %d\n", __PRETTY_FUNCTION__, card->iobase, card->length);

    chipvers = pci_inl(CMPCI_REG_INTR_CTRL, card) & CMPCI_REG_VERSION_MASK;
    if (chipvers)
    {
	if (chipvers & CMPCI_REG_VERSION_68)
	{
	    card->chipvers = 68;
	    card->channels = 8;
	}
	if (chipvers & CMPCI_REG_VERSION_55)
	{
	    card->chipvers = 55;
	    card->channels = 6;
	}
	if (chipvers & CMPCI_REG_VERSION_39)
	{
	    card->chipvers = 39;
	    if (chipvers & CMPCI_REG_VERSION_39B)
	    {
		card->channels = 6;
	    }
	    else
	    {
		card->channels = 4;
	    }
	}
    }
    else
    {
	chipvers = pci_inl(CMPCI_REG_CHANNEL_FORMAT, card) & CMPCI_REG_VERSION_37;
	if (!chipvers)
	{
	    card->chipvers = 33;
	    card->channels = 2;
	}
	else
	{
	    card->chipvers = 37;
	    card->channels = 2;
	}
    }
  /*DebugPrintF("---> chiprev = %u, model = %x, Vendor = %x\n", dev->ReadConfigByte( PCI_REVISION_ID), dev->ReadConfigWord( PCI_SUBSYSTEM_ID),
                     dev->ReadConfigWord( PCI_SUBSYSTEM_VENDOR_ID));*/

    bug("[CMI8738]: %s: chipvers = %u, chiprev = %u, model = %x, Vendor = %x\n", __PRETTY_FUNCTION__,
     card->chipvers, card->chiprev,
     card->model,
     inw_config( PCI_SUBSYSTEM_VENDOR_ID, dev));

    bug("[CMI8738]: %s: max channels = %d\n", __PRETTY_FUNCTION__, card->channels);
  
    /* Initialize chip */
    if( card_init( card ) < 0 )
    {
	DebugPrintF("Unable to initialize Card subsystem.");
	return NULL;
    }

    //DebugPrintF("INTERRUPT %lu\n", dev->MapInterrupt());
    ahi_pci_add_intserver(&card->interrupt, dev);
    card->interrupt_added = TRUE;

    card->card_initialized = TRUE;
    card->input          = 0;
    card->output         = 0;
    card->monitor_volume = Linear2MixerGain( 0, &card->monitor_volume_bits );
    card->input_gain     = Linear2RecordGain( 0x10000, &card->input_gain_bits );
    card->output_volume  = Linear2MixerGain( 0x10000, &card->output_volume_bits );
    SaveMixerState(card);

    cmimix_wr(dev, card, CMPCI_SB16_MIXER_RESET, 0);
    cmimix_wr(dev, card, CMPCI_SB16_MIXER_ADCMIX_L, 0); //(CMPCI_SB16_MIXER_LINE_SRC_R << 1) ); // set input to line
    cmimix_wr(dev, card, CMPCI_SB16_MIXER_ADCMIX_R, 0); //CMPCI_SB16_MIXER_LINE_SRC_R);

    cmimix_wr(dev, card, CMPCI_SB16_MIXER_OUTMIX, 0); // set output mute off for line and CD

    cmimix_wr(dev, card, CMPCI_SB16_MIXER_VOICE_L, 0xFF); // PCM
    cmimix_wr(dev, card, CMPCI_SB16_MIXER_VOICE_R, 0xFF);

    cmimix_wr(dev, card, CMPCI_SB16_MIXER_CDDA_L, 0x00);
    cmimix_wr(dev, card, CMPCI_SB16_MIXER_CDDA_R, 0x00);

    cmimix_wr(dev, card, CMPCI_SB16_MIXER_LINE_L, 0x00);
    cmimix_wr(dev, card, CMPCI_SB16_MIXER_LINE_R, 0x00);

    byte = pci_inb(CMPCI_REG_MIXER25, card);
    pci_outb(byte & ~0x30, CMPCI_REG_MIXER25, card); // mute Aux
    pci_outb(byte & ~0x01, CMPCI_REG_MIXER25, card); // turn on mic 20dB boost
    pci_outb(0x00, CMPCI_REG_MIXER_AUX, card);

    byte = pci_inb(CMPCI_REG_MIXER24, card);
    pci_outb(byte | CMPCI_REG_FMMUTE, CMPCI_REG_MIXER24, card);

    cmimix_wr(dev, card, CMPCI_SB16_MIXER_MIC, 0x00);

    cmimix_wr(dev, card, CMPCI_SB16_MIXER_MASTER_L, 0xFF);
    cmimix_wr(dev, card, CMPCI_SB16_MIXER_MASTER_R, 0xFF);

    card->mixerstate = cmimix_rd(dev, card, CMPCI_SB16_MIXER_OUTMIX);

#if !defined(__AROS__)
    AddResetHandler(card);
#endif

    return card;
}


/******************************************************************************
** DriverData deallocation ****************************************************
******************************************************************************/

// And this code used to be in _AHIsub_FreeAudio().

void
FreeDriverData( struct CMI8738_DATA* card,
		struct DriverBase*  AHIsubBase )
{

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

  if( card != NULL )
  {
    if( card->pci_dev != NULL )
    {
      if( card->card_initialized )
      {
        card_cleanup( card );
      }

      if( card->pci_master_enabled )
      {
        UWORD cmd;

        cmd = inw_config(PCI_COMMAND, (struct PCIDevice * ) card->pci_dev);
        cmd &= ~( PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER );
        outw_config(PCI_COMMAND, cmd, (struct PCIDevice * ) card->pci_dev );
      }
    }

    if( card->interrupt_added )
    {
      ahi_pci_rem_intserver(&card->interrupt, card->pci_dev);
    }

    FreeVec( card );
  }
}


int card_init(struct CMI8738_DATA *card)
{
    struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;
    unsigned short cod;
    unsigned long val;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

    ClearMask(dev, card, CMPCI_REG_MISC, CMPCI_REG_POWER_DOWN); // power up
    
    WriteMask(dev, card, CMPCI_REG_MISC, CMPCI_REG_BUS_AND_DSP_RESET);
    udelay(1);
    ClearMask(dev, card, CMPCI_REG_MISC, CMPCI_REG_BUS_AND_DSP_RESET);

    /* reset channels */
    WriteMask(dev, card, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_RESET | CMPCI_REG_CH1_RESET);
    udelay(1);
    ClearMask(dev, card, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_RESET | CMPCI_REG_CH1_RESET);

    /* Disable interrupts and channels */
    ClearMask(dev, card, CMPCI_REG_FUNC_0,CMPCI_REG_CH0_ENABLE | CMPCI_REG_CH1_ENABLE);
    ClearMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH0_INTR_ENABLE | CMPCI_REG_CH1_INTR_ENABLE);

    /* Configure DMA channels, ch0 = play, ch1 = capture */
    ClearMask(dev, card, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_DIR);
    WriteMask(dev, card, CMPCI_REG_FUNC_0, CMPCI_REG_CH1_DIR);
    
    // digital I/O
    ClearMask(dev, card, CMPCI_REG_FUNC_1, CMPCI_REG_SPDIFOUT_DAC | CMPCI_REG_SPDIF0_ENABLE);
    ClearMask(dev, card, CMPCI_REG_LEGACY_CTRL, CMPCI_REG_LEGACY_SPDIF_ENABLE);

    ClearMask(dev, card, CMPCI_REG_LEGACY_CTRL, 0x80000000);
    ClearMask(dev, card, CMPCI_REG_CHANNEL_FORMAT, CM_CHB3D);
	ClearMask(dev, card, CMPCI_REG_CHANNEL_FORMAT, CM_CHB3D5C);
	ClearMask(dev, card, CMPCI_REG_LEGACY_CTRL, CMPCI_REG_ENABLE_5_1);
	ClearMask(dev, card, CMPCI_REG_MISC, CMPCI_REG_2ND_SPDIFIN);
	ClearMask(dev, card, CMPCI_REG_MISC, CMPCI_REG_N4SPK3D);
    WriteMask(dev, card, CMPCI_REG_INTR_STATUS, CMPCI_REG_LEGACY_STEREO | CMPCI_REG_LEGACY_HDMA);

    return 0;
}


void card_cleanup(struct CMI8738_DATA *card)
{
}



/******************************************************************************
** Misc. **********************************************************************
******************************************************************************/

void
SaveMixerState( struct CMI8738_DATA* card )
{
    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

#if 0
  card->ac97_mic    = codec_read( card, AC97_MIC_VOL );
  card->ac97_cd     = codec_read( card, AC97_CD_VOL );
  card->ac97_video  = codec_read( card, AC97_VIDEO_VOL );
  card->ac97_aux    = codec_read( card, AC97_AUX_VOL );
  card->ac97_linein = codec_read( card, AC97_LINEIN_VOL );
  card->ac97_phone  = codec_read( card, AC97_PHONE_VOL );
#endif
}


void
RestoreMixerState( struct CMI8738_DATA* card )
{
    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

#if 0
  codec_write(card, AC97_MIC_VOL,    card->ac97_mic );
  codec_write(card, AC97_CD_VOL,     card->ac97_cd );
  codec_write(card, AC97_VIDEO_VOL,  card->ac97_video );
  codec_write(card, AC97_AUX_VOL,    card->ac97_aux );
  codec_write(card, AC97_LINEIN_VOL, card->ac97_linein );
  codec_write(card, AC97_PHONE_VOL,  card->ac97_phone );
#endif
}

void
UpdateMonitorMixer( struct CMI8738_DATA* card )
{
    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

#if 0
  int   i  = InputBits[ card->input ];
  UWORD m  = card->monitor_volume_bits & 0x801f;
  UWORD s  = card->monitor_volume_bits;
  UWORD mm = AC97_MUTE | 0x0008;
  UWORD sm = AC97_MUTE | 0x0808;

  if( i == AC97_RECMUX_STEREO_MIX ||
      i == AC97_RECMUX_MONO_MIX )
  {
    // Use the original mixer settings
    RestoreMixerState( card );
  }
  else
  {
    codec_write(card, AC97_MIC_VOL,
		       i == AC97_RECMUX_MIC ? m : mm );

    codec_write(card, AC97_CD_VOL,
		       i == AC97_RECMUX_CD ? s : sm );

    codec_write(card, AC97_VIDEO_VOL,
		       i == AC97_RECMUX_VIDEO ? s : sm );

    codec_write(card, AC97_AUX_VOL,
		       i == AC97_RECMUX_AUX ? s : sm );

    codec_write(card, AC97_LINEIN_VOL,
		       i == AC97_RECMUX_LINE ? s : sm );

    codec_write(card, AC97_PHONE_VOL,
		       i == AC97_RECMUX_PHONE ? m : mm );
  }
#endif
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


APTR DMAheader = 0;
#define GFXMEM_BUFFER (64 * 1024) // 64KB should be enough for everyone...

void *pci_alloc_consistent(size_t size, APTR *NonAlignedAddress, unsigned int boundary)
{
    void* address;
    unsigned long a;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

    address = (void *) AllocVec(size + boundary, MEMF_PUBLIC | MEMF_CLEAR);

    if (address != NULL)
    {
        a = (unsigned long) address;
        a = (a + boundary - 1) & ~(boundary - 1);
        address = (void *) a;
    }

    if (NonAlignedAddress)
    {
        *NonAlignedAddress = address;
    }

    return address;
}


void pci_free_consistent(void* addr)
{
    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

    FreeVec(addr);
}

static ULONG ResetHandler(struct ExceptionContext *ctx, struct ExecBase *pExecBase, struct CMI8738_DATA *card)
{
    struct PCIDevice *dev = card->pci_dev;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

    ClearMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH0_INTR_ENABLE);
    ClearMask(dev, card, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_ENABLE);

    return 0UL;
}

#if !defined(__AROS__)
void AddResetHandler(struct CMI8738_DATA *card)
{
    static struct Interrupt interrupt;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

    interrupt.is_Code = (void (*)())ResetHandler;
    interrupt.is_Data = (APTR) card;
    interrupt.is_Node.ln_Pri  = 0;
    interrupt.is_Node.ln_Type = NT_EXTINTERRUPT;
    interrupt.is_Node.ln_Name = "CMI8738 Reset Handler";

    AddResetCallback( &interrupt );
}
#endif
