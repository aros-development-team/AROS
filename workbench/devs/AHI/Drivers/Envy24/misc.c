/*
    Copyright ï¿½ 2004-2014, Davy Wentzler. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#ifdef __AMIGAOS4__
#undef __USE_INLINE__
#include <proto/expansion.h>
#endif

#include <proto/dos.h>

#include "library.h"
#include "regs.h"
#include "interrupt.h"
#include "misc.h"
#include "pci_wrapper.h"
#include "DriverData.h"
#include "Phase88.h"

#define MAUDIO_2496_ID 0x121434D6
#define MAUDIO_1010LT_ID 0x12143bd6
#define MAUDIO_DELTA44_ID 0x121433d6
#define MAUDIO_DELTA66_ID 0x121432d6

extern const UWORD InputBits[];
#ifdef __AMIGAOS4__
extern struct DOSIFace *IDOS;
#endif

struct Device             *TimerBase      = NULL;
struct timerequest *TimerIO        = NULL;
struct MsgPort *replymp;

/* Public functions in main.c */
int card_init(struct CardData *card, struct DriverBase* AHIsubBase);
void card_cleanup(struct CardData *card);
void AddResetHandler(struct CardData *card);


unsigned char ReadCCI(struct CardData *card, unsigned char address)
{
   pci_outb(address, CCS_ENVY_INDEX, card);
   return pci_inb(CCS_ENVY_DATA, card);
}


void WriteCCI(struct CardData *card, unsigned char address, unsigned char data)
{
   pci_outb(address, CCS_ENVY_INDEX, card);
   pci_outb(data, CCS_ENVY_DATA, card);
}


unsigned char GetGPIOData(struct CardData *card)
{
    return ReadCCI(card, CCI_GPIO_DATA);
}


void SetGPIOData(struct CardData *card, unsigned char data)
{
    WriteCCI(card, CCI_GPIO_DATA, data);
}


void SaveGPIOStatus(struct CardData *card)
{
   card->gpio_dir = ReadCCI(card, CCI_GPIO_DIR);
   card->gpio_data = ReadCCI(card, CCI_GPIO_DATA);
}


void RestoreGPIOStatus(struct CardData *card)
{
   WriteCCI(card, CCI_GPIO_DIR, card->gpio_dir);
   WriteCCI(card, CCI_GPIO_DATA, card->gpio_data);
}


void ClearMask8(struct CardData *card, unsigned char reg, unsigned char mask)
{
    UBYTE tmp;
    
    tmp = pci_inb_mt(reg, card);
    tmp &= ~mask;
    pci_outb_mt(tmp, reg, card);
}


void WriteMask8(struct CardData *card, unsigned char reg, unsigned char mask)
{
    UBYTE tmp;
    
    tmp = pci_inb_mt(reg, card);
    tmp |= mask;
    pci_outb_mt(tmp, reg, card);
}


static unsigned char read_i2c(struct PCIDevice *dev, struct CardData *card, unsigned char reg)
{
	long t = 0x10000;

    MicroDelay(500);
	pci_outb(reg, CCS_I2C_ADDR, card);
	pci_outb(0xA0, CCS_I2C_DEV_ADDRESS, card); // 0xA0 is eeprom device (binary: 1010000)
    
	while (t-- > 0 && (pci_inb(CCS_I2C_STATUS, card) & CCS_I2C_BUSY)) ;
    
	return pci_inb(CCS_I2C_DATA, card);
}


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
      DeleteMsgPort( replymp);
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
  struct CardData* card;
  UWORD command_word;
  int i;

  card = AllocVec( sizeof( *card ), MEMF_PUBLIC | MEMF_CLEAR );

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

  command_word = inw_config(PCI_COMMAND, dev);  
  command_word |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
  outw_config(PCI_COMMAND, command_word, dev);

  card->pci_master_enabled = TRUE;


  //for (i = 0; i < 6; i++)
  //    DebugPrintF("BAR[%ld] = %lx\n", i, ahi_pci_get_base_address(i, dev));

  card->iobase  = (IPTR)ahi_pci_get_base_address(0, dev);
  card->mtbase  = (IPTR)ahi_pci_get_base_address(3, dev);
  card->chiprev = inb_config(PCI_REVISION_ID, dev);
  card->model   = inw_config(PCI_SUBSYSTEM_ID, dev);

  //DebugPrintF("---> chiprev = %u, model = %x, Vendor = %x\n", inb_config(PCI_REVISION_ID, dev), inw_config(PCI_SUBSYSTEM_ID, dev),
  //                   inw_config(PCI_SUBSYSTEM_VENDOR_ID, dev));


  /* Initialize chip */
  if( card_init( card, AHIsubBase ) < 0 )
  {
    DebugPrintF("Unable to initialize Card subsystem.");
    return NULL;
  }


  card->interrupt_added = ahi_pci_add_intserver(&card->interrupt, dev);


  card->card_initialized = TRUE;
  card->input          = 0;
  card->output         = 0;
  card->monitor_volume = 0x0;
  card->input_gain     = 0x10000;
  card->output_volume  = 0x10000;
  //SaveMixerState(card);
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

        cmd = inw_config(PCI_COMMAND, card->pci_dev);
        cmd &= ~( PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER );
        outw_config(PCI_COMMAND, cmd, card->pci_dev);
      }
    }

    if( card->interrupt_added )
    {
      ahi_pci_rem_intserver(&card->interrupt, card->pci_dev);
    }

    FreeVec( card );
  }
}



