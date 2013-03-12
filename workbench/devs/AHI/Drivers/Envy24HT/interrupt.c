/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#include <config.h>

#undef __USE_INLINE__
#include <proto/expansion.h>
#include <libraries/ahi_sub.h>
#include <proto/exec.h>
#include <stddef.h>
//#include "library.h"
#include "regs.h"
#include "interrupt.h"
#include "misc.h"
#include "pci_wrapper.h"

#define min(a,b) ((a)<(b)?(a):(b))

#ifdef __MORPHOS__
#undef CALLHOOK
#define CALLHOOK CallHookA
#endif
#include <clib/alib_protos.h>

//int z = 0;

/******************************************************************************
** Hardware interrupt handler *************************************************
******************************************************************************/

#ifdef __AMIGAOS4__
LONG
CardInterrupt( struct ExceptionContext *pContext, struct ExecBase *SysBase, struct CardData* card )
#else
ULONG
CardInterrupt( struct CardData* card )
#endif
{
  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;
  struct PCIDevice *dev = (struct PCIDevice * ) card->pci_dev;

  unsigned char intreq;
  LONG  handled = 0;

  if ( ( intreq = INBYTE(card->iobase + CCS_INTR_STATUS ) ) != 0 )
  {
    //DEBUGPRINTF("INT %x\n", intreq);
    
    if (intreq & CCS_INTR_PLAYREC)
    {
       unsigned char mtstatus = INBYTE(card->mtbase + MT_INTR_STATUS);
       
       if( (mtstatus & MT_PDMA0) && AudioCtrl != NULL && card->is_playing)
       {
         unsigned long diff = INLONG(card->mtbase + MT_DMAI_PB_ADDRESS) - (unsigned long) card->playback_buffer_phys;
         
         OUTBYTE(card->mtbase + MT_INTR_STATUS, mtstatus | MT_PDMA0); // clear interrupt
         
         /*if (z == 0)
            DEBUGPRINTF("MT_PDMA0, diff = %lu, %lu\n", diff, card->current_frames);
         z++;
         if (z == 5)
            z = 0;*/
         
         if (diff >= card->current_bytesize) //card->flip == 0) // just played buf 1
         {
            card->flip = 1;
            card->current_buffer = card->playback_buffer;
            card->spdif_out_current_buffer = card->spdif_out_buffer;
            }
         else  // just played buf 2
         {
            card->flip = 0;
            card->current_buffer = (APTR) ((long) card->playback_buffer + card->current_bytesize);
            card->spdif_out_current_buffer = (APTR) ((long) card->spdif_out_buffer + card->current_bytesize / 2);
         }
         
         card->playback_interrupt_enabled = FALSE;
         CAUSE( &card->playback_interrupt );
       }

       if( ((mtstatus & MT_RDMA0) || (mtstatus & MT_RDMA1)) && AudioCtrl != NULL  && card->is_recording)
       {
         OUTBYTE(card->mtbase + MT_INTR_STATUS, mtstatus | MT_RDMA0 | MT_RDMA1); // clear interrupt
         
         /*if (z == 0)
            DEBUGPRINTF("MT_RDMA%d\n", mtstatus);
         z++;
         if (z == 50)
            z = 0;*/
   
         if( card->record_interrupt_enabled )
         {
            /* Invoke softint to convert and feed AHI with the new sample data */
   
            if (card->recflip == 0) // just played buf 1
            {
               card->recflip = 1;
               card->current_record_buffer = card->record_buffer;
               card->current_record_buffer_32bit = card->record_buffer_32bit;
               }
            else  // just played buf 2
            {
               card->recflip = 0;
               card->current_record_buffer = (APTR) ((long) card->record_buffer + card->current_record_bytesize_32bit / 2);
               card->current_record_buffer_32bit = (APTR) ((long) card->record_buffer_32bit + card->current_record_bytesize_32bit);
            }
   
            card->record_interrupt_enabled = FALSE;
            CAUSE( &card->record_interrupt );
         }
       }
   
       if( (mtstatus & MT_DMA_FIFO) && AudioCtrl != NULL )
       {
            unsigned char status = INBYTE(card->mtbase + MT_DMA_UNDERRUN);
            
            //DEBUGPRINTF("FIFO %x\n", status);
            WriteMask8(card, card->mtbase, MT_INTR_STATUS, MT_DMA_FIFO); // clear it
            
            OUTBYTE(card->mtbase + MT_DMA_UNDERRUN, status);
            WriteMask8(card, card->mtbase, MT_INTR_MASK, MT_DMA_FIFO);
       }
       
       
       OUTBYTE(card->mtbase + MT_INTR_STATUS, mtstatus); // clear interrupt
       handled = 1;
    }
  }

  return handled;
}


/******************************************************************************
** Playback interrupt handler *************************************************
******************************************************************************/

