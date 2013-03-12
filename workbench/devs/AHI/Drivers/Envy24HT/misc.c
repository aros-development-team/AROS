/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <proto/expansion.h>

#include <proto/dos.h>
#include "pci_wrapper.h"

#ifdef __amigaos4__
#include "library_card.h"
#elif __MORPHOS__
#include "library_mos.h"
#else
#include "library.h"
#endif
#include "regs.h"
#include "interrupt.h"
#include "misc.h"
#include "ak4114.h"
#include "DriverData.h"
#include "Revo51.h"


/* Global in Card.c */
extern const UWORD InputBits[];
extern struct DOSIFace *IDOS;
extern struct MMUIFace*            IMMU;

struct Device             *TimerBase      = NULL;
struct timerequest *TimerIO        = NULL;
struct MsgPort *replymp = NULL;
unsigned long Dirs[7] = {0x005FFFFF, 0x005FFFFF, 0x001EFFF7, 0x004000FA, 0x004000FA, 0x007FFF9F, 0x00FFFFFF};

/* Public functions in main.c */
int card_init(struct CardData *card);
void card_cleanup(struct CardData *card);
int aureon_ac97_init(struct CardData *card, unsigned long base);
void AddResetHandler(struct CardData *card);

#ifdef __MORPHOS__
extern struct DriverBase* AHIsubBase;
#endif

static unsigned char inits_ak4358[] = {
            0x01, 0x02, /* 1: reset + soft mute */

            0x00, 0x87, // I2S + unreset (used to be 0F)

            0x01, 0x01, // unreset + soft mute off
		    0x02, 0x4F, /* 2: DA's power up, normal speed, RSTN#=0 */
	    	0x03, 0x01, /* 3: de-emphasis 44.1 */

		    0x04, 0xFF, /* 4: LOUT1 volume (PCM) */
    		0x05, 0xFF, /* 5: ROUT1 volume */

	    	0x06, 0x00, /* 6: LOUT2 volume (analogue in monitor, doesn't work) */
		    0x07, 0x00, /* 7: ROUT2 volume */

    		0x08, 0xFF, /* 8: LOUT3 volume (dig out monitor, use it as analogue out) */
	    	0x09, 0xFF, /* 9: ROUT3 volume */

            0x0a, 0x00, /* a: DATT speed=0, ignore DZF */

		    0x0b, 0xFF, /* b: LOUT4 volume */
    		0x0c, 0xFF, /* c: ROUT4 volume */

	    	0x0d, 0xFF, // DFZ
		    0x0E, 0x00,
            0x0F, 0x00,
    		0xff, 0xff
	    };

void revo_i2s_mclk_changed(struct CardData *card)
{
    struct PCIDevice *dev = card->pci_dev;

	/* assert PRST# to converters; MT05 bit 7 */
	OUTBYTE(card->mtbase + MT_AC97_CMD_STATUS, INBYTE(card->mtbase + MT_AC97_CMD_STATUS) | 0x80);
	MicroDelay(5);
	/* deassert PRST# */
	OUTBYTE(card->mtbase + MT_AC97_CMD_STATUS, INBYTE(card->mtbase + MT_AC97_CMD_STATUS) & ~0x80);
}


void ClearMask8(struct CardData *card, unsigned long base, unsigned char reg, unsigned char mask)
{
    UBYTE tmp;
    
    tmp = INBYTE(base + reg);
    tmp &= ~mask;
    OUTBYTE(base + reg, tmp);
}


void WriteMask8(struct CardData *card, unsigned long base, unsigned char reg, unsigned char mask)
{
    UBYTE tmp;
    
    tmp = INBYTE(base + reg);
    tmp |= mask;
    OUTBYTE(base + reg, tmp);
}


void WritePartialMask(struct CardData *card, unsigned long base, unsigned char reg, unsigned long shift, unsigned long mask, unsigned long val)
{
    ULONG tmp;
    
    tmp = INLONG(base + reg);
    tmp &= ~(mask << shift);
    tmp |= val << shift;
    OUTLONG(base + reg, tmp);
}


void SetGPIOData(struct CardData *card, unsigned long base, unsigned long data)
{
    OUTWORD(base + CCS_GPIO_DATA, data & 0xFFFF);
    OUTBYTE(base + CCS_GPIO_DATA2, (data & (0xFF0000)) >> 16);
    INWORD(base + CCS_GPIO_DATA); /* dummy read for pci-posting */
}

void SetGPIOMask(struct CardData *card, unsigned long base, unsigned long data)
{
    OUTWORD(base + CCS_GPIO_MASK, data & 0xFFFF);
    OUTBYTE(base + CCS_GPIO_MASK2, (data & (0xFF0000)) >> 16);
    INWORD(base + CCS_GPIO_MASK); /* dummy read for pci-posting */
}


void SaveGPIO(struct PCIDevice *dev, struct CardData* card)
{
    card->SavedDir = INLONG(card->iobase + CCS_GPIO_DIR) & 0x7FFFFF;
    card->SavedMask = INWORD(card->iobase + CCS_GPIO_MASK);
}


void RestoreGPIO(struct PCIDevice *dev, struct CardData* card)
{
    OUTLONG(card->iobase + CCS_GPIO_DIR, card->SavedDir);
    OUTWORD(card->iobase + CCS_GPIO_MASK, card->SavedMask);
}


unsigned long GetGPIOData(struct CardData *card, unsigned long base)
{
    unsigned long data;
    
    data = (unsigned long) INBYTE(base + CCS_GPIO_DATA2);
    data = (data << 16) | INWORD(base + CCS_GPIO_DATA);
    return data;
}


void SetGPIODir(struct PCIDevice *dev, struct CardData* card, unsigned long data)
{
    OUTLONG(card->iobase + CCS_GPIO_DIR, data);
    INWORD(card->iobase + CCS_GPIO_DIR);
 }


unsigned char ReadI2C(struct PCIDevice *dev, struct CardData *card, unsigned char addr)
{
    unsigned char val;
    int counter = 0;
    
    MicroDelay(1);
    OUTBYTE(card->iobase + CCS_I2C_ADDR, addr);
    OUTBYTE(card->iobase + CCS_I2C_DEV_ADDRESS, 0xA0);
    
    while ((INBYTE(card->iobase + CCS_I2C_STATUS) & CCS_I2C_BUSY) && counter < 10000)
    {
        MicroDelay(1);
        counter++;
    }
    
    if (counter == 10000)
    {
        bug("Error reading from I2C\n");
    }
    
    val = INBYTE(card->iobase + CCS_I2C_DATA);
    
    return val;
}


void WriteI2C(struct PCIDevice *dev, struct CardData *card, unsigned chip_address, unsigned char reg, unsigned char data)
{
    int counter = 0;
    
    while ((INBYTE(card->iobase + CCS_I2C_STATUS) & CCS_I2C_BUSY) && counter < 10000)
    {
        MicroDelay(1);
        counter++;
    }
    
    if (counter == 10000)
    {
        bug("Error reading from I2C (for write)\n");
    }
    counter = 0;
    
    OUTBYTE(card->iobase + CCS_I2C_ADDR, reg);
    OUTBYTE(card->iobase + CCS_I2C_DATA, data);
    
    while ((INBYTE(card->iobase + CCS_I2C_STATUS) & CCS_I2C_BUSY) && counter < 10000)
    {
        counter++;
    }
    
    if (counter == 10000)
    {
        bug("Error reading from I2C (for write 2)\n");
    }
    
    OUTBYTE(card->iobase + CCS_I2C_DEV_ADDRESS, chip_address | CCS_ADDRESS_WRITE);
}


