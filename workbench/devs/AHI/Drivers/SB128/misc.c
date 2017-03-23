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

#include <exec/memory.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include <proto/exec.h>
#include <proto/dos.h>

#include "library.h"
#include "regs.h"
#include "interrupt.h"
#include "misc.h"

#include "pci_wrapper.h"

#ifdef __AROS__
#define DebugPrintF bug
INTGW(static, void,  playbackinterrupt, PlaybackInterrupt);
INTGW(static, void,  recordinterrupt,   RecordInterrupt);
INTGW(static, ULONG, cardinterrupt,  CardInterrupt);
#endif

/* Global in Card.c */
extern const UWORD InputBits[];

/* Public functions in main.c */
int card_init(struct SB128_DATA *card);
void card_cleanup(struct SB128_DATA *card);

#if !defined(__AROS__)
void AddResetHandler(struct SB128_DATA *card);
#endif

void micro_delay(unsigned int val)
{
  struct timerequest*         TimerIO = NULL;
  struct MsgPort *            replymp;

  replymp = (struct MsgPort *) CreateMsgPort();
  if (!replymp)
  {
    DebugPrintF("SB128: Couldn't create reply port\n");
    return;
  }

  TimerIO = (struct timerequest *)CreateIORequest(replymp, sizeof(struct timerequest));

  if (TimerIO == NULL)
  {
    DebugPrintF("SB128: Out of memory.\n");
    return;
  }

  if (OpenDevice((CONST_STRPTR) "timer.device", UNIT_MICROHZ, (struct IORequest *)TimerIO, 0) != 0)
  {
    DebugPrintF("SB128: Unable to open 'timer.device'.\n");
    return;
  }
  
  TimerIO->tr_node.io_Command = TR_ADDREQUEST;
  TimerIO->tr_time.tv_secs = 0;
  TimerIO->tr_time.tv_micro = val;
  DoIO((struct IORequest *)TimerIO);
  CloseDevice((struct IORequest *)TimerIO);
  DeleteIORequest((struct IORequest *)TimerIO);
  TimerIO = NULL;

  if (replymp)
  {
    DeleteMsgPort(replymp);
  }
}

unsigned long src_ready(struct SB128_DATA *card)
{
  unsigned int i;
  unsigned long r;

  /* Wait for the busy bit to become invalid, and then return the contents
     of the SRC register. */
  for (i = 0; i < 0x1000; i++) 
  {
    if (!((r = pci_inl(SB128_SRC, card)) & SRC_BUSY))
        return r;
    //micro_delay(1);
  }  

  DebugPrintF("SB128: SRC Ready timeout.\n");
  return 0;
}

void src_write(struct SB128_DATA *card, unsigned short addr, unsigned short data)
{
  unsigned long r;

//  ObtainSemaphore(&card->sb128_semaphore);
  
  /* Get copy of SRC register when it's not busy, add address and data, write back. */
  r = src_ready(card) & (SRC_DISABLE | SRC_DIS_DAC2 | SRC_DIS_ADC);
  r = r | (addr << SRC_ADDR_SHIFT);
  r = r | data;
  pci_outl(r | SRC_WE, SB128_SRC, card);
  //micro_delay(1);

//  ReleaseSemaphore(&card->sb128_semaphore);
}

unsigned short src_read(struct SB128_DATA *card, unsigned short addr)
{
  //There may be ES137x bugs that require accomodating in this section.
  
  unsigned long r;

//  ObtainSemaphore(&card->sb128_semaphore);
  
  /* Get copy of SRC register when it's not busy, add address, write back,
     wait for ready, then read back value. */
  r = src_ready(card) & (SRC_DISABLE | SRC_DIS_DAC2 | SRC_DIS_ADC);
  r = r | (addr << SRC_ADDR_SHIFT);
  pci_outl(r, SB128_SRC, card);
  
  /* Give the chip a chance to set the busy bit. */
  //micro_delay(1);
  
//  ReleaseSemaphore(&card->sb128_semaphore);
  
  return (src_ready(card) & 0xFFFF);
} 