#ifdef __AMIGAOS4__
void
PlaybackInterrupt( struct ExceptionContext *pContext, struct ExecBase *SysBase, struct CardData* card )
#else
void
PlaybackInterrupt( struct CardData* card )
#endif
{
  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;
  
  if( card->mix_buffer != NULL && card->current_buffer != NULL && card->is_playing)
  {
    BOOL   skip_mix;

    WORD*  src;
    size_t skip;
    size_t samples;
    int    i;
    LONG* srclong, *dstlong, *spdif_dstlong, left, right;
    int frames = card->current_frames;
    
    skip_mix = CALLHOOK( AudioCtrl->ahiac_PreTimerFunc, (Object*) AudioCtrl, 0 );  
    CALLHOOK( AudioCtrl->ahiac_PlayerFunc, (Object*) AudioCtrl, NULL );

    //DEBUGPRINTF("skip_mix = %d\n", skip_mix);

    if( ! skip_mix )
    {
      CALLHOOK( AudioCtrl->ahiac_MixerFunc, (Object*) AudioCtrl, card->mix_buffer );
    }
    
    /* Now translate and transfer to the DMA buffer */

    skip    = ( AudioCtrl->ahiac_Flags & AHIACF_HIFI ) ? 2 : 1;
    samples = card->current_bytesize >> 1;

    src     = card->mix_buffer;
    
    srclong = (LONG*) card->mix_buffer;
    dstlong = (LONG*) card->current_buffer;
    spdif_dstlong = card->spdif_out_current_buffer;

    i = frames;
#if defined(__amigaos4__) || defined(__MORPHOS__)
    if (AudioCtrl->ahiac_Flags & AHIACF_HIFI)
    {
       while( i > 0 )
       {
         left = ((*srclong & 0xFF000000) >> 24) | ((*srclong & 0x00FF0000) >> 8) | ((*srclong & 0x0000FF00) << 8);
         *dstlong++ = *spdif_dstlong++ = left;
         srclong++;
         
         right = ((*srclong & 0xFF000000) >> 24) | ((*srclong & 0x00FF0000) >> 8) | ((*srclong & 0x0000FF00) << 8);
         *dstlong++ = *spdif_dstlong++ = right;
         srclong++;

         *dstlong++ = left; // L & R of 2nd stereo channel that is unused
         *dstlong++ = right;
   
         --i;
       }
    }
    else
    {
      while( i > 0 )
       {
         *dstlong++ = *spdif_dstlong++ = (*src & 0xFF00) >> 16; srclong++;
         *dstlong++ = *spdif_dstlong++ = (*srclong & 0xFF000000) >> 16; srclong++;
         
         *dstlong++ = 0; // L & R of 2nd stereo channel that is unused
         *dstlong++ = 0;
         --i;
       }
    }
#ifdef __AMIGAOS4__
    IExec->CacheClearE(card->current_buffer, (ULONG) dstlong - (ULONG) card->current_buffer, CACRF_ClearD);
    IExec->CacheClearE(card->spdif_out_current_buffer, (ULONG) spdif_dstlong - (ULONG) card->spdif_out_current_buffer, CACRF_ClearD);
#endif
#else
    if (AudioCtrl->ahiac_Flags & AHIACF_HIFI)
    {
       while( i > 0 )
       {
         left = *srclong;
         *dstlong++ = *spdif_dstlong++ = left;
         srclong++;
         
         right = *srclong;
         *dstlong++ = *spdif_dstlong++ = right;
         srclong++;

         *dstlong++ = left; // L & R of 2nd stereo channel that is unused
         *dstlong++ = right;
   
         --i;
       }
    }
    else
    {
      while( i > 0 )
       {
         *dstlong++ = *spdif_dstlong++ = (*src); src++;
         *dstlong++ = *spdif_dstlong++ = (*src); src++;
         
         *dstlong++ = 0; // L & R of 2nd stereo channel that is unused
         *dstlong++ = 0;
         --i;
       }
    }
#endif

    CALLHOOK( AudioCtrl->ahiac_PostTimerFunc, (Object*) AudioCtrl, 0 );
  }

  card->playback_interrupt_enabled = TRUE;
}


/******************************************************************************
** Record interrupt handler ***************************************************
******************************************************************************/

#ifdef __AMIGAOS4__
void
RecordInterrupt( struct ExceptionContext *pContext, struct ExecBase *SysBase, struct CardData* card )
#else
void
RecordInterrupt( struct CardData* card )
#endif
{
  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;
  int   i   = 0, frames = card->current_record_bytesize_32bit / 4;

  if (card->input_is_24bits)
  {
     struct AHIRecordMessage rm =
     {
       AHIST_S32S,
       card->current_record_buffer_32bit,
       RECORD_BUFFER_SAMPLES
     };
   
#if defined(__amigaos4__) || defined(__MORPHOS__)
     long *srclong = card->current_record_buffer_32bit;
   
     while( i < frames )
     {
       *srclong =  ((*srclong & 0x00FF0000) >> 8) | ((*srclong & 0x0000FF00) << 8) | ((*srclong & 0x000000FF) << 24);
   
       ++i;
       ++srclong;
     }
#endif
     
     CALLHOOK( AudioCtrl->ahiac_SamplerFunc, (Object*) AudioCtrl, &rm );
  }
  else
  {
     struct AHIRecordMessage rm =
     {
       AHIST_S16S,
       card->current_record_buffer,
       RECORD_BUFFER_SAMPLES
     };

     long *src = card->current_record_buffer_32bit;
     WORD* dst = card->current_record_buffer;

#if defined(__amigaos4__) || defined(__MORPHOS__)
     while( i < frames )
     {
       *dst = ( ( *src & 0x000000FF ) << 8 ) | ( ( *src & 0x0000FF00 ) >> 8 );
   
       ++i;
       ++src;
       ++dst;
     }
#else
     while( i < frames )
     {
       *dst = (*src) >> 16;
   
       ++i;
       ++src;
       ++dst;
     }
#endif
     
     CALLHOOK( AudioCtrl->ahiac_SamplerFunc, (Object*) AudioCtrl, &rm );
  }

  card->record_interrupt_enabled = TRUE;
}