void update_spdif_bits(struct CardData *card, unsigned short val)
{
	unsigned char cbit, disabled;
    struct PCIDevice *dev = card->pci_dev;

	cbit = INBYTE(card->iobase + CCS_SPDIF_CONFIG); // get S/PDIF status
	disabled = cbit & ~CCS_SPDIF_INTEGRATED; // status without enabled bit set
    
	if (cbit != disabled) // it was enabled
		OUTBYTE(card->iobase + CCS_SPDIF_CONFIG, disabled); // so, disable it
        
	OUTWORD(card->mtbase + MT_SPDIF_TRANSMIT, val); // now we can safely write to the SPDIF control reg
    
	if (cbit != disabled)
		OUTBYTE(card->iobase + CCS_SPDIF_CONFIG, cbit); // restore
    
	OUTWORD(card->mtbase + MT_SPDIF_TRANSMIT, val); // twice???
}


void update_spdif_rate(struct CardData *card, unsigned short rate)
{
	unsigned short val, nval;
	unsigned long flags;
    struct PCIDevice *dev = card->pci_dev;

	nval = val = INWORD(card->mtbase + MT_SPDIF_TRANSMIT);
	nval &= ~(7 << 12);
	switch (rate) {
	case 44100: break;
	case 48000: nval |= 2 << 12; break;
	case 32000: nval |= 3 << 12; break;
	}
	if (val != nval)
		update_spdif_bits(card, nval);
}


static void aureon_spi_write(struct CardData *card, unsigned long base, unsigned int cs, unsigned int data, int bits)
{
    struct PCIDevice *dev = card->pci_dev;
	unsigned int tmp;
	int i;

	tmp = GetGPIOData(card, base);

    if (card->SubType == PHASE28)
    	SetGPIOMask(card, base, ~(AUREON_WM_RW|AUREON_WM_DATA|AUREON_WM_CLK|AUREON_WM_CS));
    else
        SetGPIOMask(card, base, ~(AUREON_WM_RW|AUREON_WM_DATA|AUREON_WM_CLK|AUREON_WM_CS | AUREON_CS8415_CS));
    
    SetGPIOMask(card, base, 0);
    
	tmp |= AUREON_WM_RW;
	tmp &= ~cs; 
	SetGPIOData(card, base, tmp); // set CS low
	MicroDelay(1);

   
	for (i = bits - 1; i >= 0; i--) {
		tmp &= ~AUREON_WM_CLK;
		SetGPIOData(card, base, tmp);
    	MicroDelay(1);
		if (data & (1 << i))
			tmp |= AUREON_WM_DATA;
		else
			tmp &= ~AUREON_WM_DATA;
		SetGPIOData(card, base, tmp);
    	MicroDelay(1);
		tmp |= AUREON_WM_CLK;
        SetGPIOData(card, base, tmp);
    	MicroDelay(1);
	}

	tmp &= ~AUREON_WM_CLK;
	tmp |= cs;
   SetGPIOData(card, base, tmp);
	MicroDelay(1);
	tmp |= AUREON_WM_CLK;
   SetGPIOData(card, base, tmp);
	MicroDelay(1);
}


static void aureon_ac97_write(struct CardData *card, unsigned long base, unsigned short reg, unsigned short val)
{
	unsigned int tmp;

	/* Send address to XILINX chip */
	tmp = (GetGPIOData(card, base) & ~0xFF) | (reg & 0x7F);
	SetGPIOData(card, base, tmp);
	MicroDelay(10);
	tmp |= AUREON_AC97_ADDR;
	SetGPIOData(card, base, tmp);
	MicroDelay(10);
	tmp &= ~AUREON_AC97_ADDR;
	SetGPIOData(card, base, tmp);
	MicroDelay(10);	

	/* Send low-order byte to XILINX chip */
	tmp &= ~AUREON_AC97_DATA_MASK;
	tmp |= val & AUREON_AC97_DATA_MASK;
	SetGPIOData(card, base, tmp);
	MicroDelay(10);
	tmp |= AUREON_AC97_DATA_LOW;
	SetGPIOData(card, base, tmp);
	MicroDelay(10);
	tmp &= ~AUREON_AC97_DATA_LOW;
	SetGPIOData(card, base, tmp);
	MicroDelay(10);
	
	/* Send high-order byte to XILINX chip */
	tmp &= ~AUREON_AC97_DATA_MASK;
	tmp |= (val >> 8) & AUREON_AC97_DATA_MASK;

	SetGPIOData(card, base, tmp);
	MicroDelay(10);
	tmp |= AUREON_AC97_DATA_HIGH;
	SetGPIOData(card, base, tmp);
	MicroDelay(10);
	tmp &= ~AUREON_AC97_DATA_HIGH;
	SetGPIOData(card, base, tmp);
	MicroDelay(10);
	
	/* Instruct XILINX chip to parse the data to the STAC9744 chip */
	tmp |= AUREON_AC97_COMMIT;
	SetGPIOData(card, base, tmp);
	MicroDelay(10);
	tmp &= ~AUREON_AC97_COMMIT;
	SetGPIOData(card, base, tmp);
	MicroDelay(10);	
}


int aureon_ac97_init(struct CardData *card, unsigned long base)
{
	int i;
	static unsigned short ac97_defaults[] = {
		AC97_RESET, 0x6940,
		AC97_MASTER_VOL_STEREO, 0x0101, // 0dB atten., no mute, may not exceed 0101!!!
		AC97_AUXOUT_VOL, 0x8808,
		AC97_MASTER_VOL_MONO, 0x8000,
		AC97_PHONE_VOL, 0x8008, // mute
		AC97_MIC_VOL, 0x8008,
		AC97_LINEIN_VOL, 0x8808,
		AC97_CD_VOL, 0x0808,
		AC97_VIDEO_VOL, 0x8808,
		AC97_AUX_VOL, 0x8808,
		AC97_PCMOUT_VOL, 0x8808,
        //0x1C, 0x8000,
		//0x26, 0x000F,
		//0x28, 0x0201,
		0x2C, 0xAC44,
		0x32, 0xAC44,
		//0x7C, 0x8384,
		//0x7E, 0x7644,
		(unsigned short)-1
	};
	unsigned int tmp;

	/* Cold reset */
	tmp = (GetGPIOData(card, base) | AUREON_AC97_RESET) & ~AUREON_AC97_DATA_MASK;
    
	SetGPIOData(card, base, tmp);
	MicroDelay(3);
	
	tmp &= ~AUREON_AC97_RESET;
	SetGPIOData(card, base, tmp);
	MicroDelay(3);
	
	tmp |= AUREON_AC97_RESET;
	SetGPIOData(card, base, tmp);
	MicroDelay(3);
	
	for (i=0; ac97_defaults[i] != (unsigned short)-1; i+=2)
		aureon_ac97_write(card, base, ac97_defaults[i], ac97_defaults[i+1]);

	return 0;
}




/*
 * get the current register value of WM codec
 */
static unsigned short wm_get(struct PCIDevice *dev, unsigned long base, int reg)
{
//	reg <<= 1;
//	return ((unsigned short)ice->akm[0].images[reg] << 8) | ice->akm[0].images[reg + 1];
return 0;
}

void wm_put(struct CardData *card, unsigned long base, unsigned short reg, unsigned short val)
{
	aureon_spi_write(card, base, AUREON_WM_CS, (reg << 9) | (val & 0x1ff), 16);
}



BOOL SetupTimerDevice()
{
  return TRUE;
}