/* Translate AC97 commands to AK4531 commands, and write to the AK4531 codec */
void ak4531_ac97_write(struct SB128_DATA *card, unsigned short reg, unsigned short val)
{
  char ak4531_L1 = 0;
  char ak4531_R1 = 0;
  char ak4531_L2 = 0;
  char ak4531_R2 = 0;
  
  char input_right = 0;
  char input_left  = 0;

  short left_vol = 0;
  short right_vol = 0;

  float steps = 0;

  if (reg == AC97_RECORD_SELECT)
  {
    /* Change input select settings */
    input_right = val;
    input_left = val >> 8;
   
    /* Translate as appropriate */
    switch (input_left) {
      
      case 0:
        /* Mic */
        ak4531_L2 = 0x80;
        break;

      case 1:
        /* CD */
        ak4531_L1 = 0x04;
        break;

      case 2:
        /* Video */
        break;

      case 3:
        /* Aux */
        ak4531_L2 = 0x10;
        break;

      case 4:
        /* Line in */
        ak4531_L1 = 0x10;
        break;

      case 5:
        /* Stereo Mix (all) */
        ak4531_L1 = 0x14;
        ak4531_L2 = 0xF4;
        break;

      case 6:
        /* Mono Mix */
        break;

      case 7:
        /* Phone */
        ak4531_L2 = 0x60;
        break;

      default:
        /* Shouldn't happen */
        DebugPrintF("SB128: Unsupported Record Input command\n");
    }

    switch (input_right) {
       
      case 0:
        /* Mic */
        ak4531_R2 = 0x80;
        break;

      case 1:
        /* CD */
        ak4531_R1 = 0x02;
        break;

      case 2:
        /* Video */
        break;

      case 3:
        /* Aux */
        ak4531_R2 = 0x08;
        break;

      case 4:
        /* Line in */
        ak4531_R1 = 0x08;
        break;

      case 5:
        /* Stereo Mix (all) */
        ak4531_R1 = 0x0A;
        ak4531_R2 = 0xEC;
        break;

      case 6:
        /* Mono Mix */
        break;

      case 7:
        /* Phone */
        ak4531_R2 = 0x60;
        break;

      default:
        /* Shouldn't happen */
        DebugPrintF("SB128: Unsupported Record Input command\n");
    }
    
    /* Write input values to AK4531 */
    
    codec_write(card, AK4531_INPUT_MUX_L_1, ak4531_L1);
    codec_write(card, AK4531_INPUT_MUX_R_1, ak4531_R1);
    codec_write(card, AK4531_INPUT_MUX_L_2, ak4531_L2);
    codec_write(card, AK4531_INPUT_MUX_R_2, ak4531_R2);
    return;
  }
 
  if (reg == AC97_PHONE_VOL || AC97_MIC_VOL || AC97_LINEIN_VOL || AC97_CD_VOL || AC97_AUX_VOL || AC97_PCMOUT_VOL)
  {
    /* Adjust PCM (from card) Input gain */
    if (val & AC97_MUTE)
    {
      /* Using a muted volume */
      right_vol = (AK4531_MUTE | 0x6);
      left_vol  = (AK4531_MUTE | 0x6);
    }
    else
    {
      /* Strip bits */
      right_vol = (val & 0x1F);
      left_vol  = (val >> 8);
      
      /* Convert right volume */
      if (right_vol < 0x8)
      {
        steps = 0x8 - right_vol;
        steps = steps * 1.5;
        steps = steps / 2;
        right_vol = (int) (steps + 0.5);
        right_vol = 0x6 - right_vol;
      }
      else if (right_vol > 0x8)
      {
        steps = right_vol - 0x8;
        steps = steps * 1.5;
        steps = steps / 2;
        right_vol = (int) (steps + 0.5);
        right_vol = 0x6 + right_vol;
      }
      else if (right_vol == 0x8)
      {
       /* No attentuation, no mute */
        right_vol = 0x6;  
      }

      /* Convert left volume */
      if (left_vol < 0x8)
      {
        steps = 0x8 - left_vol;
        steps = steps * 1.5;
        steps = steps / 2;
        left_vol = (int) (steps + 0.5);
        left_vol = 0x6 - left_vol;
      }
      else if (left_vol > 0x8)
      {
        steps = left_vol - 0x8;
        steps = steps * 1.5;
        steps = steps / 2;
        left_vol = (int) (steps + 0.5);
        left_vol = 0x6 + left_vol;
      }
      else if (left_vol == 0x8)
      {
        /* No attentuation, no mute */
        left_vol = 0x6;
      }
      
    }
    
    /* Write adjusted volume to appropriate place */
    /* Un-mute, and disable output, if an input muted */
    switch (reg) {

      case AC97_PHONE_VOL:
        codec_write(card, AK4531_PHONE_VOL_L, right_vol);
        codec_write(card, AK4531_PHONE_VOL_R, right_vol);
        break;

      case AC97_MIC_VOL:
        codec_write(card, AK4531_MIC_VOL, right_vol);
        break;
 
      case AC97_LINEIN_VOL:
        if ((left_vol & AK4531_MUTE) && (right_vol & AK4531_MUTE))
        {
          left_vol = 0x6;
          right_vol = 0x6;
          /* Disable on OUTPUT mux */
          card->ak4531_output_1 = (card->ak4531_output_1 & ~(AK4531_OUTPUT_LINE));
          codec_write(card, AK4531_OUTPUT_MUX_1, card->ak4531_output_1);
        }
        else
        {
          codec_write(card, AK4531_LINEIN_VOL_L, left_vol);
          codec_write(card, AK4531_LINEIN_VOL_R, right_vol);
          /* Re-enable on OUTPUT mux */
          card->ak4531_output_1 = (card->ak4531_output_1 | AK4531_OUTPUT_LINE);
          codec_write(card, AK4531_OUTPUT_MUX_1, card->ak4531_output_1);
        }
        break;

      case AC97_CD_VOL:
         if ((left_vol & AK4531_MUTE) && (right_vol & AK4531_MUTE))
        {
          left_vol = 0x6;
          right_vol = 0x6;
          /* Disable on OUTPUT mux */
          card->ak4531_output_1 = (card->ak4531_output_1 & ~(AK4531_OUTPUT_CD));
          codec_write(card, AK4531_OUTPUT_MUX_1, card->ak4531_output_1);
        }
        else
        {
          codec_write(card, AK4531_CD_VOL_L, left_vol);
          codec_write(card, AK4531_CD_VOL_R, right_vol);
          /* Re-enable on OUTPUT mux */
          card->ak4531_output_1 = (card->ak4531_output_1 | AK4531_OUTPUT_CD);
          codec_write(card, AK4531_OUTPUT_MUX_1, card->ak4531_output_1);
        }
        break;
          
      case AC97_AUX_VOL:
          if ((left_vol & AK4531_MUTE) && (right_vol & AK4531_MUTE))
        {
          left_vol = 0x6;
          right_vol = 0x6;
          /* Disable on OUTPUT mux */
          card->ak4531_output_2 = (card->ak4531_output_2 & ~(AK4531_OUTPUT_AUX));
          codec_write(card, AK4531_OUTPUT_MUX_2, card->ak4531_output_2);
        }
        else
        {
          codec_write(card, AK4531_AUX_VOL_L, left_vol);
          codec_write(card, AK4531_AUX_VOL_R, right_vol);
          /* Re-enable on OUTPUT mux */
          card->ak4531_output_2 = (card->ak4531_output_2 | AK4531_OUTPUT_AUX);
          codec_write(card, AK4531_OUTPUT_MUX_2, card->ak4531_output_2);
        }
        break;

      case AC97_PCMOUT_VOL:
        codec_write(card, AK4531_PCMOUT_VOL_L, left_vol);
        codec_write(card, AK4531_PCMOUT_VOL_R, right_vol);
        break;

      default:
        DebugPrintF("SB128: Invalid value for Volume Set\n");
    }
    
    return;

  }

}

