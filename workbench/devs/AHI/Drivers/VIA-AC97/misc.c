/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    Copyright © 2010-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <config.h>

#include <exec/memory.h>
#include <proto/expansion.h>

#include <proto/dos.h>

#include "library.h"
#include "regs.h"
#include "interrupt.h"
#include "hwaccess.h"
#include "misc.h"
#include "pci_wrapper.h"

//#define DEBUG 1

#ifdef __AROS__
#include <asm/io.h>
#include <aros/debug.h>
#define DebugPrintF bug
#endif

//SB- Some debug/err/info output stuff.

#define ERR(a) DebugPrintF("[VIA-AC97] Error: " a "\n")
#define INF(a) DebugPrintF("[VIA-AC97] Info: " a "\n")
#define INFL(a,b) DebugPrintF("[VIA-AC97] Info: " a " (0x%08lx).\n", (long)b)

#ifdef DEBUG
#define DBG(a) DebugPrintF( "[VIA-AC97] Debug: " a "\n" )
#define DBGL(a,b) DebugPrintF( "[VIA-AC97] Debug: " a " (0x%08lx).\n", (long)b )
#else
#define DBG(a)
#define DBGL(a,b)
#endif

/* Global in Card.c */
extern const UWORD InputBits[];
#ifdef __amigaos4__
extern struct DOSIFace *IDOS;
extern struct PCIIFace*            IPCI;
#endif

/* Public functions in main.c */
int card_init(struct CardData *card);
void card_cleanup(struct CardData *card);


struct Device             *TimerBase      = NULL;
struct timerequest *TimerIO        = NULL;
struct MsgPort *replymp = NULL;
void AddResetHandler(struct CardData *card);

static const unsigned long IO_PWR_MANAGEMENT = 0xdd00;
static const unsigned long IO_HW_MONITOR = 0xec00;
static const unsigned long IO_SGD = 0xdc00;
static const unsigned long IO_FM = 0xe000;
static const unsigned long IO_MIDI = 0xe400;

#ifdef __AROS__
INTGW(static, void,  playbackinterrupt, PlaybackInterrupt);
INTGW(static, void,  recordinterrupt,   RecordInterrupt);
INTGW(static, ULONG, cardinterrupt,  CardInterrupt);
#endif


void MicroDelay(unsigned int val)
{
    replymp = (struct MsgPort *) CreateMsgPort();
    if( !replymp )
    {
      DebugPrintF("Could not create the reply port!\n" );
      return;
    }
    
    TimerIO = (struct timerequest *) CreateIORequest( replymp, sizeof( struct timerequest) );

    if( TimerIO == NULL)
    {
      DebugPrintF( "Out of memory.\n" );
      return;
    }
    
    if( OpenDevice( "timer.device", UNIT_MICROHZ, (struct IORequest *) TimerIO, 0) != 0 )
    {
      DebugPrintF( "Unable to open 'timer.device'.\n" );
      return; 
    }
    else
    {
      TimerBase = (struct Device *) TimerIO->tr_node.io_Device;
    }
    
    if (TimerIO)
    {
        TimerIO->tr_node.io_Command = TR_ADDREQUEST; /* Add a request.   */
        TimerIO->tr_time.tv_secs = 0;                /* 0 seconds.      */
        TimerIO->tr_time.tv_micro = val;             /* 'val' micro seconds. */
        DoIO( (struct IORequest *) TimerIO );
        CloseDevice( (struct IORequest *) TimerIO );
        DeleteIORequest( (struct IORequest *) TimerIO);
        TimerIO = NULL;
    }
    
    if( replymp )
      DeleteMsgPort(replymp);
}


/******************************************************************************
** DriverData allocation ******************************************************
******************************************************************************/

// This code used to be in _AHIsub_AllocAudio(), but since we're now
// handling CAMD support too, it needs to be done at driver loading
// time.