void MicroDelay(unsigned int val)
{
    replymp = (struct MsgPort *) CREATEPORT( NULL, 0 );
    if( !replymp )
    {
      DEBUGPRINTF("Could not create the reply port!\n" );
      return;
    }

    TimerIO = (struct timerequest *) CREATEIOREQUEST( replymp, sizeof( struct timerequest) );

    if( TimerIO == NULL)
    {
      DEBUGPRINTF( "Out of memory.\n" );
      return;
    }
    
    if( OPENDEVICE( "timer.device", UNIT_MICROHZ, (struct IORequest *) TimerIO, 0) != 0 )
    {
      DEBUGPRINTF( "Unable to open 'timer.device'.\n" );
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
        DOIO( (struct IORequest *) TimerIO );
        CLOSEDEVICE( (struct IORequest *) TimerIO );
        DELETEIOREQUEST( (struct IORequest *) TimerIO);
        TimerIO = NULL;
    }
    
    if( replymp )
    {
      DELETEPORT(replymp);
    }
}


void CleanUpTimerDevice()
{
      
}

#ifdef __MORPHOS__
INTGW( static, void,  playbackinterrupt, PlaybackInterrupt );
INTGW( static, void,  recordinterrupt,   RecordInterrupt );
INTGW( static, ULONG, cardinterrupt,  CardInterrupt );
#endif

/******************************************************************************
** DriverData allocation ******************************************************
******************************************************************************/

// This code used to be in _AHIsub_AllocAudio(), but since we're now
// handling CAMD support too, it needs to be done at driver loading
// time.

struct CardData* AllocDriverData(struct PCIDevice *dev, struct DriverBase* AHIsubBase)
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;
  struct CardData* card;
  UWORD command_word;
  int i;

  card = ALLOCVEC( sizeof( *card ), MEMF_PUBLIC | MEMF_CLEAR );

  if( card == NULL )
  {
    Req( "Unable to allocate driver structure." );
    return NULL;
  }

  card->ahisubbase = AHIsubBase;

  card->interrupt.is_Node.ln_Type = IRQTYPE;
  card->interrupt.is_Node.ln_Pri  = 0;
  card->interrupt.is_Node.ln_Name = (STRPTR) LibName;
#ifndef __MORPHOS__
  card->interrupt.is_Code         = (void(*)(void)) CardInterrupt;
#else
  card->interrupt.is_Code         = (void(*)(void)) &cardinterrupt;
#endif
  card->interrupt.is_Data         = (APTR) card;

  card->playback_interrupt.is_Node.ln_Type = IRQTYPE;
  card->playback_interrupt.is_Node.ln_Pri  = 0;
  card->playback_interrupt.is_Node.ln_Name = (STRPTR) LibName;
#ifndef __MORPHOS__
  card->playback_interrupt.is_Code         = PlaybackInterrupt;
#else
  card->playback_interrupt.is_Code         = (void(*)(void)) &playbackinterrupt;
#endif
  card->playback_interrupt.is_Data         = (APTR) card;

  card->record_interrupt.is_Node.ln_Type = IRQTYPE;
  card->record_interrupt.is_Node.ln_Pri  = 0;
  card->record_interrupt.is_Node.ln_Name = (STRPTR) LibName;
#ifndef __MORPHOS__
  card->record_interrupt.is_Code         = RecordInterrupt;
#else
  card->record_interrupt.is_Code         = (void(*)(void)) &recordinterrupt;
#endif
  card->record_interrupt.is_Data         = (APTR) card;

  card->pci_dev = dev;

  command_word = READCONFIGWORD( PCI_COMMAND );  
  command_word |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY; // | PCI_COMMAND_MASTER;
  WRITECONFIGWORD( PCI_COMMAND, command_word );


  card->pci_master_enabled = TRUE;


  card->iobase  = ahi_pci_get_base_address(0, dev);
  card->mtbase  = ahi_pci_get_base_address(1, dev);
  card->irq     = ahi_pci_get_irq(dev);
  card->chiprev = READCONFIGBYTE( PCI_REVISION_ID);
  card->model   = READCONFIGWORD( PCI_SUBSYSTEM_ID);
  card->SavedMask = 0;

  /*DEBUGPRINTF("---> chiprev = %u, model = %x, Vendor = %x\n", READCONFIGBYTE( PCI_REVISION_ID), READCONFIGWORD( PCI_SUBSYSTEM_ID),
                     READCONFIGWORD( PCI_SUBSYSTEM_VENDOR_ID));*/



  if (SetupTimerDevice() == FALSE)
  {
    return NULL;  
  }

  /* Initialize chip */
  if( card_init( card ) < 0 )
  {
    DEBUGPRINTF("Unable to initialize Card subsystem.");
    return NULL;
  }


  //DEBUGPRINTF("INTERRUPT %lu\n", dev->MapInterrupt());
  ahi_pci_add_intserver(&card->interrupt, dev);
  card->interrupt_added = TRUE;
  
  card->card_initialized = TRUE;

  if (card->SubType == REVO51)
  {
     card->input = 1; // line in
  }
  else
  {
     card->input          = 0;
  }
  card->output         = 0;
  card->monitor_volume = 0;
  card->input_gain     = 0x10000;
  card->output_volume  = 0x10000;
  card->input_is_24bits = FALSE;
  card->playback_buffer = NULL;
  card->current_bytesize = 0;
  
  //SaveMixerState(card);

#ifdef __amigaos4__
  AddResetHandler(card);
#endif
  
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
        struct PCIDevice * dev = card->pci_dev;
        cmd = READCONFIGWORD( PCI_COMMAND );
        cmd &= ~( PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER );
        WRITECONFIGWORD( PCI_COMMAND, cmd );
      }
    }

    if( card->interrupt_added )
    {
      ahi_pci_rem_intserver(&card->interrupt, card->pci_dev);
    }

    FREEVEC( card );
  }
  
  CleanUpTimerDevice();
}


static unsigned short wm_inits[] = {
		
        0x18, 0x000,		/* All power-up */
        
		0x1b, 0x022,		/* ADC Mux (AIN1 = CD-in) */
		0x1c, 0x00B,  		/* Output1 = DAC + Aux (= ac'97 mix), output2 = DAC */
		0x1d, 0x009,		/* Output3+4 = DAC */
        
        0x16, 0x122,		/* I2S, normal polarity, 24bit */
		0x17, 0x022,		/* 256fs, slave mode */
        
		0x00, 0x17F,		/* DAC1 analog mute */
		0x01, 0x17F,		/* DAC2 analog mute */
		0x02, 0x17F,		/* DAC3 analog mute */
		0x03, 0x17F,		/* DAC4 analog mute */
		0x04, 0x7F,		/* DAC5 analog mute */
		0x05, 0x7F,		/* DAC6 analog mute */
		0x06, 0x7F,		/* DAC7 analog mute */
		0x07, 0x7F,		/* DAC8 analog mute */
		0x08, 0x17F,	/* master analog mute */
		0x09, 0x1ff,		/* DAC1 digital full */
		0x0a, 0x1ff,		/* DAC2 digital full */
		0x0b, 0xff,		/* DAC3 digital full */
		0x0c, 0xff,		/* DAC4 digital full */
		0x0d, 0xff,		/* DAC5 digital full */
		0x0e, 0xff,		/* DAC6 digital full */
		0x0f, 0xff,		/* DAC7 digital full */
		0x10, 0xff,		/* DAC8 digital full */
		0x11, 0x1ff,		/* master digital full */
		0x12, 0x000,		/* phase normal */
		0x13, 0x090,		/* unmute DAC L/R */
		0x14, 0x000,		/* all unmute (bit 5 is rec enable) */
		0x15, 0x000,		/* no deemphasis, no ZFLG */
		0x19, 0x0C,		/* 0dB gain ADC/L */
		0x1a, 0x0C		/* 0dB gain ADC/R */
	};