int card_init(struct CardData *card, struct DriverBase* AHIsubBase)
{
    struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;
    unsigned short cod;
    int i;
    unsigned int tmp;
    unsigned char b, eeprom[128];
    
    pci_outb(CCI_PRO_POWER_DOWN, CCS_ENVY_INDEX, card);
    pci_outb(0xFF, CCS_ENVY_DATA, card);
    pci_inb(CCS_ENVY_DATA, card); // dummy read
    MicroDelay(300);
    pci_outb(CCI_PRO_POWER_DOWN, CCS_ENVY_INDEX, card);
    pci_outb(0x00, CCS_ENVY_DATA, card);
    pci_inb(CCS_ENVY_DATA, card); // dummy read
    MicroDelay(300);

    // set up CCS registers
    // reset
    pci_outb(CCS_RESET_ALL | CCS_NATIVE_MODE, CCS_CTRL, card);
    MicroDelay(300);
    pci_outb(CCS_NATIVE_MODE, CCS_CTRL, card);
    MicroDelay(300);

    pci_outb(~CCS_ENABLE_PRO_MACRO, CCS_INTR_MASK, card);
    pci_outb(0xFF, CCS_INTR_STATUS, card); // clear all
    
    if (pci_inb(CCS_I2C_STATUS, card) & 0x80)
    {
        int version, size;
        unsigned long subvendor = 0;
        
        for (i = 0; i < 4; i++)
        {
            read_i2c(dev, card, i);
            MicroDelay(500);
            //DebugPrintF("Read I2C %ld = %x\n", i, read_i2c(dev, card, i));
        }
        
        subvendor = read_i2c(dev, card, 0) |
                    (read_i2c(dev, card, 1) << 8) |
                    (read_i2c(dev, card, 2) << 16) |
                    (read_i2c(dev, card, 3) << 24);
        
        switch (subvendor)
        {
            case MAUDIO_2496_ID: card->SubType = MAUDIO_2496;
                                 DebugPrintF("M-Audio Audiophile 2496 detected!\n");
                                 break;
                                 
            case MAUDIO_1010LT_ID: card->SubType = MAUDIO_1010LT;
                                   DebugPrintF("M-Audio Audiophile 1010LT detected!\n");
                                   break;

            case MAUDIO_DELTA44_ID: card->SubType = MAUDIO_DELTA44;
                                   DebugPrintF("M-Audio Delta 44 detected!\n");
                                   break;
                                                             
            case MAUDIO_DELTA66_ID: card->SubType = MAUDIO_DELTA66;
                                   DebugPrintF("M-Audio Delta 66 detected!\n");
                                   break;

            default: card->SubType = PHASE88;
                     DebugPrintF("Terratec Phase88?\n");
                     break;
        }
        
        size = read_i2c(dev, card, 4);
        version = read_i2c(dev, card, 5);
        
        //DebugPrintF("EEPROM size = %ld, version = %ld\n", size, version);
        size -= 6; // including bytes 0 - 5
        
        for (i = 0; i < size; i++)
        {
            eeprom[i] = read_i2c(dev, card, i + 6);
            //DebugPrintF("Read I2C %ld = %x\n", i + 6, eeprom[i]);
        }
        
        outb_config(0x60, eeprom[0], card->pci_dev); // Codecs
        outb_config(0x61, eeprom[1], card->pci_dev); // AC-link
        outb_config(0x62, eeprom[2], card->pci_dev); // I2S
        outb_config(0x63, eeprom[3], card->pci_dev); // S/PDIF
        
        WriteCCI(card, CCI_GPIO_MASK, eeprom[4]); // GPIO MASK
        WriteCCI(card, CCI_GPIO_DATA, eeprom[5]); // GPIO STATE
        WriteCCI(card, CCI_GPIO_DIR, eeprom[6]);  // GPIO DIR
        
        
    }
    
    if (card->SubType == PHASE88)
    {
        card->akm_type = AKM4524;
        Phase88_Init(card);
    }
    else if (card->SubType == MAUDIO_2496)
    {
        card->akm_type = AKM4528;
        
        card->codec[0].caddr = 2;
        card->codec[0].cif = 0;
        card->codec[0].data_mask = ICE1712_DELTA_AP_DOUT;
        card->codec[0].clk_mask = ICE1712_DELTA_AP_CCLK;
        
        card->codec[0].cs_mask = ICE1712_DELTA_AP_CS_CODEC;
        card->codec[0].cs_addr = ICE1712_DELTA_AP_CS_CODEC;
        card->codec[0].cs_none = 0;
        
        card->codec[0].add_flags = ICE1712_DELTA_AP_CS_DIGITAL;
        card->codec[0].mask_flags = 0;
        card->codec[0].type = AKM4528;
        
        Init_akm4xxx(card, AKM4528, MAUDIO_2496);
    }
    
    else if (card->SubType == MAUDIO_1010LT)
    {
        int chip;
        card->akm_type = AKM4524;

        for (chip = 0; chip < 4; chip++)
        {
            card->codec[chip].caddr = 2;
            card->codec[chip].cif = 0;
            card->codec[chip].data_mask = ICE1712_DELTA_1010LT_DOUT;
            card->codec[chip].clk_mask = ICE1712_DELTA_1010LT_CCLK;
            
            card->codec[chip].cs_mask = ICE1712_DELTA_1010LT_CS;

            if (chip == 0)
                card->codec[chip].cs_addr = ICE1712_DELTA_1010LT_CS_CHIP_A;
            if (chip == 1)
                card->codec[chip].cs_addr = ICE1712_DELTA_1010LT_CS_CHIP_B;
            if (chip == 2)
                card->codec[chip].cs_addr = ICE1712_DELTA_1010LT_CS_CHIP_C;
            if (chip == 3)
                card->codec[chip].cs_addr = ICE1712_DELTA_1010LT_CS_CHIP_D;

            card->codec[chip].cs_none = ICE1712_DELTA_1010LT_CS_NONE;
            card->codec[chip].add_flags = 0;
            card->codec[chip].mask_flags = 0;
            
            card->codec[chip].type = AKM4524;
        }
        
        Init_akm4xxx(card, AKM4524, MAUDIO_1010LT);
    }
    else if (card->SubType == MAUDIO_DELTA44 || card->SubType == MAUDIO_DELTA66)
    {
        card->akm_type = AKM4524;

        card->codec[0].caddr = 2;
        card->codec[0].cif = 0;
        card->codec[0].data_mask = 0x10;
        card->codec[0].clk_mask = 0x20;

        card->codec[0].cs_mask = 0x80; // 2nd codec
        card->codec[0].cs_addr = 0x80;
        card->codec[0].cs_none = 0;
        card->codec[0].add_flags = 0;
        card->codec[0].mask_flags = 0;

        card->codec[0].type = AKM4524;

        Init_akm4xxx(card, AKM4524, MAUDIO_DELTA44);

        card->codec[0].cs_mask = 0x40; // 1st codec
        card->codec[0].cs_addr = 0x40;
        Init_akm4xxx(card, AKM4524, MAUDIO_DELTA44);
    }
    
    for (i = 0x60; i < 0x64; i++)
        DebugPrintF("config %lx = %x\n", i, inb_config(i, card->pci_dev));
        
    for (i = 0x0; i < 0x1E;  i++)
        DebugPrintF("CCS %lx = %x\n", i, pci_inb(i, card));

    for (i = 0x0; i < 0x31; i++)
        DebugPrintF("CCI %lx = %x\n", i, ReadCCI(card, i));

    for (i = 0x0; i < 0x34; i++)
        DebugPrintF("MT %lx = %x\n", i, pci_inb_mt(i, card));

    return 0;
}