struct CardData*
AllocDriverData( struct PCIDevice *    dev,
		 struct DriverBase* AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;
  struct CardData* dd;
  UWORD               command_word;
  int i;
  unsigned short uval;

  // FIXME: This should be non-cachable, DMA-able memory
  dd = AllocVec( sizeof( *dd ), MEMF_PUBLIC | MEMF_CLEAR );

  if( dd == NULL )
  {
    Req( "Unable to allocate driver structure." );
    return NULL;
  }

  dd->ahisubbase = AHIsubBase;

  dd->interrupt.is_Node.ln_Type = IRQTYPE;
  dd->interrupt.is_Node.ln_Pri  = 0;
  dd->interrupt.is_Node.ln_Name = (STRPTR) LibName;
#ifdef __AROS__
  dd->interrupt.is_Code         = (void(*)(void)) cardinterrupt;
#else
  dd->interrupt.is_Code         = (void(*)(void)) CardInterrupt;
#endif
  dd->interrupt.is_Data         = (APTR) dd;

  dd->playback_interrupt.is_Node.ln_Type = IRQTYPE;
  dd->playback_interrupt.is_Node.ln_Pri  = 0;
  dd->playback_interrupt.is_Node.ln_Name = (STRPTR) LibName;
#ifdef __AROS__
  dd->playback_interrupt.is_Code         = playbackinterrupt;
#else
  dd->playback_interrupt.is_Code         = PlaybackInterrupt;
#endif
  dd->playback_interrupt.is_Data         = (APTR) dd;

  dd->record_interrupt.is_Node.ln_Type = IRQTYPE;
  dd->record_interrupt.is_Node.ln_Pri  = 0;
  dd->record_interrupt.is_Node.ln_Name = (STRPTR) LibName;
#ifdef __AROS__
  dd->record_interrupt.is_Code         = recordinterrupt;
#else
  dd->record_interrupt.is_Code         = RecordInterrupt;
#endif
  dd->record_interrupt.is_Data         = (APTR) dd;

  dd->pci_dev = dev;

  outw_config(PCI_COMMAND, 0x00, dd->pci_dev);
  //dev->WriteConfigLong( PCI_BASE_ADDRESS_0, 0xdc00 );
  
  //SB- Configure IO, if required. Force IO into 16bit/ISA address space, since it
  //    apparently won't work in 32bit address range. Really ought to check this in
  //    Linux sometime...
  unsigned long tula = ( inl_config(PCI_BASE_ADDRESS_0, dd->pci_dev) & 0xfffffffc );
  if( ( tula == 0 ) || ( tula & 0xffff0000 ) )
  {
    outl_config(PCI_BASE_ADDRESS_0, IO_SGD, dd->pci_dev);
    DBGL( "configured audio SGD IO base", IO_SGD );
  }

  tula = ( inl_config(PCI_BASE_ADDRESS_1, dd->pci_dev) & 0xfffffffc );
  if( ( tula == 0 ) || ( tula & 0xffff0000 ) )
  {
	outl_config(PCI_BASE_ADDRESS_1, IO_FM, dd->pci_dev);
      //DBGL( "configured audio FM IO base", IO_FM );
  }

  tula = ( inl_config(PCI_BASE_ADDRESS_2, dd->pci_dev) & 0xfffffffc );
  if( ( tula == 0 ) || ( tula & 0xffff0000 ) )
  {
	outl_config(PCI_BASE_ADDRESS_2, IO_MIDI, dd->pci_dev);
    //DBGL( "configured audio MIDI IO base", IO_MIDI );
  }
  
  outw_config(PCI_COMMAND, PCI_COMMAND_IO, dd->pci_dev);
  
  dd->pci_master_enabled = TRUE;

  dd->iobase  = inl_config(PCI_BASE_ADDRESS_0, dd->pci_dev) & 0xfffffffe;
  
  dd->length  = ~( ahi_pci_get_base_size(0, dd->pci_dev) & PCI_BASE_ADDRESS_IO_MASK );
  dd->irq     = ahi_pci_get_irq(dev);
  dd->chiprev = inb_config(PCI_REVISION_ID, dev);
  dd->model   = inw_config(PCI_SUBSYSTEM_ID, dev);
  
  dd->table.area = NULL;
  dd->table.addr = NULL;
  dd->table.bytes = 0;
  dd->play_idx_table = NULL;
  dd->rec_idx_table = NULL;

  /* Initialize chip */
  if( card_init( dd ) < 0 )
  {
    DebugPrintF("Unable to initialize Card subsystem.\n");
    
    FreeVec(dd);
    return NULL;
  }
  
  
  ahi_pci_add_intserver(&dd->interrupt, dd->pci_dev);
  dd->interrupt_added = TRUE;

  dd->card_initialized = TRUE;
  dd->input          = 0;
  dd->output         = 0;
  dd->monitor_volume = Linear2MixerGain( 0x10000, &dd->monitor_volume_bits );
  dd->input_gain     = Linear2RecordGain( 0x10000, &dd->input_gain_bits );
  dd->output_volume  = Linear2MixerGain( 0x10000, &dd->output_volume_bits );
  SaveMixerState(dd);
  
  AddResetHandler(dd);
  
  return dd;
}


