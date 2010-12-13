/*
     emu10kx.audio - AHI driver for SoundBlaster Live! series
     Copyright (C) 2002-2005 Martin Blom <martin@blom.org>

     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <config.h>

#include <libraries/ahi_sub.h>
#include <exec/execbase.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>

#include "library.h"
#include "8010.h"
#include "pci_wrapper.h"

#define min(a,b) ((a)<(b)?(a):(b))

#ifdef __AMIGAOS4__
# define CallHookA CallHookPkt
#endif


static WORD*
copy_mono( WORD* src, WORD* dst, int count, int stride, BOOL src32, BOOL flush_caches );

static WORD*
copy_stereo( WORD* lsrc, WORD* rsrc, WORD* dst, int count, int stride, BOOL src32, BOOL flush_caches );


/******************************************************************************
** Hardware interrupt handler *************************************************
******************************************************************************/

#ifdef __AMIGAOS4__
LONG
EMU10kxInterrupt( struct ExceptionContext *pContext, struct ExecBase *SysBase, struct EMU10kxData* dd )
#else
ULONG
EMU10kxInterrupt( struct EMU10kxData* dd )
#endif
{
  struct AHIAudioCtrlDrv* AudioCtrl = dd->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) dd->ahisubbase;
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

  ULONG intreq;
  BOOL  handled = FALSE;
  
  while( ( intreq = ahi_pci_inl( dd->card.iobase + IPR, dd->card.pci_dev ) ) != 0 )
  {
//    KPrintF("IRQ: %08lx\n", intreq );
    if( intreq & IPR_INTERVALTIMER &&
	AudioCtrl != NULL )
    {
      int hw   = sblive_readptr( &dd->card, CCCA_CURRADDR, dd->voices[0].num );
      int diff = dd->current_position - ( hw - dd->voices[0].start );

      if( diff < 0 )
      {
         diff += AudioCtrl->ahiac_MaxBuffSamples * 2;
      }

//      KPrintF( ">>> hw_pos = %08lx; current_pos = %08lx; diff=%ld <<<\n",
//	       hw, dd->current_position, diff );

      if( (ULONG) diff < dd->current_length )
      {
         if( dd->playback_interrupt_enabled )
         {
            /* Invoke softint to fetch new sample data */

            dd->playback_interrupt_enabled = FALSE;
            Cause( &dd->playback_interrupt );
/* 	    KPrintF("hw[0]=%08lx, hw[1]=%08lx, hw[2]=%08lx, hw[3]=%08lx\n", */
/* 		    sblive_readptr( &dd->card, CCCA_CURRADDR, dd->voices[0].num ), */
/* 		    sblive_readptr( &dd->card, CCCA_CURRADDR, dd->voices[1].num ), */
/* 		    sblive_readptr( &dd->card, CCCA_CURRADDR, dd->voices[2].num ), */
/* 		    sblive_readptr( &dd->card, CCCA_CURRADDR, dd->voices[3].num ) ); */
         }
      }
    }

    if( intreq & ( IPR_ADCBUFHALFFULL | IPR_ADCBUFFULL ) )
    {
      if( intreq & IPR_ADCBUFHALFFULL )
      {
         dd->current_record_buffer = dd->record_buffer;
      }
      else
      {
         dd->current_record_buffer = ( dd->record_buffer +
				      RECORD_BUFFER_SAMPLES * 4 / 2 );
      }

      if( dd->record_interrupt_enabled )
      {
         /* Invoke softint to convert and feed AHI with the new sample data */

         dd->record_interrupt_enabled = FALSE;
         Cause( &dd->record_interrupt );
      }
    }
    
    if( intreq & IPR_MIDIRECVBUFEMPTY )
    {
      unsigned char b;

      while( emu10k1_mpu_read_data( &dd->card, &b ) >= 0 )
      {
	struct ReceiveMessage msg = { b };

	if( dd->camd_receivefunc == NULL )
	{
	  KPrintF( "emu10kx.audio got unexpected IPR_MIDIRECVBUFEMPTY\n" );
	}

//	KPrintF( "\t%lx\n", (int) b );
	CallHookA( dd->camd_receivefunc, (Object*) EMU10kxBase, &msg );
      }
    }
 
    if( intreq & IPR_MIDITRANSBUFEMPTY )
    {
      ULONG b;

      if( dd->camd_transmitfunc == NULL )
      {
	KPrintF( "emu10kx.audio got unexpected IPR_MIDITRANSBUFEMPTY\n" );
      }

      b = CallHookA( dd->camd_transmitfunc, (Object*) EMU10kxBase, NULL );

//      KPrintF( "%08lx\n", b );

      // Check if d0.w is negative (as the V37 did once?) or if bit 8
      // is set (as the V40 example does)
      
      if( ( b & 0x00008100 ) == 0x0000 )
      {
	emu10k1_mpu_write_data( &dd->card, b );
      }

      if( ( !dd->camd_v40 && ( b & 0x00ff0000 ) != 0 ) ||
	  ( b & 0x00008100 ) != 0 )
      {
//	KPrintF( "Disabling interrupts\n" );
	emu10k1_irq_disable( &dd->card, INTE_MIDITXENABLE );	
      }
    }

    /* Clear interrupt pending bit(s) */
    ahi_pci_outl( intreq, dd->card.iobase + IPR, dd->card.pci_dev );

    handled = TRUE;
  }
  
  return handled;
}