static unsigned short wm_inits_Phase28[] = {
		
        0x18, 0x000,		/* All power-up */
        
        0x1b, 0x000, 		/* ADC Mux (AIN1 = Line-in, no other inputs are present) */
		0x1c, 0x009,  		/* Output1 = DAC , Output2 = DAC */
        0x1d, 0x009,		/* Output3+4 = DAC */
        
        0x16, 0x122,		/* I2S, normal polarity, 24bit */
		0x17, 0x022,		/* 256fs, slave mode */
        
		0x00, 0x000,		/* DAC1 analog mute */
		0x01, 0x000,		/* DAC2 analog mute */
		0x02, 0x7F,		/* DAC3 analog mute */
		0x03, 0x7F,		/* DAC4 analog mute */
		0x04, 0x7F,		/* DAC5 analog mute */
		0x05, 0x7F,		/* DAC6 analog mute */
		0x06, 0x7F,		/* DAC7 analog mute */
		0x07, 0x7F,		/* DAC8 analog mute */
		0x08, 0x17F,	/* master analog mute */
		0x09, 0xff,		/* DAC1 digital full */
		0x0a, 0xff,		/* DAC2 digital full */
		0x0b, 0xff,		/* DAC3 digital full */
		0x0c, 0xff,		/* DAC4 digital full */
		0x0d, 0xff,		/* DAC5 digital full */
		0x0e, 0xff,		/* DAC6 digital full */
		0x0f, 0xff,		/* DAC7 digital full */
		0x10, 0xff,		/* DAC8 digital full */
		0x11, 0x1ff,		/* master digital full */
		0x12, 0x000,		/* phase normal */
		0x13, 0x090,		/* unmute DAC L/R */
		0x14, 0x000,		/* all unmute (bit 5 is rec enable) */
		0x15, 0x000,		/* no deemphasis, no ZFLG */
		0x19, 0x0C,		/* 0dB gain ADC/L */
		0x1a, 0x0C		/* 0dB gain ADC/R */
	};
    
	static unsigned short cs_inits[] = {
		0x0441, /* RUN */
		0x0180, /* no mute */
		0x0201, /* */
		0x0605, /* master, 16-bit slave, 24bit */
	};


