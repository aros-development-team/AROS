/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License");
you may not use this file except in compliance with the License. You may obtain a copy of the License at 
http://www.aros.org/license.html 
Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF 
ANY KIND, either express or implied. See the License for the specific language governing rights and 
limitations under the License. 

The Original Code is written by Davy Wentzler.
*/

#include <config.h>

#include <exec/memory.h>
#ifdef __amigaos4__
#undef __USE_INLINE__
#include <proto/expansion.h>
#else
#include <libraries/openpci.h>
#include <proto/openpci.h>
#endif

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/fakedma.h>
#include <string.h>

#include "library_card.h"
#include "regs.h"
#include "interrupt.h"
#include "misc.h"
#include "DriverData.h"

#define CACHELINE_SIZE 32

/* Global in Card.c */
extern const UWORD InputBits[];
extern struct DOSIFace *IDOS;
extern struct FakeDMAIFace *IFakeDMA;

/* Public functions in main.c */
int card_init(struct CardData *card);
void card_cleanup(struct CardData *card);
void AddResetHandler(struct CardData *card);


void WritePartialMask(struct PCIDevice *dev, struct CardData* card, unsigned long reg, unsigned long shift, unsigned long mask, unsigned long val)
{
    ULONG tmp;
    
    tmp = dev->InLong(card->iobase + reg);
    tmp &= ~(mask << shift);
    tmp |= val << shift;
    dev->OutLong(card->iobase + reg, tmp);
}


void ClearMask(struct PCIDevice *dev, struct CardData* card, unsigned long reg, unsigned long mask)
{
    ULONG tmp;
    
    tmp = dev->InLong(card->iobase + reg);
    tmp &= ~mask;
    dev->OutLong(card->iobase + reg, tmp);
}


void WriteMask(struct PCIDevice *dev, struct CardData* card, unsigned long reg, unsigned long mask)
{
    ULONG tmp;
    
    tmp = dev->InLong(card->iobase + reg);
    tmp |= mask;
    dev->OutLong(card->iobase + reg, tmp);
}

void cmimix_wr(struct PCIDevice *dev, struct CardData* card, unsigned char port, unsigned char val)
{
        dev->OutByte(card->iobase + CMPCI_REG_SBADDR, port);
        dev->OutByte(card->iobase + CMPCI_REG_SBDATA, val);
}

unsigned char cmimix_rd(struct PCIDevice *dev, struct CardData* card, unsigned char port)
{
        dev->OutByte(card->iobase + CMPCI_REG_SBADDR, port);
        return (unsigned char) dev->InByte(card->iobase + CMPCI_REG_SBDATA);
}


/******************************************************************************
** DriverData allocation ******************************************************
******************************************************************************/

// This code used to be in _AHIsub_AllocAudio(), but since we're now
// handling CAMD support too, it needs to be done at driver loading
// time.

