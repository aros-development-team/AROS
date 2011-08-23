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

#undef __USE_INLINE__
#include <proto/expansion.h>

#include <proto/dos.h>

#include <devices/timer.h>

#include "library_card.h"
#include "regs.h"
#include "interrupt.h"
#include "misc.h"
#include "DriverData.h"

/* Global in Card.c */
extern const UWORD InputBits[];
extern struct DOSIFace *IDOS;

//extern struct timerequest *TimerIO;
extern struct MsgPort* replymp; 

/* Public functions in main.c */
int card_init(struct CardData *card);
void card_cleanup(struct CardData *card);

void AddResetHandler(struct CardData *card);

void MicroDelay(unsigned int val)
{
  struct Device*              TimerBase = NULL;
  struct TimeRequest*         TimerIO = NULL;
  struct MsgPort *            replymp;

  replymp = (struct MsgPort *) IExec->CreatePort(NULL, 0);
  if (!replymp)
  {
    IExec->DebugPrintF("SB128: Couldn't create reply port\n");
    return;
  }

  TimerIO = (struct TimeRequest *)IExec->CreateIORequest(replymp, sizeof(struct TimeRequest));

  if (TimerIO == NULL)
  {
    IExec->DebugPrintF("SB128: Out of memory.\n");
    return;
  }

  if (IExec->OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)TimerIO, 0) != 0)
  {
    IExec->DebugPrintF("SB128: Unable to open 'timer.device'.\n");
    return;
  }
  else
  {
    TimerBase = (struct Device *)TimerIO->Request.io_Device;
  }
  
  if (TimerIO)
  {
    TimerIO->Request.io_Command = TR_ADDREQUEST;
    TimerIO->Time.Seconds = 0;
    TimerIO->Time.Microseconds = val;
    IExec->DoIO((struct IORequest *)TimerIO);
    IExec->DeleteIORequest((struct IORequest *)TimerIO);
    TimerIO = NULL;
    IExec->CloseDevice((struct IORequest *)TimerIO);
    IExec->DeletePort(replymp);
  }
}

unsigned long src_ready(struct CardData *card)
{
  struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;
  unsigned int i;
  unsigned long r;

  /* Wait for the busy bit to become invalid, and then return the contents
     of the SRC register. */
  for (i = 0; i < 0x1000; i++) 
  {
    if (!((r = dev->InLong(card->iobase + SB128_SRC)) & SRC_BUSY))
        return r;
    //MicroDelay(1);
  }  

  IExec->DebugPrintF("SB128: SRC Ready timeout.\n");
  return 0;
}

void src_write(struct CardData *card, unsigned short addr, unsigned short data)
{
  struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;
  unsigned long r;

//  IExec->ObtainSemaphore(&card->sb128_semaphore);
  
  /* Get copy of SRC register when it's not busy, add address and data, write back. */
  r = src_ready(card) & (SRC_DISABLE | SRC_DIS_DAC2 | SRC_DIS_ADC);
  r = r | (addr << SRC_ADDR_SHIFT);
  r = r | data;
  dev->OutLong(card->iobase + SB128_SRC, r | SRC_WE);
  //MicroDelay(1);

//  IExec->ReleaseSemaphore(&card->sb128_semaphore);
}

unsigned short src_read(struct CardData *card, unsigned short addr)
{
  struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;

  //There may be ES137x bugs that require accomodating in this section.
  
  unsigned long r;

//  IExec->ObtainSemaphore(&card->sb128_semaphore);
  
  /* Get copy of SRC register when it's not busy, add address, write back,
     wait for ready, then read back value. */
  r = src_ready(card) & (SRC_DISABLE | SRC_DIS_DAC2 | SRC_DIS_ADC);
  r = r | (addr << SRC_ADDR_SHIFT);
  dev->OutLong(card->iobase + SB128_SRC, r);
  
  /* Give the chip a chance to set the busy bit. */
  //MicroDelay(1);
  
//  IExec->ReleaseSemaphore(&card->sb128_semaphore);
  
  return (src_ready(card) & 0xFFFF);
} 

/* Translate AC97 commands to AK4531 commands, and write to the AK4531 codec */
void ak4531_ac97_write(struct CardData *card, unsigned short reg, unsigned short val)
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
        IExec->DebugPrintF("SB128: Unsupported Record Input command\n");
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
        IExec->DebugPrintF("SB128: Unsupported Record Input command\n");
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
        IExec->DebugPrintF("SB128: Invalid value for Volume Set\n");
    }
    
    return;

  }

}