int card_init(struct CardData *card)
{
    struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;
    unsigned short cod;
    int i;
    unsigned int tmp, eepromsize;
    unsigned char eeprom[128];

    OUTBYTE(card->mtbase + MT_AC97_CMD_STATUS, MT_AC97_RESET); // tbd
    MicroDelay(5);
    OUTBYTE(card->mtbase + MT_AC97_CMD_STATUS, 0x00); // tbd
    
    //DEBUGPRINTF("Envy24HT: card_init %lx\n", card);

    OUTBYTE(card->iobase + CCS_POWER_DOWN, 0); // power up the whole thing
    
    // reset
    OUTBYTE(card->mtbase + CCS_CTRL, CCS_RESET_ALL);
    MicroDelay(100);
    ClearMask8(card, card->mtbase, CCS_CTRL, CCS_RESET_ALL);
    MicroDelay(100);
    
    //DEBUGPRINTF("Envy24HT: reading eeprom\n");
    if ((INBYTE(card->iobase + CCS_I2C_STATUS) & CCS_I2C_EPROM) != 0)
    {
	    unsigned long subvendor = 0;
        
		subvendor = 
        		(ReadI2C(dev, card, 0x00) << 0) |
				(ReadI2C(dev, card, 0x01) << 8) | 
				(ReadI2C(dev, card, 0x02) << 16) | 
				(ReadI2C(dev, card, 0x03) << 24);
        
        switch (subvendor)
        {
            case SUBVENDOR_AUREON_SKY: card->SubType = AUREON_SKY;
                                    break;
            
            case SUBVENDOR_AUREON_SPACE: card->SubType = AUREON_SPACE;
                                    break;
                                    
            case SUBVENDOR_PHASE28: card->SubType = PHASE28;
                                    break;

            case SUBVENDOR_MAUDIO_REVOLUTION51: card->SubType = REVO51;
                                    break;

            case SUBVENDOR_MAUDIO_REVOLUTION71: card->SubType = REVO71;
                                    break;
            
            case SUBVENDOR_JULIA: card->SubType = JULIA;
                                    break;
            
            case SUBVENDOR_PHASE22: card->SubType = PHASE22;
                                    break;
            
            default:
                card->SubType = AUREON_SKY;
        }
        
        DEBUGPRINTF("subvendor = %lx, Type = %d\n", subvendor, card->SubType);
    }

    
    if (card->SubType == PHASE22)
    {
        OUTBYTE(card->iobase + CCS_SYSTEM_CONFIG, 0x28);
        OUTBYTE(card->iobase + CCS_ACLINK_CONFIG, 0x80); // AC-link
        OUTBYTE(card->iobase + CCS_I2S_FEATURES, 0x70); // I2S
        OUTBYTE(card->iobase + CCS_SPDIF_CONFIG, 0xC3); // S/PDIF
    }
    else
    {
       if (card->SubType == PHASE28)
           OUTBYTE(card->iobase + CCS_SYSTEM_CONFIG, 0x2B); // MIDI, ADC+SPDIF IN, 4 DACS
       else if (card->SubType == AUREON_SPACE)
           OUTBYTE(card->iobase + CCS_SYSTEM_CONFIG, 0x0B); // ADC+SPDIF IN, 4 DACS
       else if (card->SubType == REVO71)
           OUTBYTE(card->iobase + CCS_SYSTEM_CONFIG, 0x43); // XIN1 + ADC + 4 DACS
       else if (card->SubType == REVO51)
           OUTBYTE(card->iobase + CCS_SYSTEM_CONFIG, 0x42);
       else if (card->SubType == JULIA)
           OUTBYTE(card->iobase + CCS_SYSTEM_CONFIG, 0x20); // MIDI + ADC + DAC
       else if (card->SubType == AUREON_SKY)
           OUTBYTE(card->iobase + CCS_SYSTEM_CONFIG, 0x0A); // ADC+SPDIF IN, 3 DACS
           
       OUTBYTE(card->iobase + CCS_ACLINK_CONFIG, CCS_ACLINK_I2S); // I2S in split mode
       
       if (card->SubType == JULIA)
           OUTBYTE(card->iobase + CCS_I2S_FEATURES, CCS_I2S_96KHZ | CCS_I2S_24BIT | CCS_I2S_192KHZ);
       else
           OUTBYTE(card->iobase + CCS_I2S_FEATURES, CCS_I2S_VOLMUTE | CCS_I2S_96KHZ | CCS_I2S_24BIT | CCS_I2S_192KHZ);
       
       if (card->SubType == REVO71 || card->SubType == REVO51)
           OUTBYTE(card->iobase + CCS_SPDIF_CONFIG, CCS_SPDIF_INTEGRATED | CCS_SPDIF_INTERNAL_OUT | CCS_SPDIF_EXTERNAL_OUT);
       else
           OUTBYTE(card->iobase + CCS_SPDIF_CONFIG, CCS_SPDIF_INTEGRATED | CCS_SPDIF_INTERNAL_OUT | CCS_SPDIF_IN_PRESENT | CCS_SPDIF_EXTERNAL_OUT);
    }
    
    
    card->SavedDir = Dirs[card->SubType];
    OUTBYTE(card->mtbase + MT_INTR_MASK, MT_DMA_FIFO_MASK);
       
    if (card->SubType == REVO71)
       SetGPIOMask(card, card->iobase, 0x00BFFF85);
    else if (card->SubType == REVO51)
       SetGPIOMask(card, card->iobase, 0x00BFFF05);
    else
       SetGPIOMask(card, card->iobase, 0x00000000);

    OUTLONG(card->iobase + CCS_GPIO_DIR, Dirs[card->SubType]); // input/output
    INWORD(card->iobase + CCS_GPIO_DIR);
       
    if (card->SubType == REVO71 || card->SubType == REVO51) {
       OUTWORD(card->iobase + CCS_GPIO_DATA, 0x0072);
       OUTBYTE(card->iobase + CCS_GPIO_DATA2, 0x00);
       }
    else if (card->SubType == JULIA) {
       OUTWORD(card->iobase + CCS_GPIO_DATA, 0x3819);
       }
    else {
       OUTWORD(card->iobase + CCS_GPIO_DATA, 0x0000);
       if (card->SubType != PHASE22)
           OUTBYTE(card->iobase + CCS_GPIO_DATA2, 0x00);
       }
    INWORD(card->iobase + CCS_GPIO_DATA);
    
    //SaveGPIO(dev, card);
    
    if (card->SubType == REVO71 || card->SubType == REVO51)
       OUTBYTE(card->mtbase + MT_I2S_FORMAT, 0x08);
    else
       OUTBYTE(card->mtbase + MT_I2S_FORMAT, 0);

    if (card->SubType == AUREON_SKY || card->SubType == AUREON_SPACE || card->SubType == PHASE28)
    {
        if (card->SubType == AUREON_SKY || card->SubType == AUREON_SPACE)
        {
            aureon_ac97_init(card, card->iobase);
            SetGPIOMask(card, card->iobase, ~(AUREON_WM_RESET | AUREON_WM_CS | AUREON_CS8415_CS));
        }
        else if (card->SubType == PHASE28)
            SetGPIOMask(card, card->iobase, ~(AUREON_WM_RESET | AUREON_WM_CS));
       
   
        tmp = GetGPIOData(card, card->iobase);
        tmp &= ~AUREON_WM_RESET;
        SetGPIOData(card, card->iobase, tmp);
        MicroDelay(1);
       
        if (card->SubType != PHASE28)
            tmp |= AUREON_WM_CS | AUREON_CS8415_CS;
        else
            tmp |= AUREON_WM_CS;
   	
        SetGPIOData(card, card->iobase, tmp);
       	MicroDelay(1);
   	    tmp |= AUREON_WM_RESET;
       	SetGPIOData(card, card->iobase, tmp);
        MicroDelay(1);
       
        if (card->SubType != PHASE28)
        {
            /* initialize WM8770 codec */
       	    for (i = 0; i < 60; i += 2)
            {
       		    wm_put(card, card->iobase, wm_inits[i], wm_inits[i+1]);
            }
            
            /* initialize CS8415A codec */
    	    for (i = 0; i < 4; i++)
      		    aureon_spi_write(card, card->iobase, AUREON_CS8415_CS, cs_inits[i] | 0x200000, 24);
        }
        else
        {
            /* initialize WM8770 codec */
   	        for (i = 0; i < 60; i += 2)
            {
   	    	    wm_put(card, card->iobase, wm_inits_Phase28[i], wm_inits_Phase28[i+1]);
            }
        }
    }
    else if (card->SubType == REVO51)
    {
        card->RevoFrontCodec = ALLOCVEC(sizeof(struct akm_codec), MEMF_ANY);
        card->RevoFrontCodec->caddr = 2;
        card->RevoFrontCodec->cif = 0;
        card->RevoFrontCodec->datamask = REVO_CDOUT;
        card->RevoFrontCodec->clockmask = REVO_CCLK;
        card->RevoFrontCodec->csmask = REVO_CS0 | REVO_CS1;
        card->RevoFrontCodec->csaddr = REVO_CS1;
        card->RevoFrontCodec->csnone = REVO_CS0 | REVO_CS1;
        card->RevoFrontCodec->addflags = REVO_CCLK;
        card->RevoFrontCodec->type = AKM4358;
        card->RevoFrontCodec->totalmask = 0;
        card->RevoFrontCodec->newflag = 1;
        
        
        card->RevoSurroundCodec = NULL;

        card->RevoRecCodec = ALLOCVEC(sizeof(struct akm_codec), MEMF_ANY);
        card->RevoRecCodec->caddr = 2;
        card->RevoRecCodec->csmask = REVO_CS0 | REVO_CS1;
        card->RevoRecCodec->clockmask = REVO_CCLK;
        card->RevoRecCodec->datamask = REVO_CDOUT;
        card->RevoRecCodec->type = AKM5365;
        card->RevoRecCodec->cif = 0;
        card->RevoRecCodec->addflags = REVO_CCLK;
        card->RevoRecCodec->csaddr = REVO_CS0;
        card->RevoRecCodec->csnone = REVO_CS0 | REVO_CS1;
        card->RevoRecCodec->totalmask = 0;
        card->RevoRecCodec->newflag = 1;

        OUTBYTE(card->mtbase + MT_SAMPLERATE, 8);

        {
         unsigned int tmp = GetGPIOData(card, card->iobase);
         tmp &= ~REVO_MUTE; // mute
         SetGPIOData(card, card->iobase, tmp);
        }

        Init_akm4xxx(card, card->RevoFrontCodec);

        {
         unsigned int tmp = GetGPIOData(card, card->iobase);
         tmp |= REVO_MUTE; // unmute
         SetGPIOData(card, card->iobase, tmp);
        }

        // Has to be after mute, otherwise the mask is changed in Revo51_Init() which enables the mute mask bit...
        Revo51_Init(card); // I2C
    }
    else if (card->SubType == REVO71)
    {
        card->RevoFrontCodec = ALLOCVEC(sizeof(struct akm_codec), MEMF_ANY);
        card->RevoFrontCodec->caddr = 1;
        card->RevoFrontCodec->csmask = REVO_CS1;
        card->RevoFrontCodec->clockmask = REVO_CCLK;
        card->RevoFrontCodec->datamask = REVO_CDOUT;
        card->RevoFrontCodec->type = AKM4381;
        card->RevoFrontCodec->cif = 0;
        card->RevoFrontCodec->addflags = 0; //REVO_CCLK;?
        
        card->RevoSurroundCodec = ALLOCVEC(sizeof(struct akm_codec), MEMF_ANY);
        card->RevoSurroundCodec->caddr = 3;
        card->RevoSurroundCodec->csmask = REVO_CS2;
        card->RevoSurroundCodec->clockmask = REVO_CCLK;
        card->RevoSurroundCodec->datamask = REVO_CDOUT;
        card->RevoSurroundCodec->type = AKM4355;
        card->RevoSurroundCodec->cif = 0;
        card->RevoSurroundCodec->addflags = 0; //REVO_CCLK;?
        
        OUTBYTE(card->mtbase + MT_SAMPLERATE, 8);
        
        {
         unsigned int tmp = GetGPIOData(card, card->iobase);
         tmp &= ~REVO_MUTE; // mute
         SetGPIOData(card, card->iobase, tmp);
        }
        
        Init_akm4xxx(card, card->RevoFrontCodec);
        Init_akm4xxx(card, card->RevoSurroundCodec);
        //revo_i2s_mclk_changed(card);
        
        {
         unsigned int tmp = GetGPIOData(card, card->iobase);
         tmp |= REVO_MUTE; // unmute
         SetGPIOData(card, card->iobase, tmp);
        }
    }

    else if (card->SubType == JULIA)
    {
        unsigned char *ptr, reg, data;
        
        
        
        static unsigned char inits_ak4114[] = {
            0x00, 0x00, // power down & reset
    		0x00, 0x0F, // power on
	    	0x01, 0x70, // I2S
		    0x02, 0x80, // TX1 output enable
	    	0x03, 0x49, // 1024 LRCK + transmit data
		    0x04, 0x00, // no mask
    		0x05, 0x00, // no mask
	    	0x0D, 0x41, // 
		    0x0E, 0x02,
    		0x0F, 0x2C,
	    	0x10, 0x00,
		    0x11, 0x00,
    		0xff, 0xff
	    };
        
        ptr = inits_ak4358;
		while (*ptr != 0xff) {
			reg = *ptr++;
			data = *ptr++;
			WriteI2C(dev, card, AK4358_ADDR, reg, data);
            MicroDelay(5);
		}
        
        ptr = inits_ak4114;
		while (*ptr != 0xff) {
			reg = *ptr++;
			data = *ptr++;
			WriteI2C(dev, card, AK4114_ADDR, reg, data);
            MicroDelay(100);
            }
        
        OUTBYTE(card->mtbase + MT_SAMPLERATE, MT_SPDIF_MASTER);
        OUTLONG(card->mtbase + 0x2C, 0x300200); // route
    }
    else if (card->SubType == PHASE22)
    {
        unsigned int tmp;
        
        card->RevoFrontCodec = ALLOCVEC(sizeof(struct akm_codec), MEMF_ANY);
        card->RevoFrontCodec->caddr = 2;
        card->RevoFrontCodec->csmask = 1 << 10;
        card->RevoFrontCodec->clockmask = 1 << 5;
        card->RevoFrontCodec->datamask = 1 << 4;
        card->RevoFrontCodec->type = AKM4524;
        card->RevoFrontCodec->cif = 1;
        card->RevoFrontCodec->addflags = 1 << 3;

        Init_akm4xxx(card, card->RevoFrontCodec);
    }

    //RestoreGPIO(dev, card);
    
    ClearMask8(card, card->iobase, CCS_INTR_MASK, CCS_INTR_PLAYREC); // enable

    // Enter SPI mode for CS8415A digital receiver
    /*SetGPIOMask(card, card->iobase, ~(AUREON_CS8415_CS));
	 tmp |= AUREON_CS8415_CS;
	 SetGPIOData(card, card->iobase, tmp); // set CS high
	 MicroDelay(1);
    
	 tmp &= ~AUREON_CS8415_CS;
	 SetGPIOData(card, card->iobase, tmp); // set CS low
	 MicroDelay(1);*/

    // WritePartialMask(card, card->mtbase, 0x2C, 8, 7, 6); // this line is to route the s/pdif input to the left
    // analogue output for testing purposes

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
}