void codec_write(struct SB128_DATA *card, unsigned short reg, unsigned short val)
{
  unsigned long i, r;

  /* Take hold of the hardware semaphore */
  //ObtainSemaphore(&card->sb128_semaphore);

  if(card->es1370)
  {
    for (i = 0; i < 10; i++)
    {
      if (!(pci_inl(SB128_STATUS, card) & CODEC_CSTAT ))
        goto es1370_ok1;
      Delay(1);
    }
    DebugPrintF("SB128: Couldn't write to ak4531!\n");
    return;
    
    es1370_ok1:
    pci_outw(((unsigned char)reg << ES1370_CODEC_ADD_SHIFT) | (unsigned char)val, ES1370_SB128_CODEC, card);
    micro_delay(100);
  }
  else
  {
  
    /* Check for WIP. */
    for (i = 0; i < 0x1000; i++) 
    {
  	  if (!(pci_inl(SB128_CODEC, card) & CODEC_WIP ))
  	    goto ok1;
  	}
    DebugPrintF("SB128: Couldn't write to ac97! (1)\n");
    //ReleaseSemaphore(&card->sb128_semaphore);
    return;
   
    ok1:
    /* Get copy of SRC register when it's not busy. */
    r = src_ready(card);
    /* Enable "SRC State Data", an undocumented feature! */
    pci_outl((r & (SRC_DISABLE | SRC_DIS_DAC2 | SRC_DIS_ADC)) | 0x00010000, SB128_SRC, card);

    /* Wait for "state 0", to avoid "transition states". */
    for (i = 0; i < 0x1000; i++)
    {
      if ((pci_inl(SB128_SRC, card) & 0x00870000) == 0x00)
        break;
      //micro_delay(1);
    }
 
    /* Now wait for an undocumented bit to be set (and the SRC to be NOT busy) */
    for (i = 0; i < 0x1000; i++)
    {
      if ((pci_inl(SB128_SRC, card) & 0x00870000) == 0x00010000)
        break;
      //micro_delay(1);
    }

    /* Write out the value to the codec now. */
    pci_outl((((reg << CODEC_ADD_SHIFT) & CODEC_ADD_MASK) | val), SB128_CODEC, card);

    /* Delay to make sure the chip had time to set the WIP after
       the codec write. */
    //micro_delay(1);
 
    /* Restore SRC register. */
    src_ready(card);
    pci_outl(r, SB128_SRC, card);
 
    /* Check for WIP before returning. */
    for (i = 0; i < 0x1000; i++) 
    {
	    if (!(pci_inl(SB128_CODEC, card) & CODEC_WIP))
      {
        //ReleaseSemaphore(&card->sb128_semaphore); 
        return;
      }
    }

    DebugPrintF("SB128: Couldn't write to ac97! (2)\n");
  }

  //ReleaseSemaphore(&card->sb128_semaphore);
  return;
}