/******************************************************************************
** Playback interrupt handler *************************************************
******************************************************************************/

#ifdef __AMIGAOS4__
void
PlaybackInterrupt( struct ExceptionContext *pContext, struct ExecBase *SysBase, struct EMU10kxData* dd )
#else
void
PlaybackInterrupt( struct EMU10kxData* dd )
#endif
{
  struct AHIAudioCtrlDrv* AudioCtrl = dd->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) dd->ahisubbase;
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

  if( dd->mix_buffer != NULL && dd->current_buffers[0] != NULL )
  {
    BOOL   skip_mix;

    WORD*  src;
    size_t skip;
    size_t samples;
    int    i, s;
    
    skip_mix = CallHookA( AudioCtrl->ahiac_PreTimerFunc, (Object*) AudioCtrl, 0 );

    CallHookA( AudioCtrl->ahiac_PlayerFunc, (Object*) AudioCtrl, NULL );

    if( ! skip_mix )
    {
      CallHookA( AudioCtrl->ahiac_MixerFunc, (Object*) AudioCtrl, dd->mix_buffer );
    }
    
    /* Now translate and transfer to the DMA buffer */
    samples    = dd->current_length;

    s = min( samples,
	     AudioCtrl->ahiac_MaxBuffSamples * 2 - dd->current_position );

    src = dd->mix_buffer;
    
    switch( AudioCtrl->ahiac_BuffType)
    {
      case AHIST_M16S:
	dd->current_buffers[0] = copy_mono( src, dd->current_buffers[0],
					    s, 1, FALSE,
					    EMU10kxBase->flush_caches );
	src += s;
	break;
	
      case AHIST_S16S:
	dd->current_buffers[0] = copy_stereo( src, src + 1, dd->current_buffers[0],
					      s, 2, FALSE,
					      EMU10kxBase->flush_caches );
	src += s * 2;
	break;
	
      case AHIST_M32S:
	dd->current_buffers[0] = copy_mono( src, dd->current_buffers[0],
					    s, 2, TRUE,
					    EMU10kxBase->flush_caches );
	src += s * 2;
	break;
	
      case AHIST_S32S:
	dd->current_buffers[0] = copy_stereo( src, src + 2, dd->current_buffers[0],
					      s, 4, TRUE,
					      EMU10kxBase->flush_caches );
	src += s * 4;
	break;
	
      case AHIST_L7_1:
	dd->current_buffers[0] = copy_stereo( src, src + 2, dd->current_buffers[0],
					      s, 16, TRUE,
					      EMU10kxBase->flush_caches );
	dd->current_buffers[1] = copy_stereo( src + 4, src + 6, dd->current_buffers[1],
					      s, 16, TRUE,
					      EMU10kxBase->flush_caches );
	dd->current_buffers[2] = copy_stereo( src + 8, src + 10, dd->current_buffers[2],
					      s, 16, TRUE,
					      EMU10kxBase->flush_caches );
	dd->current_buffers[3] = copy_stereo( src + 12, src + 14, dd->current_buffers[3],
					      s, 16, TRUE,
					      EMU10kxBase->flush_caches );
	src += s * 16;
	break;
    }
    
    dd->current_position += s;
    samples -= s;

    if( dd->current_position == AudioCtrl->ahiac_MaxBuffSamples * 2 )
    {
      dd->current_buffers[0] = dd->voices[0].mem.addr;
      dd->current_buffers[1] = dd->voices[1].mem.addr;
      dd->current_buffers[2] = dd->voices[2].mem.addr;
      dd->current_buffers[3] = dd->voices[3].mem.addr;
      dd->current_position = 0;
    }

    if( samples > 0 )
    {
      s = samples;

      switch( AudioCtrl->ahiac_BuffType)
      {
	case AHIST_M16S:
	  dd->current_buffers[0] = copy_mono( src, dd->current_buffers[0],
					      s, 1, FALSE,
					      EMU10kxBase->flush_caches );
	  break;
	
	case AHIST_S16S:
	  dd->current_buffers[0] = copy_stereo( src, src + 1, dd->current_buffers[0],
						s, 2, FALSE,
						EMU10kxBase->flush_caches );
	  break;
	
	case AHIST_M32S:
	  dd->current_buffers[0] = copy_mono( src, dd->current_buffers[0],
					      s, 2, TRUE,
					      EMU10kxBase->flush_caches );
	  break;
	
	case AHIST_S32S:
	  dd->current_buffers[0] = copy_stereo( src, src + 2, dd->current_buffers[0],
						s, 4, TRUE,
						EMU10kxBase->flush_caches );
	  break;
	
	case AHIST_L7_1:
	  dd->current_buffers[0] = copy_stereo( src, src + 2, dd->current_buffers[0],
						s, 16, TRUE,
						EMU10kxBase->flush_caches );
	  dd->current_buffers[1] = copy_stereo( src + 4, src + 6, dd->current_buffers[1],
						s, 16, TRUE,
						EMU10kxBase->flush_caches );
	  dd->current_buffers[2] = copy_stereo( src + 8, src + 10, dd->current_buffers[2],
						s, 16, TRUE,
						EMU10kxBase->flush_caches );
	  dd->current_buffers[3] = copy_stereo( src + 12, src + 14, dd->current_buffers[3],
						s, 16, TRUE,
						EMU10kxBase->flush_caches );
	  break;
      }
    
      dd->current_position += s;
    }

    CallHookA( AudioCtrl->ahiac_PostTimerFunc, (Object*) AudioCtrl, 0 );
  }

  dd->playback_interrupt_enabled = TRUE;
}