void
RestoreMixerState( struct CardData* card )
{
}

void
UpdateMonitorMixer( struct CardData* card )
{
}


Fixed
Linear2MixerGain( Fixed  linear,
		  UWORD* bits )
{
  static const Fixed gain[ 256 ] =
  {
	0x0, 	// -127.5 dB
	0x0, 	// -127.0 dB
	0x0, 	// -126.5 dB
	0x0, 	// -126.0 dB
	0x0, 	// -125.5 dB
	0x0, 	// -125.0 dB
	0x0, 	// -124.5 dB
	0x0, 	// -124.0 dB
	0x0, 	// -123.5 dB
	0x0, 	// -123.0 dB
	0x0, 	// -122.5 dB
	0x0, 	// -122.0 dB
	0x0, 	// -121.5 dB
	0x0, 	// -121.0 dB
	0x0, 	// -120.5 dB
	0x0, 	// -120.0 dB
	0x0, 	// -119.5 dB
	0x0, 	// -119.0 dB
	0x0, 	// -118.5 dB
	0x0, 	// -118.0 dB
	0x0, 	// -117.5 dB
	0x0, 	// -117.0 dB
	0x0, 	// -116.5 dB
	0x0, 	// -116.0 dB
	0x0, 	// -115.5 dB
	0x0, 	// -115.0 dB
	0x0, 	// -114.5 dB
	0x0, 	// -114.0 dB
	0x0, 	// -113.5 dB
	0x0, 	// -113.0 dB
	0x0, 	// -112.5 dB
	0x0, 	// -112.0 dB
	0x0, 	// -111.5 dB
	0x0, 	// -111.0 dB
	0x0, 	// -110.5 dB
	0x0, 	// -110.0 dB
	0x0, 	// -109.5 dB
	0x0, 	// -109.0 dB
	0x0, 	// -108.5 dB
	0x0, 	// -108.0 dB
	0x0, 	// -107.5 dB
	0x0, 	// -107.0 dB
	0x0, 	// -106.5 dB
	0x0, 	// -106.0 dB
	0x0, 	// -105.5 dB
	0x0, 	// -105.0 dB
	0x0, 	// -104.5 dB
	0x0, 	// -104.0 dB
	0x0, 	// -103.5 dB
	0x0, 	// -103.0 dB
	0x0, 	// -102.5 dB
	0x0, 	// -102.0 dB
	0x0, 	// -101.5 dB
	0x0, 	// -101.0 dB
	0x0, 	// -100.5 dB
	0x0, 	// -100.0 dB
	0x0, 	// -99.5 dB
	0x0, 	// -99.0 dB
	0x0, 	// -98.5 dB
	0x0, 	// -98.0 dB
	0x0, 	// -97.5 dB
	0x0, 	// -97.0 dB
	0x0, 	// -96.5 dB
	0x1, 	// -96.0 dB
	0x1, 	// -95.5 dB
	0x1, 	// -95.0 dB
	0x1, 	// -94.5 dB
	0x1, 	// -94.0 dB
	0x1, 	// -93.5 dB
	0x1, 	// -93.0 dB
	0x1, 	// -92.5 dB
	0x1, 	// -92.0 dB
	0x1, 	// -91.5 dB
	0x1, 	// -91.0 dB
	0x1, 	// -90.5 dB
	0x2, 	// -90.0 dB
	0x2, 	// -89.5 dB
	0x2, 	// -89.0 dB
	0x2, 	// -88.5 dB
	0x2, 	// -88.0 dB
	0x2, 	// -87.5 dB
	0x2, 	// -87.0 dB
	0x3, 	// -86.5 dB
	0x3, 	// -86.0 dB
	0x3, 	// -85.5 dB
	0x3, 	// -85.0 dB
	0x3, 	// -84.5 dB
	0x4, 	// -84.0 dB
	0x4, 	// -83.5 dB
	0x4, 	// -83.0 dB
	0x4, 	// -82.5 dB
	0x5, 	// -82.0 dB
	0x5, 	// -81.5 dB
	0x5, 	// -81.0 dB
	0x6, 	// -80.5 dB
	0x6, 	// -80.0 dB
	0x6, 	// -79.5 dB
	0x7, 	// -79.0 dB
	0x7, 	// -78.5 dB
	0x8, 	// -78.0 dB
	0x8, 	// -77.5 dB
	0x9, 	// -77.0 dB
	0x9, 	// -76.5 dB
	0xa, 	// -76.0 dB
	0xb, 	// -75.5 dB
	0xb, 	// -75.0 dB
	0xc, 	// -74.5 dB
	0xd, 	// -74.0 dB
	0xd, 	// -73.5 dB
	0xe, 	// -73.0 dB
	0xf, 	// -72.5 dB
	0x10, 	// -72.0 dB
	0x11, 	// -71.5 dB
	0x12, 	// -71.0 dB
	0x13, 	// -70.5 dB
	0x14, 	// -70.0 dB
	0x15, 	// -69.5 dB
	0x17, 	// -69.0 dB
	0x18, 	// -68.5 dB
	0x1a, 	// -68.0 dB
	0x1b, 	// -67.5 dB
	0x1d, 	// -67.0 dB
	0x1f, 	// -66.5 dB
	0x20, 	// -66.0 dB
	0x22, 	// -65.5 dB
	0x24, 	// -65.0 dB
	0x27, 	// -64.5 dB
	0x29, 	// -64.0 dB
	0x2b, 	// -63.5 dB
	0x2e, 	// -63.0 dB
	0x31, 	// -62.5 dB
	0x34, 	// -62.0 dB
	0x37, 	// -61.5 dB
	0x3a, 	// -61.0 dB
	0x3d, 	// -60.5 dB
	0x41, 	// -60.0 dB
	0x45, 	// -59.5 dB
	0x49, 	// -59.0 dB
	0x4d, 	// -58.5 dB
	0x52, 	// -58.0 dB
	0x57, 	// -57.5 dB
	0x5c, 	// -57.0 dB
	0x62, 	// -56.5 dB
	0x67, 	// -56.0 dB
	0x6e, 	// -55.5 dB
	0x74, 	// -55.0 dB
	0x7b, 	// -54.5 dB
	0x82, 	// -54.0 dB
	0x8a, 	// -53.5 dB
	0x92, 	// -53.0 dB
	0x9b, 	// -52.5 dB
	0xa4, 	// -52.0 dB
	0xae, 	// -51.5 dB
	0xb8, 	// -51.0 dB
	0xc3, 	// -50.5 dB
	0xcf, 	// -50.0 dB
	0xdb, 	// -49.5 dB
	0xe8, 	// -49.0 dB
	0xf6, 	// -48.5 dB
	0x104, 	// -48.0 dB
	0x114, 	// -47.5 dB
	0x124, 	// -47.0 dB
	0x136, 	// -46.5 dB
	0x148, 	// -46.0 dB
	0x15b, 	// -45.5 dB
	0x170, 	// -45.0 dB
	0x186, 	// -44.5 dB
	0x19d, 	// -44.0 dB
	0x1b6, 	// -43.5 dB
	0x1cf, 	// -43.0 dB
	0x1eb, 	// -42.5 dB
	0x208, 	// -42.0 dB
	0x227, 	// -41.5 dB
	0x248, 	// -41.0 dB
	0x26a, 	// -40.5 dB
	0x28f, 	// -40.0 dB
	0x2b6, 	// -39.5 dB
	0x2df, 	// -39.0 dB
	0x30a, 	// -38.5 dB
	0x339, 	// -38.0 dB
	0x369, 	// -37.5 dB
	0x39d, 	// -37.0 dB
	0x3d4, 	// -36.5 dB
	0x40e, 	// -36.0 dB
	0x44c, 	// -35.5 dB
	0x48d, 	// -35.0 dB
	0x4d2, 	// -34.5 dB
	0x51b, 	// -34.0 dB
	0x569, 	// -33.5 dB
	0x5bb, 	// -33.0 dB
	0x612, 	// -32.5 dB
	0x66e, 	// -32.0 dB
	0x6cf, 	// -31.5 dB
	0x737, 	// -31.0 dB
	0x7a4, 	// -30.5 dB
	0x818, 	// -30.0 dB
	0x893, 	// -29.5 dB
	0x915, 	// -29.0 dB
	0x99f, 	// -28.5 dB
	0xa31, 	// -28.0 dB
	0xacb, 	// -27.5 dB
	0xb6f, 	// -27.0 dB
	0xc1c, 	// -26.5 dB
	0xcd4, 	// -26.0 dB
	0xd97, 	// -25.5 dB
	0xe65, 	// -25.0 dB
	0xf3f, 	// -24.5 dB
	0x1027, 	// -24.0 dB
	0x111c, 	// -23.5 dB
	0x121f, 	// -23.0 dB
	0x1332, 	// -22.5 dB
	0x1455, 	// -22.0 dB
	0x158a, 	// -21.5 dB
	0x16d0, 	// -21.0 dB
	0x182a, 	// -20.5 dB
	0x1999, 	// -20.0 dB
	0x1b1d, 	// -19.5 dB
	0x1cb9, 	// -19.0 dB
	0x1e6c, 	// -18.5 dB
	0x203a, 	// -18.0 dB
	0x2223, 	// -17.5 dB
	0x2429, 	// -17.0 dB
	0x264d, 	// -16.5 dB
	0x2892, 	// -16.0 dB
	0x2afa, 	// -15.5 dB
	0x2d86, 	// -15.0 dB
	0x3038, 	// -14.5 dB
	0x3314, 	// -14.0 dB
	0x361a, 	// -13.5 dB
	0x394f, 	// -13.0 dB
	0x3cb5, 	// -12.5 dB
	0x404d, 	// -12.0 dB
	0x441d, 	// -11.5 dB
	0x4826, 	// -11.0 dB
	0x4c6d, 	// -10.5 dB
	0x50f4, 	// -10.0 dB
	0x55c0, 	// -9.5 dB
	0x5ad5, 	// -9.0 dB
	0x6036, 	// -8.5 dB
	0x65ea, 	// -8.0 dB
	0x6bf4, 	// -7.5 dB
	0x7259, 	// -7.0 dB
	0x7920, 	// -6.5 dB
	0x804d, 	// -6.0 dB
	0x87e8, 	// -5.5 dB
	0x8ff5, 	// -5.0 dB
	0x987d, 	// -4.5 dB
	0xa186, 	// -4.0 dB
	0xab18, 	// -3.5 dB
	0xb53b, 	// -3.0 dB
	0xbff9, 	// -2.5 dB
	0xcb59, 	// -2.0 dB
	0xd765, 	// -1.5 dB
	0xe429, 	// -1.0 dB
	0xf1ad, 	// -0.5 dB
	0x10000, 	// 0.0 dB
  };

  int v = 0;

  while( linear > gain[ v ] && v < 255)
  {
    ++v;
  }

  *bits = v;

  return gain[ v ];
}