unsigned short codec_read(struct SB128_DATA *card, unsigned short reg)
{
  unsigned long i, r;
  unsigned short val;

  if(card->es1370)
    return 0;
  
//ObtainSemaphore(&card->sb128_semaphore);
  
  /* Check for WIP. */
  for (i = 0; i < 0x1000; i++) {
	  if (!((pci_inl(SB128_CODEC, card)) & CODEC_WIP ))
		  goto ok1;
	}
  DebugPrintF("SB128: Couldn't read from ac97! (1)\n");
//  ReleaseSemaphore(&card->sb128_semaphore);
  return 0;
   
  ok1:

  /* Get copy of SRC register when it's not busy. */
  r = src_ready(card);

  /* Enable "SRC State Data", an undocumented feature! */
  pci_outl((r & (SRC_DISABLE | SRC_DIS_DAC1 | SRC_DIS_DAC2 | SRC_DIS_ADC)) | 0x00010000, SB128_SRC, card);

  /* Wait for "state 0", to avoid "transition states".
     Seen in open code. */
  for (i = 0; i < 0x1000; i++)
  {
    if ((pci_inl(SB128_SRC, card) & 0x00870000) == 0x00)
      break;
    //micro_delay(1);    
  }

 /* Now wait for an undocumented bit to be set (and the SRC to be NOT busy) */
  for (i = 0; i < 0x1000; i++)
  {
    if ((pci_inl(SB128_SRC, card) & 0x00870000) == 0x00010000)
      break;
    //micro_delay(1);
  }

  /* Write the read request to the chip now */
  pci_outl((((reg << CODEC_ADD_SHIFT) & CODEC_ADD_MASK) | CODEC_READ), SB128_CODEC, card);
  
  /* Give the chip time to respond to our read request. */
  //micro_delay(1);

  /* Restore SRC register. */
  src_ready(card);
  pci_outl(r, SB128_SRC, card);
 
  /* Check for WIP. */
  for (i = 0; i < 0x1000; i++) {
    if (!((pci_inl(SB128_CODEC, card)) & CODEC_WIP))
      goto ok2;
  }
  DebugPrintF("SB128: Couldn't read from ac97 (2)!\n");
//  ReleaseSemaphore(&card->sb128_semaphore);
  return 0;
    
  ok2:
    
  /* Wait for RDY. */
  //micro_delay(1);
  for (i = 0; i < 0x1000; i++) {
		if (!((pci_inl(SB128_CODEC, card)) & CODEC_RDY))
		  goto ok3;
	}
  DebugPrintF("SB128: Couldn't read from ac97 (3)!\n");
//  ReleaseSemaphore(&card->sb128_semaphore);
  return 0;

  ok3:
  //micro_delay(5); 
  Delay(1); //A delay here is crucial, remove this if you use micro_delay()
  val = pci_inl(SB128_CODEC, card);
 
//  ReleaseSemaphore(&card->sb128_semaphore);

  return val;
}

void rate_set_adc(struct SB128_DATA *card, unsigned long rate)
{
  unsigned long n, truncm, freq;

  //ObtainSemaphore(&card->sb128_semaphore);
  
  if (rate > 48000)
    rate = 48000;
  if (rate < 4000)
    rate = 4000;

  if (card->es1370)
  {
    pci_outl(((pci_inl(SB128_CONTROL, card) & ~DAC2_DIV_MASK) | (DAC2_SRTODIV(rate) << DAC2_DIV_SHIFT)), SB128_CONTROL, card);
  }
  else
  {
  
    /* This is completely undocumented */
    n = rate / 3000;
    if ((1 << n) & ((1 << 15) | (1 << 13) | (1 << 11) | (1 << 9)))
      n--;
    truncm = (21 * n - 1) | 1;
    freq = ((48000UL << 15) / rate) * n;
 
    if (rate >= 24000)
    {
      if (truncm > 239)
        truncm = 239;
      src_write(card, SRC_ADC + SRC_TRUNC, (((239 - truncm) >> 1) << 9) | (n << 4));
    }
    else
    {
      if (truncm > 119)
        truncm = 119;
      src_write(card, SRC_ADC + SRC_TRUNC, 0x8000 | (((119 - truncm) >> 1) << 9) | (n << 4));
    }
    src_write(card, SRC_ADC + SRC_INT, 
        (src_read(card, SRC_ADC + SRC_INT) & 0x00FF) | ((freq >> 5) & 0xFC00));
    src_write(card, SRC_ADC + SRC_VF, freq & 0x7FFF);
    src_write(card, SRC_VOL_ADC, n << 8);
    src_write(card, SRC_VOL_ADC + 1, n << 8);

  }

    //ReleaseSemaphore(&card->sb128_semaphore);

}