/******************************************************************************
** DriverData deallocation ****************************************************
******************************************************************************/

// And this code used to be in _AHIsub_FreeAudio().

void
FreeDriverData( struct CardData* dd,
		struct DriverBase*  AHIsubBase )
{
  if( dd != NULL )
  {
    if( dd->pci_dev != NULL )
    {
      if( dd->card_initialized )
      {
        card_cleanup( dd );
      }

      if( dd->pci_master_enabled )
      {
        UWORD cmd;

        cmd = inw_config(PCI_COMMAND, dd->pci_dev);
        cmd &= ~( PCI_COMMAND_IO | PCI_COMMAND_MASTER );
        outw_config(PCI_COMMAND, cmd, dd->pci_dev);
      }
    }

    if( dd->reset_handler_added )
    {
      RemResetCallback(&dd->reset_handler);
    }

    if( dd->interrupt_added )
    {
      ahi_pci_rem_intserver(&dd->interrupt, dd->pci_dev);
    }

    FreeVec( dd );
  }
}


void channel_reset(struct CardData *card)
{
    struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;
    
        pci_outb(VIA_REG_CTRL_TERMINATE /*| VIA_REG_CTRL_RESET*/, VIA_REG_OFFSET_CONTROL, card);
        pci_inb(VIA_REG_OFFSET_CONTROL, card);
        udelay(50);
        /* disable interrupts */
        pci_outb(0x00, VIA_REG_OFFSET_CONTROL, card);
        /* clear interrupts */
        pci_outb(0x03, VIA_REG_OFFSET_STATUS, card);
        pci_outb(0x00, VIA_REG_OFFSET_TYPE, card); /* for via686 */
        // pci_outl(0, VIA_REG_OFFSET_CURR_PTR, card);
}

//reset/init ac97 codec. Returns false if the primary codec isn't found/ready.
BOOL ac97_reset( struct PCIDevice *via686b_audio)
{
   static const unsigned long reset_delay   = 100;  //link operation delay, should be some uS..
   static const unsigned long codec_timeout = 50;   //codec ready timeout, about half a second...

   //cold reset..
   //SB- Set the link to a known initial configuration.
   //SB- Note: The 'standard' ***x reset code is quite similar to this, except it
   //          de-asserts SYNC at this point, possibly in case the link/codec is
   //          in low power mode. We don't, because if we do, the dreaded half-
   //          rate problem occurs, with ~100% reliability (..interesting).
   //          It is a bit of a worry that the 'standard' reset code fails, but
   //          perhaps it is simply because we have a completely untouched link,
   //          whereas on any 686 based PC, the link was probably confugured
   //          already in the BIOS.

   outb_config(VIA_ACLINK_CTRL,
      VIA_ACLINK_CTRL_ENABLE | VIA_ACLINK_CTRL_RESET,
      via686b_audio);
    MicroDelay( reset_delay );

   //SB- Assert reset.
   outb_config(VIA_ACLINK_CTRL, VIA_ACLINK_CTRL_ENABLE, via686b_audio);
   MicroDelay( reset_delay );

   //SB- De-assert reset, enable VRA/PCM etc.
   outb_config( VIA_ACLINK_CTRL, VIA_ACLINK_CTRL_ENABLE |
                                                    VIA_ACLINK_CTRL_RESET |
                                                    VIA_ACLINK_CTRL_VRA |
                                                    VIA_ACLINK_CTRL_PCM,
                                                    via686b_audio);
   MicroDelay( reset_delay );

   //SB- Check primary codec...
   unsigned long delay = codec_timeout;
   while( delay-- )
   {
      if(inb_config( VIA_ACLINK_STAT, via686b_audio) & VIA_ACLINK_C00_READY)
      {
         //DBG( "AC-Link reset ok." );
         return TRUE;
      }

      MicroDelay( 10000 );
   }

   DBG( "AC-Link reset, primary codec not ready!" );
   return FALSE;
}




