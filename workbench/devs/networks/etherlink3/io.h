/*

Copyright (C) 2004,2005 Neil Cafferkey

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

#ifndef IO_H
#define IO_H


#include <exec/types.h>
#include <utility/tagitem.h>

#include "endian.h"


/* I/O tags */

#define IOTAG_ByteIn (TAG_USER + 0)
#define IOTAG_WordIn (TAG_USER + 1)
#define IOTAG_LongIn (TAG_USER + 2)
#define IOTAG_QuadIn (TAG_USER + 3)
#define IOTAG_ByteOut (TAG_USER + 4)
#define IOTAG_WordOut (TAG_USER + 5)
#define IOTAG_LongOut (TAG_USER + 6)
#define IOTAG_QuadOut (TAG_USER + 7)
#define IOTAG_BytesIn (TAG_USER + 8)
#define IOTAG_WordsIn (TAG_USER + 9)
#define IOTAG_LongsIn (TAG_USER + 10)
#define IOTAG_QuadsIn (TAG_USER + 11)
#define IOTAG_BytesOut (TAG_USER + 12)
#define IOTAG_WordsOut (TAG_USER + 13)
#define IOTAG_LongsOut (TAG_USER + 14)
#define IOTAG_QuadsOut (TAG_USER + 15)
#define IOTAG_BEWordIn (TAG_USER + 16)
#define IOTAG_BELongIn (TAG_USER + 17)
#define IOTAG_BEQuadIn (TAG_USER + 18)
#define IOTAG_BEWordOut (TAG_USER + 19)
#define IOTAG_BELongOut (TAG_USER + 20)
#define IOTAG_BEQuadOut (TAG_USER + 21)
#define IOTAG_BEWordsIn (TAG_USER + 22)
#define IOTAG_BELongsIn (TAG_USER + 23)
#define IOTAG_BEQuadsIn (TAG_USER + 24)
#define IOTAG_BEWordsOut (TAG_USER + 25)
#define IOTAG_BELongsOut (TAG_USER + 26)
#define IOTAG_BEQuadsOut (TAG_USER + 27)
#define IOTAG_LEWordIn (TAG_USER + 28)
#define IOTAG_LELongIn (TAG_USER + 29)
#define IOTAG_LEQuadIn (TAG_USER + 30)
#define IOTAG_LEWordOut (TAG_USER + 31)
#define IOTAG_LELongOut (TAG_USER + 32)
#define IOTAG_LEQuadOut (TAG_USER + 33)
#define IOTAG_LEWordsIn (TAG_USER + 34)
#define IOTAG_LELongsIn (TAG_USER + 35)
#define IOTAG_LEQuadsIn (TAG_USER + 36)
#define IOTAG_LEWordsOut (TAG_USER + 37)
#define IOTAG_LELongsOut (TAG_USER + 38)
#define IOTAG_LEQuadsOut (TAG_USER + 39)
#define IOTAG_AllocDMAMem (TAG_USER + 40)
#define IOTAG_FreeDMAMem (TAG_USER + 41)


/* I/O macros */

#ifdef __i386__

#define BYTEIN(address) \
({ \
   UBYTE _BYTEIN_value; \
   __asm volatile ("inb %w1,%0":"=a" (_BYTEIN_value):"Nd" (address)); \
   _BYTEIN_value; \
})

#define WORDIN(address) \
({ \
   UWORD _WORDIN_value; \
   __asm volatile ("inw %w1,%0":"=a" (_WORDIN_value):"Nd" (address)); \
   _WORDIN_value; \
})

#define LONGIN(address) \
({ \
   ULONG _LONGIN_value; \
   __asm volatile ("inl %w1,%0":"=a" (_LONGIN_value):"Nd" (address)); \
   _LONGIN_value; \
})

#define BYTEOUT(address, value) \
({ \
   __asm volatile ("outb %b0,%w1": :"a" (value), "Nd" (address)); \
})

#define WORDOUT(address, value) \
({ \
   __asm volatile ("outw %w0,%w1": :"a" (value), "Nd" (address)); \
})

#define LONGOUT(address, value) \
({ \
   __asm volatile ("outl %0,%w1": :"a" (value), "Nd" (address)); \
})

#else

#define BYTEIN(address) \
   (*((volatile UBYTE *)(address)))

#define WORDIN(address) \
   (*((volatile UWORD *)(address)))

#define LONGIN(address) \
   (*((volatile ULONG *)(address)))

#define BYTEOUT(address, value) \
   *((volatile UBYTE *)(address)) = (value)

#define WORDOUT(address, value) \
   *((volatile UWORD *)(address)) = (value)

#define LONGOUT(address, value) \
   *((volatile ULONG *)(address)) = (value)

#endif

#define WORDSIN(address, data, count) \
   ({ \
      ULONG _WORDSIN_i = (count); \
      UWORD *_WORDSIN_p = (data); \
      while(_WORDSIN_i-- != 0) \
         *_WORDSIN_p++ = WORDIN(address); \
      _WORDSIN_p; \
   })

#define LONGSIN(address, data, count) \
   ({ \
      ULONG _LONGSIN_i = (count); \
      ULONG *_LONGSIN_p = (data); \
      while(_LONGSIN_i-- != 0) \
         *_LONGSIN_p++ = LONGIN(address); \
      _LONGSIN_p; \
   })

#define WORDSOUT(address, data, count) \
   do \
   { \
      ULONG _WORDSOUT_i = (count); \
      const UWORD *_WORDSOUT_p = (data); \
      while(_WORDSOUT_i-- != 0) \
         WORDOUT(address, *_WORDSOUT_p++); \
   } \
   while(0)

#define LONGSOUT(address, data, count) \
   do \
   { \
      ULONG _LONGSOUT_i = (count); \
      const ULONG *_LONGSOUT_p = (data); \
      while(_LONGSOUT_i-- != 0) \
         LONGOUT(address, *_LONGSOUT_p++); \
   } \
   while(0)

#define BEWORDOUT(address, value) \
   WORDOUT(address, MakeBEWord(value))

#define LEWORDIN(address) \
   LEWord(WORDIN(address))

#define LEWORDOUT(address, value) \
   WORDOUT(address, MakeLEWord(value))

#define LELONGIN(address) \
   LELong(LONGIN(address))

#define LELONGOUT(address, value) \
   LONGOUT(address, MakeLELong(value))

#define LEWORDSIN(address, data, count) \
   ({ \
      ULONG _LEWORDSIN_i = (count); \
      UWORD *_LEWORDSIN_p = (data); \
      while(_LEWORDSIN_i-- != 0) \
         *_LEWORDSIN_p++ = LEWORDIN(address); \
      _LEWORDSIN_p; \
   })


#endif