void codec_write(struct CardData *card, unsigned short reg, unsigned short val)
{
  struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;
  unsigned long i, r;

  /* Take hold of the hardware semaphore */
  //IExec->ObtainSemaphore(&card->sb128_semaphore);

  if(card->es1370)
  {
    for (i = 0; i < 10; i++)
    {
      if (!(dev->InLong(card->iobase + SB128_STATUS) & CODEC_CSTAT ))
        goto es1370_ok1;
      IDOS->Delay(1);
    }
    IExec->DebugPrintF("SB128: Couldn't write to ak4531!\n");
    return;
    
    es1370_ok1:
    dev->OutWord(card->iobase + ES1370_SB128_CODEC, ((unsigned char)reg << ES1370_CODEC_ADD_SHIFT) | (unsigned char)val);
    MicroDelay(100);
  }
  else
  {
  
    /* Check for WIP. */
    for (i = 0; i < 0x1000; i++) 
    {
  	  if (!(dev->InLong(card->iobase + SB128_CODEC) & CODEC_WIP ))
  	    goto ok1;
  	}
    IExec->DebugPrintF("SB128: Couldn't write to ac97! (1)\n");
    //IExec->ReleaseSemaphore(&card->sb128_semaphore);
    return;
   
    ok1:
    /* Get copy of SRC register when it's not busy. */
    r = src_ready(card);
    /* Enable "SRC State Data", an undocumented feature! */
    dev->OutLong(card->iobase + SB128_SRC, (r & (SRC_DISABLE | SRC_DIS_DAC2 |
          SRC_DIS_ADC)) | 0x00010000);

    /* Wait for "state 0", to avoid "transition states". */
    for (i = 0; i < 0x1000; i++)
    {
      if ((dev->InLong(card->iobase + SB128_SRC) & 0x00870000) == 0x00)
        break;
      //MicroDelay(1);
    }
 
    /* Now wait for an undocumented bit to be set (and the SRC to be NOT busy) */
    for (i = 0; i < 0x1000; i++)
    {
      if ((dev->InLong(card->iobase + SB128_SRC) & 0x00870000) == 0x00010000)
        break;
      //MicroDelay(1);
    }

    /* Write out the value to the codec now. */
    dev->OutLong(card->iobase + SB128_CODEC, (((reg << CODEC_ADD_SHIFT) & CODEC_ADD_MASK) | val));

    /* Delay to make sure the chip had time to set the WIP after
       the codec write. */
    //MicroDelay(1);
 
    /* Restore SRC register. */
    src_ready(card);
    dev->OutLong(card->iobase + SB128_SRC, r);
 
    /* Check for WIP before returning. */
    for (i = 0; i < 0x1000; i++) 
    {
	    if (!(dev->InLong(card->iobase + SB128_CODEC) & CODEC_WIP))
      {
        //IExec->ReleaseSemaphore(&card->sb128_semaphore); 
        return;
      }
    }

    IExec->DebugPrintF("SB128: Couldn't write to ac97! (2)\n");
  }

  //IExec->ReleaseSemaphore(&card->sb128_semaphore);
  return;
}