void rate_set_dac2(struct SB128_DATA *card, unsigned long rate)
{
  unsigned long freq, r;
 
  //ObtainSemaphore(&card->sb128_semaphore);
 
  if (rate > 48000)
    rate = 48000;
  if (rate < 4000)
    rate = 4000;
  
  if(card->es1370)
  {
    pci_outl(((pci_inl(SB128_CONTROL, card) & ~DAC2_DIV_MASK) | (DAC2_SRTODIV(rate) << DAC2_DIV_SHIFT)), SB128_CONTROL, card);
  }
  else
  {
  
    freq = ((rate << 15 ) + 1500) / 3000;

    /* Get copy of SRC register when it's not busy, clear, preserve the disable bits, write back. */
    r = src_ready(card) & (SRC_DISABLE | SRC_DIS_DAC1 | SRC_DIS_DAC2 | SRC_DIS_ADC);
    pci_outl(r, SB128_SRC, card);
  
    /* This is completely undocumented */
    src_write(card, SRC_DAC2 + SRC_INT, 
        (src_read(card, SRC_DAC2 + SRC_INT) & 0x00FF) | ((freq >> 5) & 0xFC00));
    src_write(card, SRC_DAC2 + SRC_VF, freq & 0x7FFF);
    r = (src_ready(card) & (SRC_DISABLE | SRC_DIS_DAC1 | SRC_DIS_ADC));
    pci_outl(r, SB128_SRC, card);
  
  }

  //ReleaseSemaphore(&card->sb128_semaphore);

}

/******************************************************************************
** DriverData allocation ******************************************************
******************************************************************************/

/* This code used to be in _AHIsub_AllocAudio(), but since we're now
   handling CAMD support too, it needs to be done at driver loading
   time. */

struct SB128_DATA*
AllocDriverData( struct PCIDevice *    dev,
		 struct DriverBase* AHIsubBase )
{
  struct SB128_DATA* card;
  UWORD command_word;

    D(bug("[SB128]: %s()\n", __PRETTY_FUNCTION__);)

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
#ifdef __AROS__
    card->interrupt.is_Code         = (void(*)(void))&cardinterrupt;
#else
    card->interrupt.is_Code         = (void(*)(void))CardInterrupt;
#endif
  card->interrupt.is_Data         = (APTR) card;

  card->playback_interrupt.is_Node.ln_Type = IRQTYPE;
  card->playback_interrupt.is_Node.ln_Pri  = 0;
  card->playback_interrupt.is_Node.ln_Name = (STRPTR) LibName;
#ifdef __AROS__
    card->playback_interrupt.is_Code         = (void(*)(void))&playbackinterrupt;
#else
    card->playback_interrupt.is_Code         = (void(*)(void))PlaybackInterrupt;
#endif
  card->playback_interrupt.is_Data         = (APTR) card;

  card->record_interrupt.is_Node.ln_Type = IRQTYPE;
  card->record_interrupt.is_Node.ln_Pri  = 0;
  card->record_interrupt.is_Node.ln_Name = (STRPTR) LibName;
#ifdef __AROS__
    card->record_interrupt.is_Code         = (void(*)(void))&recordinterrupt;
#else
    card->record_interrupt.is_Code         = (void(*)(void))RecordInterrupt;
#endif
  card->record_interrupt.is_Data         = (APTR) card;

  card->pci_dev = dev;

  command_word = inw_config( PCI_COMMAND , dev);  
  command_word |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
  outw_config( PCI_COMMAND, command_word , dev);

  card->pci_master_enabled = TRUE;

  card->iobase  = ahi_pci_get_base_address(0, dev);
  card->length  = ~( ahi_pci_get_base_size(0, dev) & PCI_BASE_ADDRESS_IO_MASK );
  card->irq     = ahi_pci_get_irq(dev);
  card->chiprev = inb_config(PCI_REVISION_ID, dev);
  card->model   = inw_config(PCI_SUBSYSTEM_ID, dev);

    D(bug("[SB128]: %s: iobase = 0x%p, len = %d\n", __PRETTY_FUNCTION__, card->iobase, card->length);)

  /* Initialise hardware access Semaphore */
  InitSemaphore(&card->sb128_semaphore);

  
  /* Initialize chip */
  if( card_init( card ) < 0 )
  {
    DebugPrintF("SB128: Unable to initialize Card subsystem.");
    return NULL;
  }

    ahi_pci_add_intserver(&card->interrupt, dev);
  card->interrupt_added = TRUE;
  

  card->card_initialized = TRUE;
  card->input          = 0;
  card->output         = 0;
  card->monitor_volume = Linear2MixerGain( 0, &card->monitor_volume_bits );
  card->input_gain     = Linear2RecordGain( 0x10000, &card->input_gain_bits );
  card->output_volume  = Linear2MixerGain( 0x10000, &card->output_volume_bits );
  SaveMixerState(card);

#if !defined(__AROS__)
  AddResetHandler(card);
#endif

  return card;
}


/******************************************************************************
** DriverData deallocation ****************************************************
******************************************************************************/

/* And this code used to be in _AHIsub_FreeAudio(). */