int card_init(struct CardData *card)
{
   struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;
   unsigned short cod, uval;
   unsigned char pval, byt;
   long *ptr;
   int teller = 0;
   ULONG val;
   struct PCIDevice *via686b_ACPI;
   BOOL 			aclink	= FALSE;


#ifdef __amigaos4__
   via686b_ACPI = IPCI->FindDeviceTags( FDT_VendorID, 0x1106, FDT_DeviceID, 0x3057,
                                                              FDT_Index,    0x00,
                                                              TAG_DONE );
#else
   via686b_ACPI = ahi_pci_find_device(0x1106, 0x3057, NULL);
#endif

   if (via686b_ACPI == NULL) // try device 0x3058
   {
#ifdef __amigaos4__
      via686b_ACPI = IPCI->FindDeviceTags( FDT_VendorID, 0x1106, FDT_DeviceID, 0x3058,
                                                              FDT_Index,    0x00,
                                                              TAG_DONE );
#else
      via686b_ACPI = ahi_pci_find_device(0x1106, 0x3058, NULL);
#endif
   }

   if (via686b_ACPI)
   {
      BOOL lock;

#ifdef __amigaos4__
      lock = via686b_ACPI->Lock( PCI_LOCK_SHARED );
      if (lock == FALSE)
      {
         DBG( "couldn't lock the ACPI! Trying anyway..." );
      }
#endif

      //SB- Configure power management, if it isn't already.
      if( !( inl_config(0x48, via686b_ACPI) & 0xfffffffe ) )
      {
         outl_config(0x48, IO_PWR_MANAGEMENT, via686b_ACPI);
         DBGL( "configured power management IO", IO_PWR_MANAGEMENT );
      }
      else
      {
         DBG("Power management IO already configured");
      }

      //SB- Enable IO (preserve other bits, but note that technically speaking
      //    we should also be clreaing PSON gating here (it's already cleared))
      outb_config( 0x41, (inb_config(0x41, via686b_ACPI) | 0x80), via686b_ACPI);
        
      //SB- Power up the 686b, if it isn't already.
      if( !( inb_config(0x42, via686b_ACPI) & 0x40 ) )
      {
         //Cause a soft resume event...
         outb(inb(IO_PWR_MANAGEMENT + 0x05) | 0x80, IO_PWR_MANAGEMENT + 0x05 );
            
         //Busy loop until the soft resume completes.
         //We have a bail out counter, but no idea how long we should wait really.
         //..1/10th of a second wasn't long enough
 	     unsigned long delay = 25;	  //1/2 a second, or so

     	 while( --delay )
         {
           if( ( inb_config( 0x42, via686b_ACPI) & 0x40))
           {
              DBG( "powered up the 686b." );
              break; //SUSC# state
           }
                
           Delay(1);
         }
            
         if( delay == 0 )
            ERR( "soft resume timed out!" );
      }
      else
      {
          DBG("VIA already powered up");
      }

#ifdef __amigaos4__
      if (lock)
         via686b_ACPI->Unlock();

      IPCI->FreeDevice( via686b_ACPI );
#endif
   }
   else
   {
      ERR( "couldn't find the ACPI!" );
      return -1;
   }

   //SB- Don't know if there's a codec yet?
//   codec_write(card, 0x2, 0x8000);
//   codec_write(card, 0x4, 0x8000);
    
   //SB- Check codec..
   aclink = ( inb_config(VIA_ACLINK_STAT, dev) & VIA_ACLINK_C00_READY );
           
   //SB- If no codec, reset.
   //SB- Note: This is a possible source of trouble. It might be safer to reset
   //          every time. Only reason not to is 'pop' avoidance on soft reboot
   //          and in case UBoot did the reset.
   if( !aclink )
   {
      aclink = ac97_reset(dev);
   }
   else
   {
      //SB- Make sure VRA/PCM are enabled if we didn't use our own reset code.

      outb_config(VIA_ACLINK_CTRL, VIA_ACLINK_CTRL_ENABLE |\
                                             VIA_ACLINK_CTRL_RESET |\
                                             VIA_ACLINK_CTRL_VRA |\
                                             VIA_ACLINK_CTRL_PCM, dev);
   }

   if( aclink )
   {
       //INF( "initialized AC'97 codec." );
   }
   else
   {
       ERR( "sorry, you don't seem to have the AC'97 codec!" );
       return -1;
   }

/*
   Delay( 1 );


   pval = inb_config(VIA_ACLINK_STAT, dev);
   
    if (! (pval & VIA_ACLINK_C00_READY))
    {
        DebugPrintF("WHY?\n");
        pci_write_config_byte(chip->pci, VIA_ACLINK_CTRL,
                                      VIA_ACLINK_CTRL_ENABLE |
                                      VIA_ACLINK_CTRL_RESET |
                                      VIA_ACLINK_CTRL_SYNC);
        udelay(100);
        
        pci_write_config_byte(chip->pci, VIA_ACLINK_CTRL, 0x00);
        udelay(100);
        
        pci_write_config_byte(chip->pci, VIA_ACLINK_CTRL, VIA_ACLINK_CTRL_INIT);
        udelay(100);
        
        pval = inb_config(VIA_ACLINK_STAT, dd->pci_dev);
        
        if (! (pval & VIA_ACLINK_C00_READY))
        {
            return -1;
        }
    }

    while (teller < 1000)
    {
        pval = inb_config(VIA_ACLINK_STAT, dd->pci_dev);
        if (pval & VIA_ACLINK_C00_READY)
            break;
        
        udelay(20);
        teller++;
    }
*/

    if ((val = pci_inl(VIA_CODEC_CMD, card)) & VIA_REG_AC97_BUSY)
        snd_printk("AC97 codec is not ready!\n");
   
    outb_config(VIA_FM_NMI_CTRL, 0, card->pci_dev);
    pci_outl(0, VIA_REG_GPI_INTR, card);

    channel_reset(card);

    codec_write(card, AC97_MASTER_VOL_STEREO, 0x0000 ); // no attenuation
    codec_write(card, AC97_AUXOUT_VOL,        0x8000 ); // volume of the rear output
    codec_write(card, AC97_MASTER_VOL_MONO,   0x8000 );
    codec_write(card, AC97_MASTER_TONE,       0x0f0f ); // bass/treble control (if present)
    codec_write(card, AC97_PCBEEP_VOL,        0x8000 ); // PC beep internal speaker?

    codec_write(card, AC97_RECORD_SELECT,     0);
    codec_write(card, AC97_RECORD_GAIN,       0x0000 ); // 0 dB gain
   
   
    // Analog mixer input gain registers
    codec_write(card, AC97_PHONE_VOL,         AC97_MUTE | 0x0008 );
    codec_write(card, AC97_MIC_VOL,           AC97_MUTE | 0x0048 ); // 10 dB boost
    codec_write(card, AC97_LINEIN_VOL,        AC97_MUTE | 0x0808 );
    codec_write(card, AC97_CD_VOL,            0x0808 );
    codec_write(card, AC97_VIDEO_VOL,         AC97_MUTE | 0x0808 );
    codec_write(card, AC97_AUX_VOL,           0x0808 );
    codec_write(card, AC97_PCMOUT_VOL,        0x0808 );
    
    DBG("card_init() was a success!");

    return 0;
}