unsigned short codec_read(struct CardData *card, unsigned short reg)
{
  struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;
  unsigned long i, r;
  unsigned short val;

  if(card->es1370)
    return 0;
  
//IExec->ObtainSemaphore(&card->sb128_semaphore);
  
  /* Check for WIP. */
  for (i = 0; i < 0x1000; i++) {
	  if (!((dev->InLong(card->iobase + SB128_CODEC)) & CODEC_WIP ))
		  goto ok1;
	}
  IExec->DebugPrintF("SB128: Couldn't read from ac97! (1)\n");
//  IExec->ReleaseSemaphore(&card->sb128_semaphore);
  return 0;
   
  ok1:

  /* Get copy of SRC register when it's not busy. */
  r = src_ready(card);

  /* Enable "SRC State Data", an undocumented feature! */
  dev->OutLong(card->iobase + SB128_SRC, (r & (SRC_DISABLE | SRC_DIS_DAC1 | SRC_DIS_DAC2 |
        SRC_DIS_ADC)) | 0x00010000);

  /* Wait for "state 0", to avoid "transition states".
     Seen in open code. */
  for (i = 0; i < 0x1000; i++)
  {
    if ((dev->InLong(card->iobase + SB128_SRC) & 0x00870000) == 0x00)
      break;
    //MicroDelay(1);    
  }

 /* Now wait for an undocumented bit to be set (and the SRC to be NOT busy) */
  for (i = 0; i < 0x1000; i++)
  {
    if ((dev->InLong(card->iobase + SB128_SRC) & 0x00870000) == 0x00010000)
      break;
    //MicroDelay(1);
  }

  /* Write the read request to the chip now */
  dev->OutLong(card->iobase + SB128_CODEC, (((reg << CODEC_ADD_SHIFT) & CODEC_ADD_MASK) | CODEC_READ));
  
  /* Give the chip time to respond to our read request. */
  //MicroDelay(1);

  /* Restore SRC register. */
  src_ready(card);
  dev->OutLong(card->iobase + SB128_SRC, r);
 
  /* Check for WIP. */
  for (i = 0; i < 0x1000; i++) {
    if (!((dev->InLong(card->iobase + SB128_CODEC)) & CODEC_WIP))
      goto ok2;
  }
  IExec->DebugPrintF("SB128: Couldn't read from ac97 (2)!\n");
//  IExec->ReleaseSemaphore(&card->sb128_semaphore);
  return 0;
    
  ok2:
    
  /* Wait for RDY. */
  //MicroDelay(1);
  for (i = 0; i < 0x1000; i++) {
		if (!((dev->InLong(card->iobase + SB128_CODEC)) & CODEC_RDY))
		  goto ok3;
	}
  IExec->DebugPrintF("SB128: Couldn't read from ac97 (3)!\n");
//  IExec->ReleaseSemaphore(&card->sb128_semaphore);
  return 0;

  ok3:
  //MicroDelay(5); 
  IDOS->Delay(1); //A delay here is crucial, remove this if you use MicroDelay()
  val = dev->InLong(card->iobase + SB128_CODEC);
 
//  IExec->ReleaseSemaphore(&card->sb128_semaphore);

  return val;
}