Fixed
Linear2AKMGain( Fixed  linear,
		  UWORD* bits )
{
  static const Fixed gain[ 101 ] =
  {
0x0, 	// -100 dB
0x0, 	// -99 dB
0x0, 	// -98 dB
0x0, 	// -97 dB
0x1, 	// -96 dB
0x1, 	// -95 dB
0x1, 	// -94 dB
0x1, 	// -93 dB
0x1, 	// -92 dB
0x1, 	// -91 dB
0x2, 	// -90 dB
0x2, 	// -89 dB
0x2, 	// -88 dB
0x2, 	// -87 dB
0x3, 	// -86 dB
0x3, 	// -85 dB
0x4, 	// -84 dB
0x4, 	// -83 dB
0x5, 	// -82 dB
0x5, 	// -81 dB
0x6, 	// -80 dB
0x7, 	// -79 dB
0x8, 	// -78 dB
0x9, 	// -77 dB
0xa, 	// -76 dB
0xb, 	// -75 dB
0xd, 	// -74 dB
0xe, 	// -73 dB
0x10, 	// -72 dB
0x12, 	// -71 dB
0x14, 	// -70 dB
0x17, 	// -69 dB
0x1a, 	// -68 dB
0x1d, 	// -67 dB
0x20, 	// -66 dB
0x24, 	// -65 dB
0x29, 	// -64 dB
0x2e, 	// -63 dB
0x34, 	// -62 dB
0x3a, 	// -61 dB
0x41, 	// -60 dB
0x49, 	// -59 dB
0x52, 	// -58 dB
0x5c, 	// -57 dB
0x67, 	// -56 dB
0x74, 	// -55 dB
0x82, 	// -54 dB
0x92, 	// -53 dB
0xa4, 	// -52 dB
0xb8, 	// -51 dB
0xcf, 	// -50 dB
0xe8, 	// -49 dB
0x104, 	// -48 dB
0x124, 	// -47 dB
0x148, 	// -46 dB
0x170, 	// -45 dB
0x19d, 	// -44 dB
0x1cf, 	// -43 dB
0x208, 	// -42 dB
0x248, 	// -41 dB
0x28f, 	// -40 dB
0x2df, 	// -39 dB
0x339, 	// -38 dB
0x39d, 	// -37 dB
0x40e, 	// -36 dB
0x48d, 	// -35 dB
0x51b, 	// -34 dB
0x5bb, 	// -33 dB
0x66e, 	// -32 dB
0x737, 	// -31 dB
0x818, 	// -30 dB
0x915, 	// -29 dB
0xa31, 	// -28 dB
0xb6f, 	// -27 dB
0xcd4, 	// -26 dB
0xe65, 	// -25 dB
0x1027, 	// -24 dB
0x121f, 	// -23 dB
0x1455, 	// -22 dB
0x16d0, 	// -21 dB
0x1999, 	// -20 dB
0x1cb9, 	// -19 dB
0x203a, 	// -18 dB
0x2429, 	// -17 dB
0x2892, 	// -16 dB
0x2d86, 	// -15 dB
0x3314, 	// -14 dB
0x394f, 	// -13 dB
0x404d, 	// -12 dB
0x4826, 	// -11 dB
0x50f4, 	// -10 dB
0x5ad5, 	// -9 dB
0x65ea, 	// -8 dB
0x7259, 	// -7 dB
0x804d, 	// -6 dB
0x8ff5, 	// -5 dB
0xa186, 	// -4 dB
0xb53b, 	// -3 dB
0xcb59, 	// -2 dB
0xe429, 	// -1 dB
0x10000, 	// 0 dB

  };

  int v = 0;

  while( linear > gain[ v ] && v < 100)
  {
    ++v;
  }

  *bits = v + 0x9B;

  return gain[ v ];
}



