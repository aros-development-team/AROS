/*

Copyright (C) 2010-2012 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/timer.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include "device.h"


#ifdef AH_DEBUG
static   int ath_hal_debug = 1;
#endif

int   ath_hal_dma_beacon_response_time = 2;   /* in TU's */
int   ath_hal_sw_beacon_response_time = 10;   /* in TU's */
int   ath_hal_additional_swba_backoff = 0;   /* in TU's */

struct DevBase *hal_dev_base;

#define base hal_dev_base


void ath_hal_vprintf(APTR ah, const char* fmt, va_list ap)
{
}



void ath_hal_printf(APTR ah, const char* fmt, ...)
{
}



void HALDEBUG(APTR ah, unsigned int mask, const char* fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
#ifdef __AROS__
   kprintf("[atheros] ");
   vkprintf(fmt, ap);
#else
   ath_hal_vprintf(ah, fmt, ap);
#endif
   va_end(ap);
}



const char *ath_hal_ether_sprintf(const UBYTE *mac)
{
   static char etherbuf[18];
#if 0
   snprintf(etherbuf, sizeof(etherbuf), "%02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#else
etherbuf[0] = '\0';
#endif
   return etherbuf;
}



void ath_hal_delay(int n)
{
   struct timeval time, end_time;

   GetSysTime(&end_time);
   time.tv_secs = 0;
   time.tv_micro = n;
   AddTime(&end_time, &time);

   while(CmpTime(&end_time, &time) < 0)
      GetSysTime(&time);

   return;
}



ULONG ath_hal_getuptime(APTR ah)
{
   struct timeval time;

   GetSysTime(&time);

   return time.tv_secs * 1000 + time.tv_micro / 1000;
}



void *ath_hal_malloc(size_t size)
{
   return AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR);
}



void ath_hal_free(void* p)
{
   FreeVec(p);
}



void ath_hal_memzero(void *dst, size_t n)
{
   UBYTE *buf = dst;

   while(n-- != 0)
      *buf++ = 0;
}



void *ath_hal_memcpy(void *dst, const void *src, size_t n)
{
   CopyMem(src, dst, n);

   return dst;
}



int ath_hal_memcmp(const void *a, const void *b, size_t n)
{
   const UBYTE *ba = a, *bb = b;

   while(n != 0 && *ba == *bb)
      ba++, bb++;
   return *ba - *bb;
}