void rate_set_adc(struct CardData *card, unsigned long rate)
{
  struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;
  
  unsigned long n, truncm, freq;

  //IExec->ObtainSemaphore(&card->sb128_semaphore);
  
  if (rate > 48000)
    rate = 48000;
  if (rate < 4000)
    rate = 4000;

  if (card->es1370)
  {
    dev->OutLong(card->iobase + SB128_CONTROL, ((dev->InLong(card->iobase + SB128_CONTROL) & ~DAC2_DIV_MASK) | (DAC2_SRTODIV(rate) << DAC2_DIV_SHIFT)));
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

    //IExec->ReleaseSemaphore(&card->sb128_semaphore);

}

void rate_set_dac2(struct CardData *card, unsigned long rate)
{
  struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;

  unsigned long freq, r;
 
  //IExec->ObtainSemaphore(&card->sb128_semaphore);
 
  if (rate > 48000)
    rate = 48000;
  if (rate < 4000)
    rate = 4000;
  
  if(card->es1370)
  {
    dev->OutLong(card->iobase + SB128_CONTROL, ((dev->InLong(card->iobase + SB128_CONTROL) & ~DAC2_DIV_MASK) | (DAC2_SRTODIV(rate) << DAC2_DIV_SHIFT)));
  }
  else
  {
  
    freq = ((rate << 15 ) + 1500) / 3000;

    /* Get copy of SRC register when it's not busy, clear, preserve the disable bits, write back. */
    r = src_ready(card) & (SRC_DISABLE | SRC_DIS_DAC1 | SRC_DIS_DAC2 | SRC_DIS_ADC);
    dev->OutLong(card->iobase + SB128_SRC, r);
  
    /* This is completely undocumented */
    src_write(card, SRC_DAC2 + SRC_INT, 
        (src_read(card, SRC_DAC2 + SRC_INT) & 0x00FF) | ((freq >> 5) & 0xFC00));
    src_write(card, SRC_DAC2 + SRC_VF, freq & 0x7FFF);
    r = (src_ready(card) & (SRC_DISABLE | SRC_DIS_DAC1 | SRC_DIS_ADC));
    dev->OutLong(card->iobase + SB128_SRC, r);
  
  }

  //IExec->ReleaseSemaphore(&card->sb128_semaphore);

}

/******************************************************************************
** DriverData allocation ******************************************************
******************************************************************************/

/* This code used to be in _AHIsub_AllocAudio(), but since we're now
   handling CAMD support too, it needs to be done at driver loading
   time. */

struct CardData*
AllocDriverData( struct PCIDevice *    dev,
		 struct DriverBase* AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;
  struct CardData* card;
  UWORD command_word;
  int i;
  BOOL res = FALSE;

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

  card->playback_interrupt.is_Node.ln_Type = NT_INTERRUPT;
  card->playback_interrupt.is_Node.ln_Pri  = 0;
  card->playback_interrupt.is_Node.ln_Name = (STRPTR) LibName;
  card->playback_interrupt.is_Code         = PlaybackInterrupt;
  card->playback_interrupt.is_Data         = (APTR) card;

  card->record_interrupt.is_Node.ln_Type = NT_INTERRUPT;
  card->record_interrupt.is_Node.ln_Pri  = 0;
  card->record_interrupt.is_Node.ln_Name = (STRPTR) LibName;
  card->record_interrupt.is_Code         = RecordInterrupt;
  card->record_interrupt.is_Data         = (APTR) card;

  card->pci_dev = dev;

  command_word = dev->ReadConfigWord( PCI_COMMAND );  
  command_word |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
  dev->WriteConfigWord( PCI_COMMAND, command_word );

  card->pci_master_enabled = TRUE;

  card->iobase  = dev->GetResourceRange(0)->BaseAddress;
  card->length  = ~( dev->GetResourceRange(0)->Size & PCI_BASE_ADDRESS_IO_MASK );
  card->irq     = dev->MapInterrupt();
  card->chiprev = dev->ReadConfigByte( PCI_REVISION_ID);
  card->model   = dev->ReadConfigWord( PCI_SUBSYSTEM_ID);

  IExec->DebugPrintF("SB128: Device = %x, Vendor = %x, Revision = %x\n", dev->ReadConfigWord(PCI_DEVICE_ID),
                     dev->ReadConfigWord(PCI_VENDOR_ID), dev->ReadConfigByte(PCI_REVISION_ID));


  /* Initialise hardware access Semaphore */
  IExec->InitSemaphore(&card->sb128_semaphore);

  
  /* Initialize chip */
  if( card_init( card ) < 0 )
  {
    IExec->DebugPrintF("SB128: Unable to initialize Card subsystem.");
    return NULL;
  }


  res = IExec->AddIntServer(dev->MapInterrupt(), &card->interrupt );
  card->interrupt_added = TRUE;
  

  card->card_initialized = TRUE;
  card->input          = 0;
  card->output         = 0;
  card->monitor_volume = Linear2MixerGain( 0, &card->monitor_volume_bits );
  card->input_gain     = Linear2RecordGain( 0x10000, &card->input_gain_bits );
  card->output_volume  = Linear2MixerGain( 0x10000, &card->output_volume_bits );
  SaveMixerState(card);

  AddResetHandler(card);
  
  return card;
}


/******************************************************************************
** DriverData deallocation ****************************************************
******************************************************************************/

/* And this code used to be in _AHIsub_FreeAudio(). */

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
    unsigned int i;

    /* Check if the card is an original ES1370 - different code needed */
    if (dev->ReadConfigWord(2) == 0x5000)
        card->es1370 = TRUE;
    else
        card->es1370 = FALSE;
    
    /* Different init sequence for the ES1370 */
    if (card->es1370)
    {
      /* Enable CODEC access, set DAC sample rate to 44100 */
      dev->OutLong(card->iobase + SB128_CONTROL, CTRL_CDC_EN | (DAC2_SRTODIV(44100) << DAC2_DIV_SHIFT));
      dev->OutLong(card->iobase + SB128_SCON, 0x00);
      IExec->DebugPrintF("SB128: Did RATE init\n");
      
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

      IExec->DebugPrintF("SB128: Did VOLUME init\n");
    }   
    else
    {
      /* Basic clear of everything */
      dev->OutLong(card->iobase + SB128_CONTROL, 0x00);
      dev->OutLong(card->iobase + SB128_SCON, 0x00);
      dev->OutLong(card->iobase + SB128_LEGACY, 0x00);

      /* Magical CT5880 AC97 enable bit plus 20ms delay
      (Gotta love the undocumented stuff) */
      dev->OutLong(card->iobase + SB128_STATUS, 0x20000000);
      IDOS->Delay(1);

      /* Assert the AC97 reset, and wait 20ms */
      dev->OutLong(card->iobase + SB128_CONTROL, CODEC_RESET);
      IDOS->Delay(1);
      /* De-assert delay, and wait 20ms */
      dev->OutLong(card->iobase + SB128_CONTROL, 0x00);
      IDOS->Delay(1);

      IExec->DebugPrintF("SB128: Did AC97 reset.\n");

      /* Disable the Sample Rate Converter (SRC) */
      src_ready(card);
      dev->OutLong(card->iobase + SB128_SRC, SRC_DISABLE);
      /* Clear the SRC RAM */
      for (i = 0; i < 0x80; i++)
        src_write(card, i, 0);

      IExec->DebugPrintF("SB128: Did SRC wipe.\n");

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

      IExec->DebugPrintF("SB128: Did SRC init.\n");

      rate_set_adc(card, 44100);
      rate_set_dac2(card, 44100);

      /* Re-enable the SRC */
      src_ready(card);
      dev->OutLong(card->iobase + SB128_SRC, 0);

      card->currentPlayFreq = 9;
      card->currentRecFreq = 9;

      IExec->DebugPrintF("SB128: Did RATE init.\n");

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

      IExec->DebugPrintF("SB128: Did VOLUME init.\n");

      cod = codec_read(card, AC97_RESET);
      IExec->DebugPrintF("SB128: AC97 capabilities = %x\n", cod);

      cod = codec_read(card, AC97_VENDOR_ID0);
      IExec->DebugPrintF("SB128: AC97_VENDOR_ID0 = %x\n", cod);

      cod = codec_read(card, AC97_VENDOR_ID1);
      IExec->DebugPrintF("SB128: AC97_VENDOR_ID1 = %x\n", cod);
    }

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
  card->ac97_mic    = codec_read( card, AC97_MIC_VOL );
  card->ac97_cd     = codec_read( card, AC97_CD_VOL );
  card->ac97_aux    = codec_read( card, AC97_AUX_VOL );
  card->ac97_linein = codec_read( card, AC97_LINEIN_VOL );
  card->ac97_phone  = codec_read( card, AC97_PHONE_VOL );
}