void card_cleanup(struct CardData *card)
{
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


#define CACHELINE_SIZE 4096

void *pci_alloc_consistent(size_t size, APTR *NonAlignedAddress,
  struct DriverBase* AHIsubBase)
{
  void* address;
  unsigned long a;

#ifdef __AMIGAOS4__
  if (OpenResource("newmemory.resource"))
  {
    *NonAlignedAddress =
      address = AllocVecTags(size, AVT_Type, MEMF_SHARED, AVT_Contiguous, TRUE, AVT_Lock, TRUE,
                                    AVT_PhysicalAlignment, 32, AVT_Clear, 0, TAG_DONE);
  }
  else
#endif
  {
    *NonAlignedAddress =
      address = AllocVec(size + CACHELINE_SIZE, MEMF_PUBLIC | MEMF_CLEAR);

      if( address != NULL )
      {
        a = (unsigned long) address;
        a = (a + CACHELINE_SIZE - 1) & ~(CACHELINE_SIZE - 1);
        address = (void *) a;
      }
  }

  return address;
}


void pci_free_consistent(void* addr, struct DriverBase* AHIsubBase)
{
  FreeVec( addr );
}


#ifdef __AMIGAOS4__
static ULONG ResetHandler(struct ExceptionContext *ctx, struct ExecBase *pExecBase, struct CardData *card)
{
    ClearMask8(card, MT_DMA_CONTROL, MT_PLAY_START | MT_REC_START);
    WriteMask8(card, MT_INTR_MASK_STATUS, MT_PLAY_MASK | MT_REC_MASK);


    return 0UL;
}
#endif

void AddResetHandler(struct CardData *card)
{
#ifdef __AMIGAOS4__
    static struct Interrupt interrupt;

    interrupt.is_Code = (void (*)())ResetHandler;
    interrupt.is_Data = (APTR) card;
    interrupt.is_Node.ln_Pri  = 0;
    interrupt.is_Node.ln_Type = NT_EXTINTERRUPT;
    interrupt.is_Node.ln_Name = "reset handler";

    AddResetCallback( &interrupt );
#endif
}