static WORD*
copy_mono( WORD* src, WORD* dst, int count, int stride, BOOL src32, BOOL flush_caches )
{
  WORD* first = dst;
  WORD* last  = dst + count;
  int x, y;

#ifndef WORDS_BIGENDIAN
  if( src32 )
  {
    // Move to high 16 bits
    ++src;
  }
#endif

  for( x = 0, y = 0; y < count; x += stride, ++y )
  {
#ifndef WORDS_BIGENDIAN
    dst[y] = src[x];
#else
    dst[y] = ( ( src[x] & 0xff ) << 8 ) | ( ( src[x] & 0xff00 ) >> 8 );
#endif
  }

  if( flush_caches )
  {
    CacheClearE( first, (ULONG) last - (ULONG) first, CACRF_ClearD );
  }

  return last;
}


static WORD*
copy_stereo( WORD* lsrc, WORD* rsrc, WORD* dst, int count, int stride, BOOL src32, BOOL flush_caches )
{
  WORD* first = dst;
  WORD* last  = dst + count * 2;
  int x, y;

#ifndef WORDS_BIGENDIAN
  if( src32 )
  {
    // Move to high 16 bits
    ++lsrc;
    ++rsrc;
  }
#endif

  for( x = 0, y = 0; y < count * 2; x += stride, y += 2 )
  {
#ifndef WORDS_BIGENDIAN
    dst[y+0] = lsrc[x];
    dst[y+1] = rsrc[x];
#else
    dst[y+0] = ( ( lsrc[x] & 0xff ) << 8 ) | ( ( lsrc[x] & 0xff00 ) >> 8 );
    dst[y+1] = ( ( rsrc[x] & 0xff ) << 8 ) | ( ( rsrc[x] & 0xff00 ) >> 8 );
#endif
  }

  if( flush_caches )
  {
    CacheClearE( first, (ULONG) last - (ULONG) first, CACRF_ClearD );
  }

  return last;
}


/******************************************************************************
** Record interrupt handler ***************************************************
******************************************************************************/

#ifdef __AMIGAOS4__
void
RecordInterrupt( struct ExceptionContext *pContext, struct ExecBase *SysBase, struct EMU10kxData* dd )
#else
void
RecordInterrupt( struct EMU10kxData* dd )
#endif
{
  struct AHIAudioCtrlDrv* AudioCtrl = dd->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) dd->ahisubbase;
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;
#ifdef __AMIGAOS4__
  ULONG  CacheCommand = CACRF_InvalidateD;
#else
  ULONG  CacheCommand = CACRF_ClearD;
#endif


  struct AHIRecordMessage rm =
  {
    AHIST_S16S,
    dd->current_record_buffer,
    RECORD_BUFFER_SAMPLES / 2
  };

  int   i   = 0;
  WORD* ptr = dd->current_record_buffer;

#ifndef __AMIGAOS4__
  // As OS4 can do invalidate only, we don't need to do flushing here.
  // Between the invalidate at the end, DMA and entering this interrupt code,
  // nobody should have touched this half of the record buffer.
 
  if( EMU10kxBase->flush_caches )
  {
    // This is used to invalidate the cache

    CacheClearE( dd->current_record_buffer,
		 RECORD_BUFFER_SAMPLES / 2 * 4,
		 CacheCommand );
  }
#endif

#ifdef WORDS_BIGENDIAN
  while( i < RECORD_BUFFER_SAMPLES / 2 * 2 )
  {
    *ptr = ( ( *ptr & 0xff ) << 8 ) | ( ( *ptr & 0xff00 ) >> 8 );
    
    ++i;
    ++ptr;
  }
#endif

  CallHookA( AudioCtrl->ahiac_SamplerFunc, (Object*) AudioCtrl, &rm );

  if( EMU10kxBase->flush_caches )
  {
    // This is used to make sure the call above doesn't push dirty data
    // the next time it's called. God help us if dd->current_record_buffer
    // is not a the beginning of a cache line and there are dirty data
    // in the DMA buffer before or after the current buffer.
    
    CacheClearE( dd->current_record_buffer,
		 RECORD_BUFFER_SAMPLES / 2 * 4,
		 CacheCommand );
  }

  dd->record_interrupt_enabled = TRUE;
}