void
RestoreMixerState( struct CardData* card )
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
UpdateMonitorMixer( struct CardData* card )
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


void *pci_alloc_consistent(size_t size, APTR * NonAlignedAddress)
{
  void* address;
  unsigned long a;

  if (IExec->OpenResource("newmemory.resource"))
  {
    address = IExec->AllocVecTags(size, AVT_Type, MEMF_SHARED, AVT_Contiguous, TRUE, AVT_Lock, TRUE,
                                  AVT_PhysicalAlignment, CACHELINE_SIZE, AVT_Clear, 0, TAG_DONE);

    *NonAlignedAddress = address;
  }
  else
  {
    address = IExec->AllocVec(size + CACHELINE_SIZE, MEMF_PUBLIC | MEMF_CLEAR);

    *NonAlignedAddress = address;

    if( address != NULL )
    {
      a = (unsigned long) address;
      a = (a + CACHELINE_SIZE - 1) & ~(CACHELINE_SIZE - 1);
      address = (void *) a;
    }
 
  }
  
  return address;
}


void pci_free_consistent(void* addr)
{
  IExec->FreeVec(addr);
}

static ULONG ResetHandler(struct ExceptionContext *ctx, struct ExecBase *pExecBase, struct CardData *card)
{
  struct PCIDevice *dev = card->pci_dev;

  //Stop SB128 interrupts and playback/recording
  unsigned long ctrl;

  ctrl = dev->InLong(card->iobase + SB128_CONTROL);
  ctrl &= ( ~(CTRL_DAC2_EN)  &  ~(CTRL_ADC_EN) );
  
  /* Stop */
  dev->OutLong(card->iobase + SB128_CONTROL, ctrl);

  /* Clear and mask interrupts */
  dev->OutLong(card->iobase + SB128_SCON, (dev->InLong(card->iobase + SB128_SCON)) & SB128_IRQ_MASK);

  return 0UL;
}

void AddResetHandler(struct CardData *card)
{
  static struct Interrupt interrupt;

  interrupt.is_Code = (void (*)())ResetHandler;
  interrupt.is_Data = (APTR) card;
  interrupt.is_Node.ln_Pri = 0;
  interrupt.is_Node.ln_Type = NT_EXTINTERRUPT;
  interrupt.is_Node.ln_Name = "SB128 Reset Handler";

  IExec->AddResetCallback( &interrupt );
}