Fixed
Linear2RecordGain( Fixed  linear,
		   UWORD* bits )
{
  static const Fixed gain[ 32 ] =
  {
0x404d, 	// -12 dB
0x4826, 	// -11 dB
0x50f4, 	// -10 dB
0x5ad5, 	// -9 dB
0x65ea, 	// -8 dB
0x7259, 	// -7 dB
0x804d, 	// -6 dB
0x8ff5, 	// -5 dB
0xa186, 	// -4 dB
0xb53b, 	// -3 dB
0xcb59, 	// -2 dB
0xe429, 	// -1 dB
0x10000, 	// 0 dB
0x11f3c, 	// 1 dB
0x14248, 	// 2 dB
0x1699c, 	// 3 dB
0x195bb, 	// 4 dB
0x1c73d, 	// 5 dB
0x1fec9, 	// 6 dB
0x23d1c, 	// 7 dB
0x2830a, 	// 8 dB
0x2d181, 	// 9 dB
0x3298b, 	// 10 dB
0x38c52, 	// 11 dB
0x3fb27, 	// 12 dB
0x47782, 	// 13 dB
0x5030a, 	// 14 dB
0x59f98, 	// 15 dB
0x64f40, 	// 16 dB
0x71457, 	// 17 dB
0x7f17a, 	// 18 dB
0x8e99a, 	// 19 dB
  };

  int v = 0;

  while( linear > gain[ v ] && v < 32)
  {
    ++v;
  }

  *bits = v;

  return gain[ v ];
}


ULONG
SamplerateToLinearPitch( ULONG samplingrate )
{
  samplingrate = (samplingrate << 8) / 375;
  return (samplingrate >> 1) + (samplingrate & 1);
}


#define CACHELINE_SIZE 4096

void *pci_alloc_consistent(size_t size, APTR *NonAlignedAddress, unsigned int boundary)
{
#ifdef __amigaos4__
  void* address;
  unsigned long a;

  if (IExec->OpenResource("newmemory.resource"))
  {
      address = ALLOCVECTags(size, AVT_Type, MEMF_SHARED, AVT_Contiguous, TRUE, AVT_Lock, TRUE,
                                    AVT_PhysicalAlignment, 32, AVT_Clear, 0, TAG_DONE);
  }
  else
  {
      address = ALLOCVEC( size + CACHELINE_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
    
      if( address != NULL )
      {
        a = (unsigned long) address;
        a = (a + CACHELINE_SIZE - 1) & ~(CACHELINE_SIZE - 1);
        address = (void *) a;
      }
  }
  
  *NonAlignedAddress = address;

  return address;
#else
    void* address;
    unsigned long a;

#ifdef __MORPHOS__
    //address = AllocVecDMA(size + boundary, MEMF_PUBLIC | MEMF_CLEAR);
    #if 0
    address = AllocVec(size + CACHELINE_SIZE - 1, MEMF_PUBLIC & ~MEMF_CLEAR);
    Forbid();
    FreeMem(address, size + CACHELINE_SIZE - 1);
    address = AllocAbs(size, (void*) ((ULONG) (address + CACHELINE_SIZE - 1) & ~(CACHELINE_SIZE-1) ) );
    Permit();
    #endif
    address = AllocVecDMA(size, MEMF_PUBLIC | MEMF_CLEAR);
    *NonAlignedAddress = address;

    memset(address, 0, size);

    return address;

#else
    address = AllocVec(size + boundary, MEMF_PUBLIC | MEMF_CLEAR);

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
#endif
#endif
}


void pci_free_consistent(void* addr)
{
#ifdef __MORPHOS__
  FreeVecDMA(addr);
#else
  FREEVEC( addr );
#endif
}

#ifdef __amigaos4__
static ULONG ResetHandler(struct ExceptionContext *ctx, struct ExecBase *pExecBase, struct CardData *card)
{
    struct PCIDevice *dev = card->pci_dev;
    
    ClearMask8(card, card->mtbase, MT_DMA_CONTROL, MT_PDMA0_START | MT_PDMA4_START | MT_RDMA0_MASK | MT_RDMA1_MASK);
    WriteMask8(card, card->mtbase, MT_INTR_MASK, MT_DMA_FIFO_MASK | MT_PDMA0_MASK | MT_RDMA0_MASK | MT_RDMA1_MASK);
    
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
#endif



#if 0
    if ((INBYTE(card->iobase + CCS_I2C_STATUS) & CCS_I2C_EPROM) == 0)
    {
        DEBUGPRINTF("No EEPROM found!\n");
    }
    else
    {
        DEBUGPRINTF("EEPROM found!\n");
    }
    
    
    /*OUTBYTE(card->iobase + CCS_I2C_DATA, 0x11);
    OUTBYTE(card->iobase + CCS_I2C_ADDR, 0);
    
    while (INBYTE(card->iobase + CCS_I2C_STATUS) & CCS_I2C_BUSY)
    {
    }
    OUTBYTE(card->iobase + CCS_I2C_DEV_ADDRESS, 0xA0);
    DELAY(1);*/
    
    for (i = 0; i < 6; i++)
    {
        OUTBYTE(card->iobase + CCS_I2C_ADDR, i);
        
        while (INBYTE(card->iobase + CCS_I2C_STATUS) & CCS_I2C_BUSY)
        {
        }      
        
        OUTBYTE(card->iobase + CCS_I2C_DEV_ADDRESS, 0xA0);
        
        while (INBYTE(card->iobase + CCS_I2C_STATUS) & CCS_I2C_BUSY)
        {
        }
             
        
        DEBUGPRINTF("%d = %x\n",i, INBYTE(card->iobase + CCS_I2C_DATA));
        DELAY(1);
    }
#endif