void card_cleanup(struct CardData *card)
{
}



/******************************************************************************
** Misc. **********************************************************************
******************************************************************************/

void
SaveMixerState( struct CardData* dd )
{
  dd->ac97_mic    = codec_read( dd, AC97_MIC_VOL );
  dd->ac97_cd     = codec_read( dd, AC97_CD_VOL );
  dd->ac97_video  = codec_read( dd, AC97_VIDEO_VOL );
  dd->ac97_aux    = codec_read( dd, AC97_AUX_VOL );
  dd->ac97_linein = codec_read( dd, AC97_LINEIN_VOL );
  dd->ac97_phone  = codec_read( dd, AC97_PHONE_VOL );
}


void
RestoreMixerState( struct CardData* dd )
{
  codec_write(dd, AC97_MIC_VOL,    dd->ac97_mic );
  codec_write(dd, AC97_CD_VOL,     dd->ac97_cd );
  codec_write(dd, AC97_VIDEO_VOL,  dd->ac97_video );
  codec_write(dd, AC97_AUX_VOL,    dd->ac97_aux );
  codec_write(dd, AC97_LINEIN_VOL, dd->ac97_linein );
  codec_write(dd, AC97_PHONE_VOL,  dd->ac97_phone );
}

void
UpdateMonitorMixer( struct CardData* dd )
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
    codec_write(dd, AC97_MIC_VOL,
		       i == AC97_RECMUX_MIC ? m : mm );

    codec_write(dd, AC97_CD_VOL,
		       i == AC97_RECMUX_CD ? s : sm );

    codec_write(dd, AC97_VIDEO_VOL,
		       i == AC97_RECMUX_VIDEO ? s : sm );

    codec_write(dd, AC97_AUX_VOL,
		       i == AC97_RECMUX_AUX ? s : sm );

    codec_write(dd, AC97_LINEIN_VOL,
		       i == AC97_RECMUX_LINE ? s : sm );

    codec_write(dd, AC97_PHONE_VOL,
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


#define CACHELINE_SIZE 32

void *pci_alloc_consistent(size_t size, APTR *NonAlignedAddress)
{
  void* address;
  unsigned long a;

#ifdef __amigaos4__
  if (OpenResource("newmemory.resource"))
  {
      address = AllocVecTags(size, AVT_Type, MEMF_SHARED, AVT_Contiguous, TRUE, AVT_Lock, TRUE,
                                    AVT_PhysicalAlignment, 32, AVT_Clear, 0, TAG_DONE);
  }
  else
#endif
  {
      address = AllocVec( size + CACHELINE_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
    
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
  FreeVec( addr );
}


#ifdef __amigaos4__
static ULONG ResetHandler(struct ExceptionContext *ctx, struct ExecBase *pExecBase, struct CardData *card)
#else
static ULONG ResetHandler(struct CardData *card)
#endif
{
    struct PCIDevice *dev = card->pci_dev;
    
    unsigned char val = VIA_REG_CTRL_TERMINATE;
    
    pci_outb(val, VIA_REG_OFFSET_CONTROL, card);
    pci_outb(val, VIA_REG_OFFSET_CONTROL + RECORD, card);

    channel_reset(card);

    return 0UL;
}


void AddResetHandler(struct CardData *card)
{
    struct Interrupt *handler = &card->reset_handler;

    handler->is_Code = (APTR)ResetHandler;
    handler->is_Data = (APTR) card;
    handler->is_Node.ln_Pri  = 0;
#ifdef __amigaos4__
    handler->is_Node.ln_Type = NT_EXTINTERRUPT;
#else
    handler->is_Node.ln_Type = NT_INTERRUPT;
#endif
    handler->is_Node.ln_Name = "VIA-AC97 reset handler";

    card->reset_handler_added = AddResetCallback(handler);
}