struct CardData*
AllocDriverData( struct PCIDevice *dev,
		 struct DriverBase* AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;
  struct CardData* card;
  UWORD command_word;
  int i, v;
  unsigned char byte;

  // FIXME: This should be non-cachable, DMA-able memory
  card = IExec->AllocVec( sizeof( *card ), MEMF_PUBLIC | MEMF_CLEAR );

  if( card == NULL )
  {
    Req( "Unable to allocate driver structure." );
    return NULL;
  }

  card->ahisubbase = AHIsubBase;

  card->interrupt.is_Node.ln_Type = NT_EXTINTERRUPT;
  card->interrupt.is_Node.ln_Pri  = 0;
  card->interrupt.is_Node.ln_Name = (STRPTR) LibName;
  card->interrupt.is_Code         = (void(*)(void)) CardInterrupt;
  card->interrupt.is_Data         = (APTR) card;

  card->playback_interrupt.is_Node.ln_Type = NT_EXTINTERRUPT;
  card->playback_interrupt.is_Node.ln_Pri  = 0;
  card->playback_interrupt.is_Node.ln_Name = (STRPTR) LibName;
  card->playback_interrupt.is_Code         = PlaybackInterrupt;
  card->playback_interrupt.is_Data         = (APTR) card;

  card->record_interrupt.is_Node.ln_Type = NT_EXTINTERRUPT;
  card->record_interrupt.is_Node.ln_Pri  = 0;
  card->record_interrupt.is_Node.ln_Name = (STRPTR) LibName;
  card->record_interrupt.is_Code         = RecordInterrupt;
  card->record_interrupt.is_Data         = (APTR) card;

  card->pci_dev = dev;

  command_word = dev->ReadConfigWord( PCI_COMMAND );  
  command_word |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
  dev->WriteConfigWord( PCI_COMMAND, command_word );

  card->pci_master_enabled = TRUE;

  /*for (i = 0; i < 6; i++)
  {
     if (dev->GetResourceRange(i))
         IExec->DebugPrintF("BAR[%ld] = %lx\n", i, dev->GetResourceRange(i)->BaseAddress);
  }*/

  card->iobase  = dev->GetResourceRange(0)->BaseAddress;
  card->length  = ~( dev->GetResourceRange(0)->Size & PCI_BASE_ADDRESS_IO_MASK );
  card->irq     = dev->MapInterrupt();
  card->chiprev = dev->ReadConfigByte( PCI_REVISION_ID);
  card->model   = dev->ReadConfigWord( PCI_SUBSYSTEM_ID);

  /*IExec->DebugPrintF("---> chiprev = %u, model = %x, Vendor = %x\n", dev->ReadConfigByte( PCI_REVISION_ID), dev->ReadConfigWord( PCI_SUBSYSTEM_ID),
                     dev->ReadConfigWord( PCI_SUBSYSTEM_VENDOR_ID));*/


  /* Initialize chip */
  if( card_init( card ) < 0 )
  {
    IExec->DebugPrintF("Unable to initialize Card subsystem.");
    return NULL;
  }


  //IExec->DebugPrintF("INTERRUPT %lu\n", dev->MapInterrupt());
  IExec->AddIntServer(dev->MapInterrupt(), &card->interrupt );
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
  
  byte = dev->InByte(card->iobase + CMPCI_REG_MIXER25);
  dev->OutByte(card->iobase + CMPCI_REG_MIXER25, byte & ~0x30); // mute Aux
  dev->OutByte(card->iobase + CMPCI_REG_MIXER25, byte & ~0x01); // turn on mic 20dB boost
  dev->OutByte(card->iobase + CMPCI_REG_MIXER_AUX, 0x00);
  
  byte = dev->InByte(card->iobase + CMPCI_REG_MIXER24);
  dev->OutByte(card->iobase + CMPCI_REG_MIXER24, byte | CMPCI_REG_FMMUTE);
  
  cmimix_wr(dev, card, CMPCI_SB16_MIXER_MIC, 0x00);
  
  cmimix_wr(dev, card, CMPCI_SB16_MIXER_MASTER_L, 0xFF);
  cmimix_wr(dev, card, CMPCI_SB16_MIXER_MASTER_R, 0xFF);
  
  card->mixerstate = cmimix_rd(dev, card, CMPCI_SB16_MIXER_OUTMIX);
  
  AddResetHandler(card);
  
  return card;
}


/******************************************************************************
** DriverData deallocation ****************************************************
******************************************************************************/

// And this code used to be in _AHIsub_FreeAudio().

void
FreeDriverData( struct CardData* card,
		struct DriverBase*  AHIsubBase )
{
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

        cmd = ((struct PCIDevice * ) card->pci_dev)->ReadConfigWord( PCI_COMMAND );
        cmd &= ~( PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER );
        ((struct PCIDevice * ) card->pci_dev)->WriteConfigWord( PCI_COMMAND, cmd );
      }
    }

    if( card->interrupt_added )
    {
      IExec->RemIntServer(((struct PCIDevice * ) card->pci_dev)->MapInterrupt(), &card->interrupt );
    }

    IExec->FreeVec( card );
  }
}