void
FreeDriverData( struct SB128_DATA* card,
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


int card_init(struct SB128_DATA *card)
{
    struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;
    unsigned short cod;
    unsigned int i;

    /* Check if the card is an original ES1370 - different code needed */
    if (inw_config(2, dev) == 0x5000)
        card->es1370 = TRUE;
    else
        card->es1370 = FALSE;
    
    /* Different init sequence for the ES1370 */
    if (card->es1370)
    {
      /* Enable CODEC access, set DAC sample rate to 44100 */
      pci_outl(CTRL_CDC_EN | (DAC2_SRTODIV(44100) << DAC2_DIV_SHIFT), SB128_CONTROL, card);
      pci_outl(0x00, SB128_SCON, card);
      DebugPrintF("SB128: Did RATE init\n");
      
      /* CODEC initialisation */
      codec_write(card, AK4531_RESET,             0x03); /* Enable CODEC */
      codec_write(card, AK4531_CLOCK_SEL,         0x00); /* CODEC ADC and DAC use PLL */
      codec_write(card, AK4531_RECORD_SELECT,     0x00); /* CODEC ADC set to use mixer for input */
      codec_write(card, AK4531_RECORD_GAIN_MIC,   0x00); /* Mic gain set to 0 dB */

      /* Volume initialisation */
      codec_write(card, AK4531_MASTER_VOL_L,    0x00); /* No attentuation */
      codec_write(card, AK4531_MASTER_VOL_R,    0x00);
      codec_write(card, AK4531_MASTER_VOL_MONO, 0x00);

      /* Analogue mixer input gain registers */
      codec_write(card, AK4531_PHONE_VOL_L,   AK4531_MUTE | 0x06);
      codec_write(card, AK4531_PHONE_VOL_R,   AK4531_MUTE | 0x06);
      codec_write(card, AK4531_MIC_VOL,       AK4531_MUTE | 0x06);
      codec_write(card, AK4531_LINEIN_VOL_L,  AK4531_MUTE | 0x06);
      codec_write(card, AK4531_LINEIN_VOL_R,  AK4531_MUTE | 0x06);
      codec_write(card, AK4531_CD_VOL_L,      0x06);
      codec_write(card, AK4531_CD_VOL_R,      0x06);
      codec_write(card, AK4531_AUX_VOL_L,     0x06);
      codec_write(card, AK4531_AUX_VOL_R,     0x06);
      codec_write(card, AK4531_PCMOUT_VOL_L,  0x06);
      codec_write(card, AK4531_PCMOUT_VOL_R,  0x06);

      /* Mixer registers */
     
      /* Always on */
      codec_write(card, AK4531_OUTPUT_MUX_1,  0x1F);
      codec_write(card, AK4531_OUTPUT_MUX_2,  0x3F);

      /* Store value of OUTPUT MUX registers */
      card->ak4531_output_1 = 0x1F;
      card->ak4531_output_2 = 0x3F;
     
      /* Analogous to "Record Select", only TMIC and Phone enabled here */
      codec_write(card, AK4531_INPUT_MUX_L_1,  0x00);
      codec_write(card, AK4531_INPUT_MUX_R_1,  0x00);
      codec_write(card, AK4531_INPUT_MUX_L_2,  0x80);
      codec_write(card, AK4531_INPUT_MUX_R_2,  0x80);

      DebugPrintF("SB128: Did VOLUME init\n");
    }   
    else
    {
      /* Basic clear of everything */
      pci_outl(0x00, SB128_CONTROL, card);
      pci_outl(0x00, SB128_SCON, card);
      pci_outl(0x00, SB128_LEGACY, card);

      /* Magical CT5880 AC97 enable bit plus 20ms delay
      (Gotta love the undocumented stuff) */
      pci_outl(0x20000000, SB128_STATUS, card);
      Delay(1);

      /* Assert the AC97 reset, and wait 20ms */
      pci_outl(CODEC_RESET, SB128_CONTROL, card);
      Delay(1);
      /* De-assert delay, and wait 20ms */
      pci_outl(0x00, SB128_CONTROL, card);
      Delay(1);

      DebugPrintF("SB128: Did AC97 reset.\n");

      /* Disable the Sample Rate Converter (SRC) */
      src_ready(card);
      pci_outl(SRC_DISABLE, SB128_SRC, card);
      /* Clear the SRC RAM */
      for (i = 0; i < 0x80; i++)
        src_write(card, i, 0);

      DebugPrintF("SB128: Did SRC wipe.\n");

      /* Perform basic configuration of the SRC, not well documented! */
      src_write(card, SRC_DAC1 + SRC_TRUNC, 0x100);
      src_write(card, SRC_DAC1 + SRC_INT, 0x4000);
      src_write(card, SRC_DAC2 + SRC_TRUNC, 0x100);
      src_write(card, SRC_DAC2 + SRC_INT, 0x4000);
      src_write(card, SRC_VOL_ADC, 0x1000);
      src_write(card, SRC_VOL_ADC + 1, 0x1000);
      src_write(card, SRC_VOL_DAC1, 0x1000);
      src_write(card, SRC_VOL_DAC1 + 1, 0x1000);
      src_write(card, SRC_VOL_DAC2, 0x1000);
      src_write(card, SRC_VOL_DAC2 + 1, 0x1000);

      DebugPrintF("SB128: Did SRC init.\n");

      rate_set_adc(card, 44100);
      rate_set_dac2(card, 44100);

      /* Re-enable the SRC */
      src_ready(card);
      pci_outl(0, SB128_SRC, card);

      card->currentPlayFreq = 9;
      card->currentRecFreq = 9;

      DebugPrintF("SB128: Did RATE init.\n");

      /* Initialise registers of AC97 to default */
      codec_write(card, AC97_RESET, 0x00);

      /* Initialise volumes of AC97 */
      codec_write(card, AC97_MASTER_VOL_STEREO, 0x0000 ); /* no attenuation */
      codec_write(card, AC97_AUXOUT_VOL,        0x0000 ); /* volume of the rear output */
      codec_write(card, AC97_MASTER_VOL_MONO,   0x0000 );
      codec_write(card, AC97_MASTER_TONE,       0x0f0f ); /* bass/treble control (if present) */

      codec_write(card, AC97_RECORD_SELECT,     0);
      codec_write(card, AC97_RECORD_GAIN,       0x0000 ); /* 0 dB gain */

      /* Analogue mixer input gain registers */
      codec_write(card, AC97_PHONE_VOL,         AC97_MUTE | 0x0008 );
      codec_write(card, AC97_MIC_VOL,           AC97_MUTE | 0x0008 );
      codec_write(card, AC97_LINEIN_VOL,        AC97_MUTE | 0x0808 );
      codec_write(card, AC97_CD_VOL,            0x0808 );
      codec_write(card, AC97_VIDEO_VOL,         AC97_MUTE | 0x0808 );
      codec_write(card, AC97_AUX_VOL,           0x0808 );
      codec_write(card, AC97_PCMOUT_VOL,        0x0808 );

      DebugPrintF("SB128: Did VOLUME init.\n");

      cod = codec_read(card, AC97_RESET);
      DebugPrintF("SB128: AC97 capabilities = %x\n", cod);

      cod = codec_read(card, AC97_VENDOR_ID0);
      DebugPrintF("SB128: AC97_VENDOR_ID0 = %x\n", cod);

      cod = codec_read(card, AC97_VENDOR_ID1);
      DebugPrintF("SB128: AC97_VENDOR_ID1 = %x\n", cod);
    }

    return 0;
}


void card_cleanup(struct SB128_DATA *card)
{
}



/******************************************************************************
** Misc. **********************************************************************
******************************************************************************/

void
SaveMixerState( struct SB128_DATA* card )
{
  card->ac97_mic    = codec_read( card, AC97_MIC_VOL );
  card->ac97_cd     = codec_read( card, AC97_CD_VOL );
  card->ac97_aux    = codec_read( card, AC97_AUX_VOL );
  card->ac97_linein = codec_read( card, AC97_LINEIN_VOL );
  card->ac97_phone  = codec_read( card, AC97_PHONE_VOL );
}


void
RestoreMixerState( struct SB128_DATA* card )
{
  if(card->es1370)
  {
    /* Not possible to save the state, so restore all volumes to mid levels */
    ak4531_ac97_write(card, AC97_MIC_VOL, 0x0808);
    ak4531_ac97_write(card, AC97_CD_VOL, 0x0808);
    ak4531_ac97_write(card, AC97_AUX_VOL, 0x0808);
    ak4531_ac97_write(card, AC97_LINEIN_VOL, 0x0808);
    ak4531_ac97_write(card, AC97_PHONE_VOL, 0x0808);
  }
  else
  {
    codec_write(card, AC97_MIC_VOL,    card->ac97_mic );
    codec_write(card, AC97_CD_VOL,     card->ac97_cd );
    codec_write(card, AC97_AUX_VOL,    card->ac97_aux );
    codec_write(card, AC97_LINEIN_VOL, card->ac97_linein );
    codec_write(card, AC97_PHONE_VOL,  card->ac97_phone );
  }
}

void
UpdateMonitorMixer( struct SB128_DATA* card )
{
  int   i  = InputBits[ card->input ];
  UWORD m  = card->monitor_volume_bits & 0x801f;
  UWORD s  = card->monitor_volume_bits;
  UWORD mm = AC97_MUTE | 0x0008;
  UWORD sm = AC97_MUTE | 0x0808;

  if( i == AC97_RECMUX_STEREO_MIX )
  {
    /* Use the original mixer settings */
    RestoreMixerState( card );
  }
  else
  {
    if(card->es1370)
    {
      ak4531_ac97_write(card, AC97_MIC_VOL,
		        i == AC97_RECMUX_MIC ? m : mm );

      ak4531_ac97_write(card, AC97_CD_VOL,
		        i == AC97_RECMUX_CD ? s : sm );

      ak4531_ac97_write(card, AC97_AUX_VOL,
		        i == AC97_RECMUX_AUX ? s : sm );

      ak4531_ac97_write(card, AC97_LINEIN_VOL,
		        i == AC97_RECMUX_LINE ? s : sm );

      ak4531_ac97_write(card, AC97_PHONE_VOL,
		        i == AC97_RECMUX_PHONE ? m : mm );
    }
    else
    {
      codec_write(card, AC97_MIC_VOL,
		        i == AC97_RECMUX_MIC ? m : mm );

      codec_write(card, AC97_CD_VOL,
		        i == AC97_RECMUX_CD ? s : sm );

      codec_write(card, AC97_AUX_VOL,
		        i == AC97_RECMUX_AUX ? s : sm );

      codec_write(card, AC97_LINEIN_VOL,
		        i == AC97_RECMUX_LINE ? s : sm );

      codec_write(card, AC97_PHONE_VOL,
		        i == AC97_RECMUX_PHONE ? m : mm );
    }
  }
}


Fixed
Linear2MixerGain( Fixed  linear,
		  UWORD* bits )
{
  static const Fixed gain[ 33 ] =
  {
    260904, /* +12.0 dB */
    219523, /* +10.5 dB */
    184706, /*  +9.0 dB */
    155410, /*  +7.5 dB */
    130762, /*  +6.0 dB */
    110022, /*  +4.5 dB */
    92572,  /*  +3.0 dB */
    77890,  /*  +1.5 dB */
    65536,  /*  ±0.0 dB */
    55142,  /*  -1.5 dB */
    46396,  /*  -3.0 dB */
    39037,  /*  -4.5 dB */
    32846,  /*  -6.0 dB */
    27636,  /*  -7.5 dB */
    23253,  /*  -9.0 dB */
    19565,  /* -10.5 dB */
    16462,  /* -12.0 dB */
    13851,  /* -13.5 dB */
    11654,  /* -15.0 dB */
    9806,   /* -16.5 dB */
    8250,   /* -18.0 dB */
    6942,   /* -19.5 dB */
    5841,   /* -21.0 dB */
    4915,   /* -22.5 dB */
    4135,   /* -24.0 dB */
    3479,   /* -25.5 dB */
    2927,   /* -27.0 dB */
    2463,   /* -28.5 dB */
    2072,   /* -30.0 dB */
    1744,   /* -31.5 dB */
    1467,   /* -33.0 dB */
    1234,   /* -34.5 dB */
    0       /*   -oo dB */
  };

  int v = 0;
  while( linear < gain[ v ] )
  {
    ++v;
  }

  if( v == 32 )
  {
    *bits = 0x8000; /* Mute */
  }
  else
  {
    *bits = ( v << 8 ) | v;
  }
  return gain[ v ];
}

Fixed
Linear2RecordGain( Fixed  linear,
		   UWORD* bits )
{
  static const Fixed gain[ 17 ] =
  {
    873937, /* +22.5 dB */
    735326, /* +21.0 dB */
    618700, /* +19.5 dB */
    520571, /* +18.0 dB */
    438006, /* +16.5 dB */
    368536, /* +15.0 dB */
    310084, /* +13.5 dB */
    260904, /* +12.0 dB */
    219523, /* +10.5 dB */
    184706, /*  +9.0 dB */
    155410, /*  +7.5 dB */
    130762, /*  +6.0 dB */
    110022, /*  +4.5 dB */
    92572,  /*  +3.0 dB */
    77890,  /*  +1.5 dB */
    65536,  /*  ±0.0 dB */
    0       /*  -oo dB */
  };

  int v = 0;

  while( linear < gain[ v ] )
  {
    ++v;
  }

  if( v == 16 )
  {
    *bits = 0x8000; /* Mute */
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


void *pci_alloc_consistent(size_t size, APTR *NonAlignedAddress, unsigned int boundary)
{
    void* address;
    unsigned long a;

    D(bug("[SB128]: %s()\n", __PRETTY_FUNCTION__);)

    address = (void *) AllocVec(size + boundary, (
#if defined(__AROS__) && (__WORDSIZE==64)
        MEMF_31BIT |
#endif
        MEMF_PUBLIC | MEMF_CLEAR));

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
    D(bug("[SB128]: %s()\n", __PRETTY_FUNCTION__);)

    FreeVec(addr);
}

#if !defined(__AROS__)
static ULONG ResetHandler(struct ExceptionContext *ctx, struct ExecBase *pExecBase, struct SB128_DATA *card)
{
  struct PCIDevice *dev = card->pci_dev;

  //Stop SB128 interrupts and playback/recording
  unsigned long ctrl;

  ctrl = pci_inl(SB128_CONTROL, card);
  ctrl &= ( ~(CTRL_DAC2_EN)  &  ~(CTRL_ADC_EN) );
  
  /* Stop */
  pci_outl(ctrl, SB128_CONTROL, card);

  /* Clear and mask interrupts */
  pci_outl((pci_inl(SB128_SCON, card) & SB128_IRQ_MASK), SB128_SCON, card);

  return 0UL;
}

void AddResetHandler(struct SB128_DATA *card)
{
  static struct Interrupt interrupt;

  interrupt.is_Code = (void (*)())ResetHandler;
  interrupt.is_Data = (APTR) card;
  interrupt.is_Node.ln_Pri = 0;
  interrupt.is_Node.ln_Type = NT_EXTINTERRUPT;
  interrupt.is_Node.ln_Name = "SB128 Reset Handler";

  AddResetCallback( &interrupt );
}
#endif
