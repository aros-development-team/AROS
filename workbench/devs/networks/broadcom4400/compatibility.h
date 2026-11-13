/*

Copyright (C) 2001-2020 Neil Cafferkey

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

#ifndef COMPATIBILITY_H
#define COMPATIBILITY_H


#include <exec/types.h>


#ifndef UPINT
#ifdef __AROS__
typedef IPTR UPINT;
typedef SIPTR PINT;
#else
typedef ULONG UPINT;
typedef LONG PINT;
#endif
#endif

#ifndef REG
#if defined(__mc68000) && !defined(__AROS__)
#define _REG(A, B) B __asm(#A)
#define REG(A, B) _REG(A, B)
#else
#define REG(A, B) B
#endif
#endif

#define _STR(A) #A
#define STR(A) _STR(A)

#ifndef __AROS__
#define USE_HACKS
#endif

#ifndef BASE_REG
#define BASE_REG a6
#endif

#ifdef __amigaos4__
#undef CachePreDMA
#define CachePreDMA(address, length, flags) \
   ({ \
      struct DMAEntry _dma_entry = {0}; \
      if(StartDMA((address), *(length), (flags)) == 1) \
         GetDMAList((address), *(length), (flags), &_dma_entry); \
      *(length) = _dma_entry.BlockLength; \
      (ULONG)_dma_entry.PhysicalAddress | (ULONG)address & 0xfff; \
   })
#undef CachePostDMA
#define CachePostDMA(address, length, flags) \
   EndDMA(address, *(length), flags);
#endif


#endif