int card_init(struct CardData *card)
{
    struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;
    unsigned short cod;
    unsigned long val;

    ClearMask(dev, card, CMPCI_REG_MISC, CMPCI_REG_POWER_DOWN); // power up
    
    WriteMask(dev, card, CMPCI_REG_MISC, CMPCI_REG_BUS_AND_DSP_RESET);
    IDOS->Delay(1);
    ClearMask(dev, card, CMPCI_REG_MISC, CMPCI_REG_BUS_AND_DSP_RESET);

    /* reset channels */
    WriteMask(dev, card, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_RESET | CMPCI_REG_CH1_RESET);
     IDOS->Delay(1);
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
	ClearMask(dev, card, CMPCI_REG_MISC, 0x100);
	ClearMask(dev, card, CMPCI_REG_MISC, CMPCI_REG_N4SPK3D);
    WriteMask(dev, card, CMPCI_REG_INTR_STATUS, CMPCI_REG_LEGACY_STEREO | CMPCI_REG_LEGACY_HDMA);

    return 0;
}


void card_cleanup(struct CardData *card)
{
}



/******************************************************************************
** Misc. **********************************************************************
******************************************************************************/

void
SaveMixerState( struct CardData* card )
{
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
RestoreMixerState( struct CardData* card )
{
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
UpdateMonitorMixer( struct CardData* card )
{
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

void *pci_alloc_consistent(size_t size, APTR *NonAlignedAddress)
{
  void* address;
  unsigned long a;

  if (IFakeDMA)
  {
        if (size > GFXMEM_BUFFER)
        {
            IExec->DebugPrintF("Error allocating memory! Asking %ld bytes\n", size);
        }

        if (DMAheader == 0)
        {
            DMAheader = IFakeDMA->AllocDMABuffer(GFXMEM_BUFFER); // alloc once
        }

        if (DMAheader)
        {
			APTR bufStart = IFakeDMA->GetDMABufferAttr(DMAheader, DMAB_PUDDLE );
			ULONG bufSize = (ULONG)IFakeDMA->GetDMABufferAttr(DMAheader, DMAB_SIZE );

            address = bufStart;
            *NonAlignedAddress = address;

            a = (unsigned long) address;
            a = (a + CACHELINE_SIZE - 1) & ~(CACHELINE_SIZE - 1);
            address = (void *) a;

            return address;
        }
  }

  IExec->DebugPrintF("No gfx mem\n");
  if (IExec->OpenResource("newmemory.resource"))
  {
      address = IExec->AllocVecTags(size, AVT_Type, MEMF_SHARED, AVT_Contiguous, TRUE, AVT_Lock, TRUE,
                                    AVT_PhysicalAlignment, 32, AVT_Clear, 0, TAG_DONE);
  }
  else
  {
      address = IExec->AllocVec( size + CACHELINE_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
    
      if( address != NULL )
      {
        a = (unsigned long) address;
        a = (a + CACHELINE_SIZE - 1) & ~(CACHELINE_SIZE - 1);
        address = (void *) a;
      }
  }
  
  *NonAlignedAddress = address;
  
  return address;
}


void pci_free_consistent(void* addr)
{
  if (IFakeDMA)
  {
  }
  else
  {
      IExec->FreeVec( addr );
  }
}

static ULONG ResetHandler(struct ExceptionContext *ctx, struct ExecBase *pExecBase, struct CardData *card)
{
    struct PCIDevice *dev = card->pci_dev;
    
    ClearMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH0_INTR_ENABLE);
    ClearMask(dev, card, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_ENABLE);

    return 0UL;
}


void AddResetHandler(struct CardData *card)
{
    static struct Interrupt interrupt;

    interrupt.is_Code = (void (*)())ResetHandler;
    interrupt.is_Data = (APTR) card;
    interrupt.is_Node.ln_Pri  = 0;
    interrupt.is_Node.ln_Type = NT_EXTINTERRUPT;
    interrupt.is_Node.ln_Name = "reset handler";

    IExec->AddResetCallback( &interrupt );
}

